#include "penciltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QDateTime>
#include <QtMath>

PencilTool::PencilTool(QObject *parent)
    : Tool(ToolType::Pencil, parent)
    , m_currentPath(nullptr)
    , m_lastPressure(0.5)
    , m_lastEventMs(0)
{
    setStrokeWidth(2.0);
    m_pressureSensitive = false;
}

void PencilTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    event->setAccepted(true);
    m_isDrawing = true;

    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);
    m_currentPath->setObjectOpacity(m_strokeOpacity);
    m_currentPath->setSmoothPaths(true);
    m_currentPath->setMinPointDistance(qMax(0.5, m_smoothingAmount * 0.05));
    m_currentPath->setTexture(static_cast<PathTexture>(static_cast<int>(m_texture)));

    m_lastPoint    = event->scenePos();
    m_lastPressure = m_pressureSensitive ? m_currentPressure : 0.5;
    m_lastEventMs  = QDateTime::currentMSecsSinceEpoch();

    // FIX #27: Use pressure data path when pressure sensitivity is on.
    // This stores reproducible scaleable pressure points — the stroke is
    // re-renderable at any zoom because it's still vector geometry, just
    // with per-segment width driven by the recorded pressure values.
    if (m_pressureSensitive) {
        m_currentPath->setPressureConnectAnchors(m_pressureConnectAnchors);
        m_currentPath->setPressureConnectionWidthScale(m_anchorConnectWidthScale);
        m_currentPath->addPressurePoint(m_lastPoint, 0.05); // taper in
    } else {
        m_currentPath->moveTo(m_lastPoint);
    }

    canvas->addObject(m_currentPath);
    canvas->update();
}

void PencilTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    if (!m_isDrawing || !m_currentPath) return;

    const QPointF current = event->scenePos();
    const QPointF delta   = current - m_lastPoint;
    const qreal   dist    = qSqrt(delta.x()*delta.x() + delta.y()*delta.y());

    const qreal minStep = m_pressureSensitive ? m_anchorMinDistance : 1.0;
    if (dist < minStep) return;

    if (m_pressureSensitive) {
        // FIX #27: Record pressure at every registered point.
        // When a real tablet is connected, m_currentPressure reflects hardware
        // pressure. With a mouse, we simulate it from drawing speed (same
        // technique as BrushTool) so the feature works without a tablet.
        qreal pressure;
        if (m_currentPressure < 0.99) {
            // Real tablet hardware pressure
            pressure = m_lastPressure * 0.3 + m_currentPressure * 0.7;
        } else {
            // Mouse: simulate from time-between-points (slow = heavy)
            const qint64 now       = QDateTime::currentMSecsSinceEpoch();
            const qint64 elapsedMs = qBound((qint64)1, now - m_lastEventMs, (qint64)200);
            const qreal  normTime  = qBound(0.0, (elapsedMs - 4.0) / 76.0, 1.0);
            const qreal  raw       = 0.15 + normTime * 0.70; // [0.15..0.85]
            pressure = m_lastPressure * 0.4 + raw * 0.6;
            pressure = qBound(0.10, pressure, 0.95);
            m_lastEventMs = now;
        }
        m_lastPressure = pressure;
        m_currentPath->addPressurePoint(current, pressure);
    } else {
        // Standard smooth pencil stroke (Catmull-Rom midpoint)
        const QPointF mid = (m_lastPoint + current) / 2.0;
        m_currentPath->quadTo(m_lastPoint, mid);
    }

    m_lastPoint = current;
}

void PencilTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_isDrawing && m_currentPath) {
        if (m_pressureSensitive) {
            // FIX #27: Taper out on release
            const QPointF tip = event->scenePos();
            const QPointF mid = (m_lastPoint + tip) / 2.0;
            m_currentPath->addPressurePoint(mid, m_lastPressure * 0.3);
            m_currentPath->addPressurePoint(tip, 0.05);
        } else {
            m_currentPath->lineTo(event->scenePos());
        }
    }

    m_currentPath  = nullptr;
    m_lastPressure = 0.5;
    m_lastEventMs  = 0;
    Tool::mouseReleaseEvent(event, canvas);
}
