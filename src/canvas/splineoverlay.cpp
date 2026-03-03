#include "splineoverlay.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainterPath>
#include <QtMath>

SplineOverlay::SplineOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);

    // CRITICAL: This allows the widget to grab the keyboard
    setFocusPolicy(Qt::StrongFocus);

    setAttribute(Qt::WA_NoSystemBackground, true);
    // Optional: make it slightly more obvious it's active
    setStyleSheet("background: transparent;");
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

    // Semi-transparent dark overlay to indicate interpolation mode
    p.fillRect(rect(), QColor(0, 0, 0, 60));

    // Header banner
    QRect banner(0, 0, width(), 36);
    p.fillRect(banner, QColor(28, 28, 30, 230));  // Dark charcoal for interpolation banner
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(14);
    p.setFont(f);
    p.drawText(banner.adjusted(12, 0, -12, 0),
               Qt::AlignVCenter | Qt::AlignLeft,
               "✦  INTERPOLATING   —  Click canvas to add nodes  |  Drag to move  |  Del to remove  |  Enter to commit  |  Esc to cancel");

    if (m_nodes.isEmpty()) return;

    // Draw spline curve
    drawSpline(p);

    // Draw nodes
    for (int i = 0; i < m_nodes.size(); ++i) {
        const QPointF &pt = m_nodes[i];

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

void SplineOverlay::drawSpline(QPainter &p) const
{
    if (m_nodes.size() < 2) {
        // Single node — draw a dot
        p.setPen(QPen(QColor(28, 28, 30, 230), 2, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        return;
    }

    // Draw glow / shadow
    QPainterPath path;
    QList<QPointF> pts = catmullRomPoints(30);
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

    // Dashed extension lines from node to spline tangents
    p.setPen(QPen(QColor(255, 255, 255, 40), 1, Qt::DashLine));
    for (int i = 1; i < m_nodes.size(); ++i) {
        p.drawLine(m_nodes[i - 1], m_nodes[i]);
    }
}

/**
 * Catmull-Rom interpolation through the control points.
 * Returns `segments` line segments between each pair of nodes.
 */
QList<QPointF> SplineOverlay::catmullRomPoints(int segments) const
{
    QList<QPointF> result;
    if (m_nodes.size() < 2) return result;

    // Pad ends so Catmull-Rom works at endpoints
    QList<QPointF> pts;
    pts << m_nodes.first();   // phantom start
    pts << m_nodes;
    pts << m_nodes.last();    // phantom end

    for (int i = 1; i < pts.size() - 2; ++i) {
        QPointF p0 = pts[i - 1];
        QPointF p1 = pts[i];
        QPointF p2 = pts[i + 1];
        QPointF p3 = pts[i + 2];

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
int SplineOverlay::nodeAt(const QPointF &pos) const
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (QLineF(pos, m_nodes[i]).length() <= HIT_RADIUS)
            return i;
    }
    return -1;
}

void SplineOverlay::mousePressEvent(QMouseEvent *event)
{
  this->setFocus();
    QPointF pos = event->pos();

    if (event->button() == Qt::LeftButton) {
        int hit = nodeAt(pos);
        if (hit >= 0) {
            m_draggingIdx = hit;
            m_dragging = true;
        } else {
            // Add new node in sorted order (by X position)
            int insertIdx = m_nodes.size();
            for (int i = 0; i < m_nodes.size(); ++i) {
                if (pos.x() < m_nodes[i].x()) {
                    insertIdx = i;
                    break;
                }
            }
            m_nodes.insert(insertIdx, pos);
            m_draggingIdx = insertIdx;
            m_dragging = true;
            emit splineChanged(m_nodes);
        }
        update();
    }

    if (event->button() == Qt::RightButton) {
        // Right-click removes nearest node
        int hit = nodeAt(pos);
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
        m_nodes[m_draggingIdx] = event->pos();
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
        // Delete last node
        if (!m_nodes.isEmpty()) {
            m_nodes.removeLast();
            update();
            emit splineChanged(m_nodes);
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}
