#include "selecttool.h"
#include "canvas/vectorcanvas.h"

SelectTool::SelectTool(QObject *parent)
    : Tool(ToolType::Select, parent)
{
}

void SelectTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    // Let the scene handle selection
    event->setAccepted(false);
}

void SelectTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    event->setAccepted(false);
}

void SelectTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    event->setAccepted(false);
}
