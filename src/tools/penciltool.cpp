#include "penciltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"

PencilTool::PencilTool(QObject *parent)
    : Tool(ToolType::Pencil, parent)
    , m_currentPath(nullptr)
{
}

void PencilTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);
    
    // Create new path object
    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);
    m_currentPath->addPoint(event->scenePos());
    
    canvas->addObject(m_currentPath);
}

void PencilTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) {
    Q_UNUSED(canvas); // This fixes the 'unused parameter' warning

    if (!m_currentPath) return; // Use m_currentPath instead of m_currentPathItem

    QPointF newPoint = event->scenePos();
    QPainterPath path = m_currentPath->path();
    QPointF lastPoint = path.currentPosition();

    if (QLineF(lastPoint, newPoint).length() > 3.0) {
        QPointF midPoint = (lastPoint + newPoint) / 2.0;
        path.quadTo(lastPoint, midPoint);

        m_currentPath->setPath(path);
    }
}

void PencilTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    
    Tool::mouseReleaseEvent(event, canvas);
    m_currentPath = nullptr;
}
