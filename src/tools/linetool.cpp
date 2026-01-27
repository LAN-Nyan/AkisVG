#include "linetool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>

LineTool::LineTool(QObject *parent)
    : Tool(ToolType::Line, parent)
    , m_currentLine(nullptr)
{
}

void LineTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (event->button() != Qt::LeftButton) {
        event->setAccepted(false);
        return;
    }

    event->setAccepted(true);
    m_startPoint = event->scenePos();
    
    // Create a new path object for the line
    m_currentLine = new PathObject();
    m_currentLine->setStrokeColor(m_strokeColor);
    m_currentLine->setStrokeWidth(m_strokeWidth);
    
    // Start the path
    QPainterPath path;
    path.moveTo(m_startPoint);
    m_currentLine->setPath(path);
    
    // Add to canvas immediately so we can see it while dragging
    canvas->addItem(m_currentLine);
}

void LineTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas);
    
    if (!m_currentLine) return;
    
    // Update the line endpoint as the mouse moves
    QPointF currentPos = event->scenePos();
    
    QPainterPath path;
    path.moveTo(m_startPoint);
    path.lineTo(currentPos);
    
    m_currentLine->setPath(path);
}

void LineTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event);
    
    if (!m_currentLine) {
        Tool::mouseReleaseEvent(event, canvas);
        return;
    }
    
    // Remove from scene (it will be re-added via the command)
    canvas->removeItem(m_currentLine);
    
    // Add to layer via undo command
    canvas->addObject(m_currentLine);
    
    m_currentLine = nullptr;
    
    Tool::mouseReleaseEvent(event, canvas);
}
