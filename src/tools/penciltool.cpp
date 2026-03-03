#include "penciltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QtMath>

PencilTool::PencilTool(QObject *parent)
    : Tool(ToolType::Pencil, parent)
    , m_currentPath(nullptr)
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
    // minPointDistance drives how aggressively raw points are filtered.
    // At smoothing=0: keep nearly every point (0.5px minimum).
    // At smoothing=100: keep only widely-spaced points (5px minimum).
    // Default smoothing=50 → 2.5px threshold.
    m_currentPath->setMinPointDistance(qMax(0.5, m_smoothingAmount * 0.05));
    m_currentPath->setTexture(static_cast<PathTexture>(static_cast<int>(m_texture)));

    m_lastPoint = event->scenePos();
    m_currentPath->moveTo(m_lastPoint);

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

    // Only skip true duplicates
    if (dist < 1.0) return;

    // Smooth midpoint quadratic: control = last raw point, end = midpoint.
    // This keeps the rendered curve behind the cursor by half a segment,
    // which is the standard trick for smooth real-time freehand paths.
    const QPointF mid = (m_lastPoint + current) / 2.0;
    m_currentPath->quadTo(m_lastPoint, mid);

    m_lastPoint = current;
}

void PencilTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_isDrawing && m_currentPath)
        m_currentPath->lineTo(event->scenePos());

    m_currentPath = nullptr;
    Tool::mouseReleaseEvent(event, canvas);
}
