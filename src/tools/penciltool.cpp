#include "filltool.h"
#include "canvas/vectorcanvas.h"
#include <QMouseEvent>
#include <QStack>
#include <QGraphicsSceneMouseEvent>

FillTool::FillTool(QObject *parent)
    : Tool(ToolType::Fill, parent) // Pass ToolType::Fill here
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

    QImage image = canvas->currentImage();
    qreal dpr = image.devicePixelRatio();

    // Convert scene coordinate to image coordinate
    QPointF scenePos = event->scenePos() - canvas->sceneRect().topLeft();
    QPoint startPoint = (scenePos * dpr).toPoint();

    if (!image.rect().contains(startPoint)) return;

    // Perform flood fill
    floodFill(image, startPoint, m_fillColor);

    // Convert the filled image back to a path/object
    // For now, we'll create a rectangle with the fill color
    // A better approach would be to convert the filled region to a vector shape
    canvas->updateCurrentImage(image);
    canvas->update();

    // TODO: Convert filled raster image to vector objects
    // For a proper implementation, you'd want to trace the filled region
    // and create vector shapes from it
}

void FillTool::floodFill(QImage &image, const QPoint &start, const QColor &fillColor)
{
    QColor targetColor = image.pixelColor(start);

    if (targetColor == fillColor) return;
    if (start.x() < 0 || start.y() < 0 || start.x() >= image.width() || start.y() >= image.height()) return;

    QStack<QPoint> stack;
    stack.push(start);

    while (!stack.isEmpty()) {
        QPoint p = stack.pop();
        int x = p.x();
        int y = p.y();

        if (x < 0 || y < 0 || x >= image.width() || y >= image.height()) continue;
        if (image.pixelColor(x, y) != targetColor) continue;

        image.setPixelColor(x, y, fillColor);

        stack.push(QPoint(x + 1, y));
        stack.push(QPoint(x - 1, y));
        stack.push(QPoint(x, y + 1));
        stack.push(QPoint(x, y - 1));
    }
}
