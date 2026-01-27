#include "filltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include "canvas/objects/pathobject.h"
#include "canvas/objects/shapeobject.h"
#include "core/commands.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QDebug>

FillTool::FillTool(QObject *parent)
    : Tool(ToolType::Fill, parent)
{
}

void FillTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (event->button() != Qt::LeftButton) {
        event->setAccepted(false);
        return;
    }

    // Accept the event so canvas knows we handled it
    event->setAccepted(true);

    QPointF clickPos = event->scenePos();
    
    // Find all items at this position (in reverse order to get topmost first)
    QList<QGraphicsItem*> itemsAtPos = canvas->items(clickPos, Qt::IntersectsItemShape, Qt::DescendingOrder);
    
    // Look for vector objects we can fill
    VectorObject* targetObject = nullptr;
    for (QGraphicsItem* item : itemsAtPos) {
        VectorObject* vecObj = dynamic_cast<VectorObject*>(item);
        if (vecObj) {
            // Convert click position to item's local coordinates
            QPointF localPos = vecObj->mapFromScene(clickPos);
            
            // Check different object types
            PathObject* pathObj = dynamic_cast<PathObject*>(vecObj);
            ShapeObject* shapeObj = dynamic_cast<ShapeObject*>(vecObj);
            
            bool insideShape = false;
            
            if (pathObj) {
                // For paths, check if inside the path
                QPainterPath path = pathObj->path();
                insideShape = path.contains(localPos);
            } else if (shapeObj) {
                // For shapes, use the shape's own contains method
                insideShape = shapeObj->contains(localPos);
            } else {
                // For other objects, use default contains
                insideShape = vecObj->contains(localPos);
            }
            
            if (insideShape) {
                targetObject = vecObj;
                break;
            }
        }
    }
    
    if (targetObject) {
        // Use undo command to fill the shape (supports undo/redo)
        FillColorCommand *cmd = new FillColorCommand(targetObject, m_fillColor);
        canvas->undoStack()->push(cmd);
        
        canvas->update();
        qDebug() << "Filled object with color:" << m_fillColor;
    } else {
        qDebug() << "No object found at click position";
    }
}

void FillTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    // Fill tool doesn't need move handling
    Q_UNUSED(event);
    Q_UNUSED(canvas);
}

void FillTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mouseReleaseEvent(event, canvas);
}
