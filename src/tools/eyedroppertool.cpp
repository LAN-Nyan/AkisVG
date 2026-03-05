#include "eyedroppertool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QImage>
#include <QGraphicsView>

EyedropperTool::EyedropperTool(QObject *parent)
    : Tool(ToolType::eyedropper, parent) // Add 'Eyedropper' to your ToolType enum
{
    m_name = "Color Picker";
}

// FIX #16: Sample a 1px region of the screen at the given scene position.
// Falls back to VectorObject stroke/fill if screen grab fails.
static QColor screenColorAt(const QPointF &scenePos, VectorCanvas *canvas)
{
    if (!canvas || canvas->views().isEmpty())
        return QColor();

    QGraphicsView *view = canvas->views().first();
    // Convert scene → viewport → global screen coords
    QPoint vpPt   = view->mapFromScene(scenePos);
    QPoint globalPt = view->viewport()->mapToGlobal(vpPt);

    // Grab a 1×1 pixel from the screen at that position
    QScreen *screen = QGuiApplication::screenAt(globalPt);
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return QColor();

    QPixmap px = screen->grabWindow(0, globalPt.x(), globalPt.y(), 1, 1);
    if (px.isNull()) return QColor();

    return px.toImage().pixelColor(0, 0);
}

void EyedropperTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    // FIX #16: Try screen-pixel sampling first so the tool works on images
    // and any complex/raster content, not just VectorObject stroke/fill.
    QColor screenColor = screenColorAt(event->scenePos(), canvas);

    if (screenColor.isValid()) {
        emit colorPicked(screenColor, screenColor);
        event->accept();
        return;
    }

    // Fallback: extract from VectorObject (stroke/fill metadata)
    QGraphicsItem *item = canvas->itemAt(event->scenePos(), QTransform());
    VectorObject *vObj = dynamic_cast<VectorObject*>(item);

    if (vObj) {
        QColor pickedStroke = vObj->strokeColor();
        QColor pickedFill = vObj->fillColor();

        if (event->modifiers() & Qt::ShiftModifier) {
            canvas->currentTool()->setFillColor(pickedFill);
        } else {
            canvas->currentTool()->setStrokeColor(pickedStroke);
        }

        emit colorPicked(pickedStroke, pickedFill);
        event->accept();
    } else {
        event->ignore();
    }
}

void EyedropperTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    QGraphicsItem *item = canvas->itemAt(event->scenePos(), QTransform());

    // Check if we are hovering over a valid object
    if (dynamic_cast<VectorObject*>(item)) {
        event->accept();
    }
}
