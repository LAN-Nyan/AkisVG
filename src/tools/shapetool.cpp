#include "shapetool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/shapeobject.h"

#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>

ShapeTool::ShapeTool(ShapeType shapeType, QObject *parent)
    : Tool(shapeType == ShapeType::Rectangle ? ToolType::Rectangle : ToolType::Ellipse, parent)
    , m_shapeType(shapeType)
    , m_previewItem(nullptr)
{
}

void ShapeTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);
    m_startPoint = event->scenePos();

    // Create preview shape
    QPen pen(m_strokeColor, m_strokeWidth);
    pen.setStyle(Qt::DashLine);
    QBrush brush(m_fillColor);

    if (m_shapeType == ShapeType::Rectangle) {
        QGraphicsRectItem *rect = new QGraphicsRectItem();
        rect->setPen(pen);
        rect->setBrush(brush);
        rect->setOpacity(0.5);
        canvas->addItem(rect);
        m_previewItem = rect;
    } else {
        QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem();
        ellipse->setPen(pen);
        ellipse->setBrush(brush);
        ellipse->setOpacity(0.5);
        canvas->addItem(ellipse);
        m_previewItem = ellipse;
    }
}

void ShapeTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)

    if (m_isDrawing && m_previewItem) {
        QPointF currentPoint = event->scenePos();
        QRectF rect(m_startPoint, currentPoint);
        rect = rect.normalized();

        if (m_shapeType == ShapeType::Rectangle) {
            QGraphicsRectItem *rectItem = static_cast<QGraphicsRectItem*>(m_previewItem);
            rectItem->setRect(rect);
        } else {
            QGraphicsEllipseItem *ellipseItem = static_cast<QGraphicsEllipseItem*>(m_previewItem);
            ellipseItem->setRect(rect);
        }
    }
}

void ShapeTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_previewItem) {
        // Remove preview
        canvas->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;

        // Create actual shape object
        QPointF currentPoint = event->scenePos();
        QRectF rect(m_startPoint, currentPoint);
        rect = rect.normalized();

        // Only create if shape has some size
        if (rect.width() > 2 && rect.height() > 2) {
            ShapeObject *shape = new ShapeObject(m_shapeType == ShapeType::Rectangle ?
                                                     ShapeObject::Rectangle : ShapeObject::Ellipse);
            shape->setRect(rect);
            shape->setStrokeColor(m_strokeColor);
            shape->setFillColor(m_fillColor);
            shape->setStrokeWidth(m_strokeWidth);

            canvas->addObject(shape);
        }
    }

    Tool::mouseReleaseEvent(event, canvas);
}

