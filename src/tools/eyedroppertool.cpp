#include "eyedroppertool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"

EyedropperTool::EyedropperTool(QObject *parent)
    : Tool(ToolType::Select, parent) // Using Select type or adding a new Eyedropper type
{
    m_name = "Color Picker";
}

void EyedropperTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    // 1. Find the item at the clicked position
    QGraphicsItem *item = canvas->itemAt(event->scenePos(), QTransform());
    VectorObject *vObj = dynamic_cast<VectorObject*>(item);

    if (vObj) {
        // 2. Extract colors from your VectorObject DNA
        QColor pickedStroke = vObj->strokeColor();
        QColor pickedFill = vObj->fillColor();

        // 3. Update the global tool settings (inherited from tool.h)
        // If user holds Shift, maybe pick Fill; otherwise pick Stroke
        if (event->modifiers() & Qt::ShiftModifier) {
            canvas->currentTool()->setFillColor(pickedFill);
        } else {
            canvas->currentTool()->setStrokeColor(pickedStroke);
        }

        emit colorPicked(pickedStroke, pickedFill);
    }
}

void EyedropperTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    // Optional: Update a "Preview" UI element as the user hovers
    QGraphicsItem *item = canvas->itemAt(event->scenePos(), QTransform());
    if (VectorObject *vObj = dynamic_cast<VectorObject*>(item)) {
        // You could emit a signal here for a "live preview" magnifying glass
    }
}
