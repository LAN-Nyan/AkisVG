#include "penciltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>

PencilTool::PencilTool(QObject *parent)
    : Tool(ToolType::Pencil, parent)
    , m_currentPath(nullptr)
{
}

void PencilTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    // CRITICAL: Accept the event so canvas knows we're handling it
    event->setAccepted(true);

    // 1. Safety check: ensure we don't have a path already
    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);

    // 2. Start the path at the mouse position
    QPainterPath path;
    path.moveTo(event->scenePos());
    m_currentPath->setPath(path);

    // 3. Add to canvas (which handles the UndoStack)
    canvas->addObject(m_currentPath);
}

void PencilTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!m_currentPath) return;

    QPainterPath path = m_currentPath->path();
    QPointF newPoint = event->scenePos();
    QPointF lastPoint = path.currentPosition();

    // 4. Distance threshold (prevents jitter and bloated path data)
    if (QLineF(lastPoint, newPoint).length() > 2.0) {
        // Simple linear approach for a "Pencil":
        path.lineTo(newPoint);

        // Update the object and trigger a scene update
        m_currentPath->setPath(path);
        canvas->update();
    }
}

void PencilTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!m_currentPath) return;

    // Finalize the path
    m_currentPath = nullptr;

    Tool::mouseReleaseEvent(event, canvas);
}
