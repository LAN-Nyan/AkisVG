#include "erasertool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include "core/project.h"
#include "core/layer.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>

EraserTool::EraserTool(QObject *parent)
    : Tool(ToolType::Eraser, parent)
{
    setStrokeWidth(20.0); // Erasers are large
}

void EraserTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);
    m_lastErasePoint = event->scenePos();
    eraseAtPoint(event->scenePos(), canvas);
}

void EraserTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_isDrawing) {
        eraseAtPoint(event->scenePos(), canvas);
        m_lastErasePoint = event->scenePos();
    }
}

void EraserTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    Tool::mouseReleaseEvent(event, canvas);
}

void EraserTool::eraseAtPoint(const QPointF &point, VectorCanvas *canvas)
{
    // Get items under the eraser
    QRectF eraseRect(point.x() - m_strokeWidth/2,
                     point.y() - m_strokeWidth/2,
                     m_strokeWidth, m_strokeWidth);

    QList<QGraphicsItem*> items = canvas->items(eraseRect);

    for (QGraphicsItem *item : items) {
        VectorObject *obj = dynamic_cast<VectorObject*>(item);
        if (obj) {
            // Check if the object intersects with eraser area
            if (obj->boundingRect().intersects(eraseRect)) {
                canvas->removeObject(obj);
            }
        }
    }
}
