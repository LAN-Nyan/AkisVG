#include "splineoverlay.h"
#include "canvasview.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainterPath>
#include <QtMath>
#include <QGraphicsView>

SplineOverlay::SplineOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);

    // CRITICAL: This allows the widget to grab the keyboard
    setFocusPolicy(Qt::StrongFocus);

    setAttribute(Qt::WA_NoSystemBackground, true);
    setStyleSheet("background: transparent;");
}

// ── Scene ↔ viewport coordinate helpers ──────────────────────────────────────

// The overlay is parented to the viewport of a CanvasView.
// We need to convert between viewport pixel coords (used for mouse events / painting)
// and scene (canvas) coords (stored in m_nodes so they stay correct under zoom/pan).
static QGraphicsView* parentView(const QWidget *overlay)
{
    // overlay's parent is the viewport; viewport's parent is the QGraphicsView
    if (!overlay->parentWidget()) return nullptr;
    return qobject_cast<QGraphicsView*>(overlay->parentWidget()->parent());
}

// Returns the canvas rect in viewport (pixel) coordinates.
// The spline overlay must always be visually constrained to this rect.
static QRectF canvasViewportRect(const QWidget *overlay)
{
    auto *view = parentView(overlay);
    if (!view) return QRectF();
    // The scene rect IS the canvas rect; map its corners to viewport pixels.
    QRectF sr = view->scene() ? view->scene()->sceneRect() : QRectF();
    if (sr.isEmpty()) return QRectF(QPointF(0,0), QSizeF(overlay->size()));
    QPointF tl = view->mapFromScene(sr.topLeft());
    QPointF br = view->mapFromScene(sr.bottomRight());
    return QRectF(tl, br).normalized();
}

QPointF SplineOverlay::sceneToViewport(const QPointF &scenePos) const
{
    if (auto *view = parentView(this))
        return view->mapFromScene(scenePos);
    return scenePos;
}

QPointF SplineOverlay::viewportToScene(const QPointF &vp) const
{
    if (auto *view = parentView(this))
        return view->mapToScene(vp.toPoint());
    return vp;
}

void SplineOverlay::setNodes(const QList<QPointF> &nodes)
{
    m_nodes = nodes;
    update();
    emit splineChanged(m_nodes);
}

void SplineOverlay::clearNodes()
{
    m_nodes.clear();
    m_draggingIdx = -1;
    update();
    emit splineChanged(m_nodes);
}

void SplineOverlay::commitSpline()
{
    emit committed(m_nodes);
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void SplineOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // FIX #30: Restrict all painting to the exact canvas rect in viewport coords
    QRectF cvr = canvasViewportRect(this);
    if (cvr.isEmpty()) cvr = QRectF(rect()); // fallback: full viewport

    // Semi-transparent dark overlay — only within the canvas bounds
    p.fillRect(cvr, QColor(0, 0, 0, 60));

    // Header banner drawn full-width at the top of the CANVAS rect (not full widget)
    QRectF banner(cvr.left(), cvr.top(), cvr.width(), 36);
    p.fillRect(banner, QColor(28, 28, 30, 230));
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(14);
    p.setFont(f);
    p.drawText(banner.adjusted(12, 0, -12, 0),
               Qt::AlignVCenter | Qt::AlignLeft,
               "✦  INTERPOLATING   —  Click canvas to add nodes  |  Drag to move  |  Del to remove  |  Enter to commit  |  Esc to cancel");

    if (m_nodes.isEmpty()) return;

    // Clip all further drawing to the canvas rect
    p.setClipRect(cvr);

    // Convert all scene nodes to viewport coords for painting
    QList<QPointF> vpNodes;
    for (const QPointF &n : m_nodes)
        vpNodes << sceneToViewport(n);

    // Draw spline curve
    drawSpline(p, vpNodes);

    // Draw nodes
    for (int i = 0; i < vpNodes.size(); ++i) {
        const QPointF &pt = vpNodes[i];

        // Drop shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 80));
        p.drawEllipse(pt + QPointF(2, 2), NODE_RADIUS + 1, NODE_RADIUS + 1);

        // Node body
        QColor fill = (i == m_draggingIdx)
            ? QColor(255, 180, 0)
            : (i == 0 || i == m_nodes.size() - 1)
                ? QColor(42, 200, 100)
                : QColor(200, 50, 50);

        p.setBrush(fill);
        p.setPen(QPen(Qt::white, 2));
        p.drawEllipse(pt, NODE_RADIUS, NODE_RADIUS);

        // Frame label
        p.setPen(Qt::white);
        QFont labelFont = p.font();
        labelFont.setPixelSize(10);
        labelFont.setBold(true);
        p.setFont(labelFont);
        p.drawText(QRectF(pt.x() - 20, pt.y() + NODE_RADIUS + 3, 40, 16),
                   Qt::AlignCenter,
                   QString("K%1").arg(i + 1));
    }
}

void SplineOverlay::drawSpline(QPainter &p, const QList<QPointF> &vpNodes) const
{
    if (vpNodes.size() < 2) return;

    // Draw glow / shadow
    QPainterPath path;
    QList<QPointF> pts = catmullRomPoints(vpNodes, 30);
    if (pts.isEmpty()) return;

    path.moveTo(pts.first());
    for (int i = 1; i < pts.size(); ++i)
        path.lineTo(pts[i]);

    // Glow
    p.setPen(QPen(QColor(210, 45, 45, 50), 8));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    // Main line
    p.setPen(QPen(QColor(200, 50, 50), 2.5));
    p.drawPath(path);

    // Dashed extension lines
    p.setPen(QPen(QColor(255, 255, 255, 40), 1, Qt::DashLine));
    for (int i = 1; i < vpNodes.size(); ++i)
        p.drawLine(vpNodes[i - 1], vpNodes[i]);
}

QList<QPointF> SplineOverlay::catmullRomPoints(const QList<QPointF> &pts, int segments) const
{
    QList<QPointF> result;
    if (pts.size() < 2) return result;

    QList<QPointF> padded;
    padded << pts.first();
    padded << pts;
    padded << pts.last();

    for (int i = 1; i < padded.size() - 2; ++i) {
        QPointF p0 = padded[i - 1];
        QPointF p1 = padded[i];
        QPointF p2 = padded[i + 1];
        QPointF p3 = padded[i + 2];

        for (int s = 0; s <= segments; ++s) {
            qreal t = static_cast<qreal>(s) / segments;
            qreal t2 = t * t;
            qreal t3 = t2 * t;

            QPointF pt = 0.5 * (
                2.0 * p1
                + (-p0 + p2) * t
                + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
                + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
            );
            result << pt;
        }
    }
    return result;
}

// ── Mouse ─────────────────────────────────────────────────────────────────────
int SplineOverlay::nodeAt(const QPointF &vpPos) const
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        QPointF vpNode = sceneToViewport(m_nodes[i]);
        if (QLineF(vpPos, vpNode).length() <= HIT_RADIUS)
            return i;
    }
    return -1;
}

void SplineOverlay::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();
    QPointF vpPos = event->pos();

    if (event->button() == Qt::LeftButton) {
        int hit = nodeAt(vpPos);
        if (hit >= 0) {
            m_draggingIdx = hit;
            m_dragging = true;
        } else {
            // Convert viewport click → scene coords and insert in sorted X order
            QPointF scenePos = viewportToScene(vpPos);
            int insertIdx = m_nodes.size();
            for (int i = 0; i < m_nodes.size(); ++i) {
                if (scenePos.x() < m_nodes[i].x()) {
                    insertIdx = i;
                    break;
                }
            }
            m_nodes.insert(insertIdx, scenePos);
            m_draggingIdx = insertIdx;
            m_dragging = true;
            emit splineChanged(m_nodes);
        }
        update();
    }

    if (event->button() == Qt::RightButton) {
        int hit = nodeAt(vpPos);
        if (hit >= 0) {
            m_nodes.removeAt(hit);
            m_draggingIdx = -1;
            emit splineChanged(m_nodes);
            update();
        }
    }
}

void SplineOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && m_draggingIdx >= 0 && m_draggingIdx < m_nodes.size()) {
        // Store dragged position in scene coords
        m_nodes[m_draggingIdx] = viewportToScene(event->pos());
        update();
        emit splineChanged(m_nodes);
    }
}

void SplineOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
    m_draggingIdx = -1;
}

void SplineOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        commitSpline();
    } else if (event->key() == Qt::Key_Escape) {
        emit exitRequested();
    } else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (!m_nodes.isEmpty()) {
            m_nodes.removeLast();
            update();
            emit splineChanged(m_nodes);
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

// Removed Rest
