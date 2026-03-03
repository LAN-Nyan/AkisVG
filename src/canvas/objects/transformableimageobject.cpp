#include "transformableimageobject.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <cmath>
#include <optional>

// ─── ctor ─────────────────────────────────────────────────────────────────────

TransformableImageObject::TransformableImageObject(const QImage &image,
                                                   QGraphicsItem *parent)
    : VectorObject(parent)
    , m_image(image)
    , m_pixmap(QPixmap::fromImage(image))
    , m_w(image.width())
    , m_h(image.height())
    , m_pos(0, 0)
    , m_origW(image.width())
    , m_origH(image.height())
    , m_origAngle(0.0)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false); // we handle selection ourselves
}

// ─── VectorObject clone ───────────────────────────────────────────────────────

VectorObject* TransformableImageObject::clone() const
{
    auto *copy = new TransformableImageObject(m_image);
    copy->m_w     = m_w;
    copy->m_h     = m_h;
    copy->m_pos   = m_pos;
    copy->m_angle = m_angle;
    return copy;
}

// ─── transforms ───────────────────────────────────────────────────────────────

QTransform TransformableImageObject::localToScene() const
{
    QTransform t;
    t.translate(m_pos.x(), m_pos.y());
    t.rotate(m_angle);
    return t;
}

QTransform TransformableImageObject::sceneToLocal() const
{
    return localToScene().inverted();
}

// ─── QGraphicsItem interface ──────────────────────────────────────────────────

QRectF TransformableImageObject::boundingRect() const
{
    QPolygonF corners;
    QTransform t = localToScene();
    corners << t.map(QPointF(-m_w/2, -m_h/2))
            << t.map(QPointF( m_w/2, -m_h/2))
            << t.map(QPointF( m_w/2,  m_h/2))
            << t.map(QPointF(-m_w/2,  m_h/2));
    return corners.boundingRect().adjusted(-HANDLE_RADIUS*2, -HANDLE_RADIUS*2-ROTATE_HANDLE_DIST,
                                            HANDLE_RADIUS*2,  HANDLE_RADIUS*2);
}

void TransformableImageObject::paint(QPainter *painter,
                                     const QStyleOptionGraphicsItem * /*option*/,
                                     QWidget * /*widget*/)
{
    painter->save();
    painter->translate(m_pos);
    painter->rotate(m_angle);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(QRectF(-m_w/2, -m_h/2, m_w, m_h).toRect(), m_pixmap);
    painter->restore();
}

// ─── hit tests ───────────────────────────────────────────────────────────────

bool TransformableImageObject::hitTestImage(QPointF scenePos) const
{
    QPointF local = sceneToLocal().map(scenePos);
    return localRect().contains(local);
}

std::optional<HandleRole> TransformableImageObject::hitTestHandle(QPointF scenePos) const
{
    if (!m_selected)
        return std::nullopt;

    QVector<QPointF> handles = handlePositions();
    const HandleRole roles[] = {
        HandleRole::TopLeft,    HandleRole::TopCenter,    HandleRole::TopRight,
        HandleRole::MiddleLeft,                           HandleRole::MiddleRight,
        HandleRole::BottomLeft, HandleRole::BottomCenter, HandleRole::BottomRight,
        HandleRole::Rotate
    };

    for (int i = 0; i < handles.size(); ++i) {
        QPointF delta = scenePos - handles[i];
        if (std::sqrt(delta.x()*delta.x() + delta.y()*delta.y()) <= HANDLE_RADIUS * 2.0)
            return roles[i];
    }
    return std::nullopt;
}

// ─── handle positions ─────────────────────────────────────────────────────────

QVector<QPointF> TransformableImageObject::handlePositions() const
{
    QTransform t = localToScene();
    qreal hw = m_w / 2, hh = m_h / 2;

    QVector<QPointF> pts;
    pts << t.map(QPointF(-hw, -hh))   // TL
        << t.map(QPointF(  0, -hh))   // TC
        << t.map(QPointF( hw, -hh))   // TR
        << t.map(QPointF(-hw,   0))   // ML
        << t.map(QPointF( hw,   0))   // MR
        << t.map(QPointF(-hw,  hh))   // BL
        << t.map(QPointF(  0,  hh))   // BC
        << t.map(QPointF( hw,  hh));  // BR
    // Rotate handle above TC
    pts << t.map(QPointF(0, -hh - ROTATE_HANDLE_DIST));
    return pts;
}

// ─── drawHandles ──────────────────────────────────────────────────────────────

void TransformableImageObject::drawHandles(QPainter *painter) const
{
    if (!m_selected)
        return;

    QVector<QPointF> handles = handlePositions();
    QTransform t = localToScene();
    qreal hw = m_w / 2, hh = m_h / 2;

    painter->save();

    // Dashed bounding box
    QPen boxPen(QColor(0, 120, 255), 1.0, Qt::DashLine);
    boxPen.setDashPattern({5, 3});
    painter->setPen(boxPen);
    painter->setBrush(Qt::NoBrush);
    QPolygonF box;
    box << t.map(QPointF(-hw,-hh)) << t.map(QPointF(hw,-hh))
        << t.map(QPointF( hw, hh)) << t.map(QPointF(-hw, hh));
    painter->drawPolygon(box);

    // Line from TC to rotate handle
    painter->drawLine(handles[1], handles[8]);

    // Corner / edge handles (square, white fill)
    for (int i = 0; i < 8; ++i) {
        painter->setPen(QPen(QColor(0, 80, 200), 1.5));
        painter->setBrush(Qt::white);
        painter->drawRect(QRectF(handles[i] - QPointF(HANDLE_RADIUS, HANDLE_RADIUS),
                                 QSizeF(HANDLE_RADIUS*2, HANDLE_RADIUS*2)));
    }

    // Rotate handle (orange circle)
    painter->setPen(QPen(QColor(255, 160, 0), 1.5));
    painter->setBrush(QColor(255, 220, 100, 200));
    painter->drawEllipse(handles[8], HANDLE_RADIUS, HANDLE_RADIUS);

    painter->restore();
}

// ─── transform drag ──────────────────────────────────────────────────────────

void TransformableImageObject::beginTransform(HandleRole role, QPointF startScene)
{
    m_activeHandle = role;
    m_dragStart    = startScene;
    m_origPos      = m_pos;
    m_origW        = m_w;
    m_origH        = m_h;
    m_origAngle    = m_angle;
    m_transforming = true;
}

void TransformableImageObject::continueTransform(QPointF currentScene)
{
    if (!m_transforming)
        return;

    prepareGeometryChange();

    if (m_activeHandle == HandleRole::Rotate) {
        QPointF fromCentre = currentScene - m_origPos;
        m_angle = std::atan2(fromCentre.y(), fromCentre.x()) * 180.0 / M_PI + 90.0;
        return;
    }

    QPointF delta = currentScene - m_dragStart;

    // Decompose delta into local (un-rotated) axes
    qreal rad  = m_origAngle * M_PI / 180.0;
    qreal cosA = std::cos(rad), sinA = std::sin(rad);
    qreal dx   =  delta.x() * cosA + delta.y() * sinA;
    qreal dy   = -delta.x() * sinA + delta.y() * cosA;

    switch (m_activeHandle) {
    case HandleRole::TopLeft:
        m_w = qMax(10.0, m_origW - dx);
        m_h = qMax(10.0, m_origH - dy);
        m_pos = m_origPos + QPointF(-(m_w-m_origW)/2*cosA + (m_h-m_origH)/2*sinA,
                                     -(m_w-m_origW)/2*sinA - (m_h-m_origH)/2*cosA);
        break;
    case HandleRole::TopRight:
        m_w = qMax(10.0, m_origW + dx);
        m_h = qMax(10.0, m_origH - dy);
        m_pos = m_origPos + QPointF((m_w-m_origW)/2*cosA + (m_h-m_origH)/2*sinA,
                                     (m_w-m_origW)/2*sinA - (m_h-m_origH)/2*cosA);
        break;
    case HandleRole::BottomLeft:
        m_w = qMax(10.0, m_origW - dx);
        m_h = qMax(10.0, m_origH + dy);
        m_pos = m_origPos + QPointF(-(m_w-m_origW)/2*cosA - (m_h-m_origH)/2*sinA,
                                      -(m_w-m_origW)/2*sinA + (m_h-m_origH)/2*cosA);
        break;
    case HandleRole::BottomRight:
        m_w = qMax(10.0, m_origW + dx);
        m_h = qMax(10.0, m_origH + dy);
        m_pos = m_origPos + QPointF((m_w-m_origW)/2*cosA - (m_h-m_origH)/2*sinA,
                                     (m_w-m_origW)/2*sinA + (m_h-m_origH)/2*cosA);
        break;
    case HandleRole::TopCenter:
        m_h = qMax(10.0, m_origH - dy);
        m_pos = m_origPos + QPointF( (m_h-m_origH)/2*sinA, -(m_h-m_origH)/2*cosA);
        break;
    case HandleRole::BottomCenter:
        m_h = qMax(10.0, m_origH + dy);
        m_pos = m_origPos + QPointF(-(m_h-m_origH)/2*sinA, (m_h-m_origH)/2*cosA);
        break;
    case HandleRole::MiddleLeft:
        m_w = qMax(10.0, m_origW - dx);
        m_pos = m_origPos + QPointF(-(m_w-m_origW)/2*cosA, -(m_w-m_origW)/2*sinA);
        break;
    case HandleRole::MiddleRight:
        m_w = qMax(10.0, m_origW + dx);
        m_pos = m_origPos + QPointF((m_w-m_origW)/2*cosA, (m_w-m_origW)/2*sinA);
        break;
    default:
        break;
    }
}

void TransformableImageObject::endTransform()
{
    m_transforming = false;
}
