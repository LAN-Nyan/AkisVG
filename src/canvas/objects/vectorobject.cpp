#include "vectorobject.h"

VectorObject::VectorObject(QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_strokeColor(Qt::black)
    , m_fillColor(Qt::transparent)
    , m_strokeWidth(2.0)
    , m_objectOpacity(1.0)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

void VectorObject::setStrokeColor(const QColor &color)
{
    m_strokeColor = color;
    update();
}

void VectorObject::setFillColor(const QColor &color)
{
    m_fillColor = color;
    update();
}

void VectorObject::setStrokeWidth(qreal width)
{
    m_strokeWidth = qMax(0.0, width);
    update();
}

void VectorObject::setObjectOpacity(qreal opacity)
{
    m_objectOpacity = qBound(0.0, opacity, 1.0);
    setOpacity(m_objectOpacity);
}
