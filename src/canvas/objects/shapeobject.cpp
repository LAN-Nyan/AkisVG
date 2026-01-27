#include "shapeobject.h"
#include <QPainter>
#include <QPen>

ShapeObject::ShapeObject(Type type, QGraphicsItem *parent)
    : VectorObject(parent)
    , m_shapeType(type)
{
}

VectorObject* ShapeObject::clone() const {
    // Assuming your constructor takes the shape type
    ShapeObject* copy = new ShapeObject(this->m_shapeType);

    // Copy Shape-specific geometry (e.g., the rectangle bounds)
    copy->setRect(this->m_rect);

    // Copy base VectorObject properties
    copy->setStrokeColor(this->strokeColor());
    copy->setFillColor(this->fillColor());
    copy->setStrokeWidth(this->strokeWidth());
    copy->setObjectOpacity(this->objectOpacity());

    copy->setPos(this->pos());
    copy->setRotation(this->rotation());
    copy->setScale(this->scale());

    return copy;
}

void ShapeObject::setRect(const QRectF &rect)
{
    prepareGeometryChange();
    m_rect = rect;
    setPos(rect.topLeft());
    m_rect.moveTopLeft(QPointF(0, 0));
    update();
}

QRectF ShapeObject::boundingRect() const
{
    qreal penWidth = m_strokeWidth / 2.0;
    return m_rect.adjusted(-penWidth, -penWidth, penWidth, penWidth);
}

void ShapeObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen(m_strokeColor, m_strokeWidth);
    painter->setPen(pen);

    if (m_fillColor != Qt::transparent && m_fillColor.alpha() > 0) {
        painter->setBrush(m_fillColor);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    if (m_shapeType == Rectangle) {
        painter->drawRect(m_rect);
    } else {
        painter->drawEllipse(m_rect);
    }

    // Draw selection outline if selected
    if (isSelected()) {
        QPen selectPen(Qt::blue, 1, Qt::DashLine);
        painter->setPen(selectPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}
