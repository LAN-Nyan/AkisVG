#include "shapeobject.h"
#include <QPainter>
#include <QPen>

ShapeObject::ShapeObject(Type type, QGraphicsItem *parent)
    : VectorObject(parent)
    , m_shapeType(type)
{
}

VectorObject* ShapeObject::clone() const
{
    ShapeObject *copy = new ShapeObject(m_shapeType);
    copy->setRect(m_rect);
    copy->setStrokeColor(strokeColor());
    copy->setFillColor(fillColor());
    copy->setStrokeWidth(strokeWidth());
    copy->setObjectOpacity(objectOpacity());
    copy->setRoundedCorners(m_roundedCorners);
    copy->setCornerRadius(m_cornerRadius);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setScale(scale());
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
    qreal pw = qMax(m_strokeWidth, 1.0) / 2.0;
    return m_rect.adjusted(-pw, -pw, pw, pw);
}

void ShapeObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);

    // Stroke
    if (m_strokeWidth > 0) {
        painter->setPen(QPen(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    } else {
        painter->setPen(Qt::NoPen);
    }

    // Fill
    painter->setBrush((m_fillColor.alpha() > 0) ? QBrush(m_fillColor) : Qt::NoBrush);

    if (m_shapeType == Rectangle) {
        if (m_roundedCorners && m_cornerRadius > 0)
            painter->drawRoundedRect(m_rect, m_cornerRadius, m_cornerRadius);
        else
            painter->drawRect(m_rect);
    } else {
        painter->drawEllipse(m_rect);
    }

    // Selection highlight
    if (isSelected()) {
        painter->setPen(QPen(QColor(42, 130, 218), 1.5, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
    }
}
