#include "imageobject.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QLineF>

ImageObject::ImageObject(QGraphicsItem *parent)
    : VectorObject(parent)
    , m_size(200, 200)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

VectorObject* ImageObject::clone() const
{
    ImageObject *copy = new ImageObject();
    copy->setImage(m_pixmap);
    copy->setSize(m_size);
    copy->setImagePath(m_imagePath);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setScale(scale());
    copy->setStrokeColor(strokeColor());
    copy->setFillColor(fillColor());
    copy->setStrokeWidth(strokeWidth());
    copy->setObjectOpacity(objectOpacity());
    return copy;
}

void ImageObject::setImage(const QPixmap &pixmap)
{
    prepareGeometryChange();
    m_pixmap = pixmap;

    // Auto-size to image if not already sized
    if (m_size.width() == 200 && m_size.height() == 200) {
        m_size = m_pixmap.size();

        // Limit to reasonable size
        if (m_size.width() > 800) {
            qreal ratio = 800.0 / m_size.width();
            m_size = QSizeF(800, m_size.height() * ratio);
        }
    }

    update();
}

void ImageObject::setImagePath(const QString &path)
{
    m_imagePath = path;
    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        setImage(pixmap);
    }
}

void ImageObject::setSize(const QSizeF &size)
{
    prepareGeometryChange();
    m_size = size;
    update();
}

QRectF ImageObject::boundingRect() const
{
    return QRectF(QPointF(0, 0), m_size);
}

void ImageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_pixmap.isNull()) {
        painter->drawPixmap(boundingRect().toRect(), m_pixmap);
    } else {
        // Placeholder if image not loaded
        painter->fillRect(boundingRect(), QColor(100, 100, 100));
        painter->setPen(Qt::white);
        painter->drawText(boundingRect(), Qt::AlignCenter, "Image");
    }

    // Draw selection outline if selected
    if (isSelected()) {
        QPen selectPen(Qt::blue, 2, Qt::DashLine);
        painter->setPen(selectPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());

        // Draw resize handles
        const qreal handleSize = 8;
        QRectF bounds = boundingRect();

        // Corner handles
        painter->setBrush(Qt::white);
        painter->setPen(Qt::blue);
        painter->drawRect(bounds.topLeft().x() - handleSize/2,
                          bounds.topLeft().y() - handleSize/2,
                          handleSize, handleSize);
        painter->drawRect(bounds.topRight().x() - handleSize/2,
                          bounds.topRight().y() - handleSize/2,
                          handleSize, handleSize);
        painter->drawRect(bounds.bottomLeft().x() - handleSize/2,
                          bounds.bottomLeft().y() - handleSize/2,
                          handleSize, handleSize);
        painter->drawRect(bounds.bottomRight().x() - handleSize/2,
                          bounds.bottomRight().y() - handleSize/2,
                          handleSize, handleSize);
    }
}

// ── Resize handle interaction (#18) ──────────────────────────────────────────
// Hit-test a corner handle in scene coordinates.
static int cornerHit(const QPointF &scenePos, const QRectF &sb)
{
    const qreal R = 12.0;
    if (QLineF(scenePos, sb.topLeft()).length()     < R) return 0;
    if (QLineF(scenePos, sb.topRight()).length()    < R) return 1;
    if (QLineF(scenePos, sb.bottomLeft()).length()  < R) return 2;
    if (QLineF(scenePos, sb.bottomRight()).length() < R) return 3;
    return -1;
}

void ImageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_pressScenePos = event->scenePos();
    m_pressSize     = m_size;
    m_resizing      = false;

    if (isSelected()) {
        QRectF sb = mapToScene(boundingRect()).boundingRect();
        int c = cornerHit(event->scenePos(), sb);
        if (c >= 0) { m_resizing = true; m_resizeCorner = c; event->accept(); return; }
    }
    QGraphicsItem::mousePressEvent(event);
}

void ImageObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_resizing) { QGraphicsItem::mouseMoveEvent(event); return; }

    QPointF d = event->scenePos() - m_pressScenePos;
    qreal nw = m_pressSize.width(), nh = m_pressSize.height();
    switch (m_resizeCorner) {
    case 0: nw = qMax(20.0, m_pressSize.width()-d.x()); nh = qMax(20.0, m_pressSize.height()-d.y()); break;
    case 1: nw = qMax(20.0, m_pressSize.width()+d.x()); nh = qMax(20.0, m_pressSize.height()-d.y()); break;
    case 2: nw = qMax(20.0, m_pressSize.width()-d.x()); nh = qMax(20.0, m_pressSize.height()+d.y()); break;
    case 3: nw = qMax(20.0, m_pressSize.width()+d.x()); nh = qMax(20.0, m_pressSize.height()+d.y()); break;
    }
    // Shift = lock aspect ratio
    if ((event->modifiers() & Qt::ShiftModifier) && m_pressSize.height() > 0) {
        qreal asp = m_pressSize.width() / m_pressSize.height();
        if (qAbs(nw-m_pressSize.width()) >= qAbs(nh-m_pressSize.height())) nh = nw/asp;
        else                                                                  nw = nh*asp;
    }
    prepareGeometryChange();
    m_size = QSizeF(nw, nh);
    update();
    event->accept();
}

void ImageObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_resizing = false;
    QGraphicsItem::mouseReleaseEvent(event);
}
