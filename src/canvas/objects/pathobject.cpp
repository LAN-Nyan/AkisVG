#include "pathobject.h"
#include <QPen>

PathObject::PathObject(QGraphicsItem *parent)
    : VectorObject(parent)
{
}

void PathObject::setPath(const QPainterPath &path)
{
    prepareGeometryChange();
    m_path = path;
    update();
}

void PathObject::addPoint(const QPointF &point)
{
    prepareGeometryChange();
    if (m_path.elementCount() == 0) {
        m_path.moveTo(point);
    } else {
        m_path.lineTo(point);
    }
    update();
}

void PathObject::lineTo(const QPointF &point)
{
    prepareGeometryChange();
    m_path.lineTo(point);
    update();
}

QRectF PathObject::boundingRect() const
{
    qreal penWidth = m_strokeWidth / 2.0;
    return m_path.boundingRect().adjusted(-penWidth, -penWidth, penWidth, penWidth);
}

void PathObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    QPen pen(m_strokeColor, m_strokeWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    
    if (m_fillColor != Qt::transparent) {
        painter->setBrush(m_fillColor);
    } else {
        painter->setBrush(Qt::NoBrush);
    }
    
    painter->drawPath(m_path);
    
    // Draw selection outline if selected
    if (isSelected()) {
        QPen selectPen(Qt::blue, 1, Qt::DashLine);
        painter->setPen(selectPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}
