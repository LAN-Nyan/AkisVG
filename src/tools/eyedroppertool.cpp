#include "eyedroppertool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include <QGraphicsSceneMouseEvent>

EyedropperTool::EyedropperTool(QObject *parent)
    : Tool(ToolType::eyedropper, parent) // Add 'Eyedropper' to your ToolType enum
{
    m_name = "Color Picker";
}

void EyedropperTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    // 1. Find the item at the clicked position
    QGraphicsItem *item = canvas->itemAt(event->scenePos(), QTransform());
    VectorObject *vObj = dynamic_cast<VectorObject*>(item);

    if (vObj) {
        // 2. Extract colors
        QColor pickedStroke = vObj->strokeColor();
        QColor pickedFill = vObj->fillColor();

        // 3. Update settings based on modifiers
        if (event->modifiers() & Qt::ShiftModifier) {
            canvas->currentTool()->setFillColor(pickedFill);
        } else {
            canvas->currentTool()->setStrokeColor(pickedStroke);
        }

        emit colorPicked(pickedStroke, pickedFill);

        // 4. CRITICAL: Tell Qt you handled the event so it doesn't
        // start dragging the ItemIsMovable object
        event->accept();
    } else {
        // If we hit empty space, ignore so the canvas can deselect items
        event->ignore();
    }
}

void EyedropperTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    QGraphicsItem *item = canvas->itemAt(event->scenePos(), QTransform());

    // Check if we are hovering over a valid object
    if (dynamic_cast<VectorObject*>(item)) {
        // You can update a cursor or a small preview UI here later
        event->accept();
    }
}
