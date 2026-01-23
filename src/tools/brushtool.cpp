#include "brushtool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"

BrushTool::BrushTool(QObject *parent)
    : Tool(ToolType::Brush, parent)
    , m_currentPath(nullptr)
{
    setStrokeWidth(8.0); // Brushes are thicker by default
}

void BrushTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);

    // Create new path with brush characteristics
    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);
    m_currentPath->addPoint(event->scenePos());

    canvas->addObject(m_currentPath);
}

void BrushTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)

    if (m_isDrawing && m_currentPath) {
        m_currentPath->lineTo(event->scenePos());
    }
}

void BrushTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)

    Tool::mouseReleaseEvent(event, canvas);
    m_currentPath = nullptr;
}
