#include "brushtool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QDateTime>
#include <QtMath>

BrushTool::BrushTool(QObject *parent)
    : Tool(ToolType::Brush, parent)
    , m_currentPath(nullptr)
    , m_lastPressure(0.05)
    , m_lastEventMs(0)
{
    setStrokeWidth(8.0);
    m_pressureSensitive = true;
}

void BrushTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);

    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);
    m_currentPath->setObjectOpacity(m_strokeOpacity);
    m_currentPath->setTexture(static_cast<PathTexture>(static_cast<int>(m_texture)));
    m_currentPath->setSmoothPaths(true);

    m_lastPoint   = event->scenePos();
    m_lastPressure = 0.05;
    m_lastEventMs  = QDateTime::currentMSecsSinceEpoch();

    if (m_pressureSensitive) {
        m_currentPath->addPressurePoint(m_lastPoint, 0.05);
    } else {
        m_currentPath->moveTo(m_lastPoint);
    }

    canvas->addObject(m_currentPath);
    canvas->update();
}

void BrushTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    if (!m_isDrawing || !m_currentPath) return;

    const QPointF currentPoint = event->scenePos();
    const QPointF delta        = currentPoint - m_lastPoint;
    const qreal   distance     = qSqrt(delta.x()*delta.x() + delta.y()*delta.y());

    // Always register the point if it moved at all — smoothing happens in Catmull-Rom.
    // Only skip true duplicates (< 1px) to avoid degenerate zero-length segments.
    if (distance < 1.0) return;

    qreal pressure;

    if (m_pressureSensitive) {
        if (m_currentPressure < 0.99) {
            // ── Real tablet: use hardware pressure with light smoothing ──────
            pressure = m_lastPressure * 0.2 + m_currentPressure * 0.8;
        } else {
            // ── Mouse / no tablet: time-between-points simulation ─────────────
            // Slow, deliberate strokes → short time between events → heavy (wide).
            // Fast, sweeping strokes  → long time between events  → light (thin).
            //
            // Qt sends mouse-move events at ~60–125Hz regardless of speed,
            // but it coalesces them — so a fast stroke produces the same event
            // RATE but each event covers more distance. We measure time since
            // the last *registered* point (after the distance threshold).
            //
            // At typical 60Hz: each event is ~16ms apart.
            // Slow careful stroke: moves <5px in 16ms → might accumulate several
            //   events before we register one, so elapsed ≈ 20–80ms.
            // Fast sweep: covers 30px per event → elapsed ≈ 8–16ms.
            //
            // We map elapsed time [4ms…80ms] → pressure [0.12…0.95].

            const qint64 now        = QDateTime::currentMSecsSinceEpoch();
            const qint64 elapsedMs  = qBound((qint64)1, now - m_lastEventMs, (qint64)200);

            // Normalise: [4ms=fast=thin] to [80ms=slow=heavy]
            // clamp so tiny values don't blow up and large gaps (stroke restart) don't dominate
            const qreal normTime = qBound(0.0, (elapsedMs - 4.0) / 76.0, 1.0);
            qreal rawPressure    = 0.12 + normTime * 0.83;  // → [0.12 … 0.95]

            // Light smoothing so the width doesn't jitter event-to-event
            pressure = m_lastPressure * 0.35 + rawPressure * 0.65;
            pressure = qBound(0.10, pressure, 0.98);

            m_lastEventMs = now;
        }

        m_lastPressure = pressure;
        m_currentPath->addPressurePoint(currentPoint, pressure);
    } else {
        const QPointF mid = (m_lastPoint + currentPoint) / 2.0;
        m_currentPath->quadTo(m_lastPoint, mid);
    }

    m_lastPoint = currentPoint;
}

void BrushTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_isDrawing && m_currentPath) {
        if (m_pressureSensitive) {
            // Taper out smoothly
            const QPointF tip = event->scenePos();
            const QPointF mid = (m_lastPoint + tip) / 2.0;
            m_currentPath->addPressurePoint(mid, m_lastPressure * 0.3);
            m_currentPath->addPressurePoint(tip, 0.05);
        } else {
            m_currentPath->lineTo(event->scenePos());
        }
    }

    Tool::mouseReleaseEvent(event, canvas);
    m_currentPath  = nullptr;
    m_lastPressure = 0.05;
    m_lastEventMs  = 0;
}
