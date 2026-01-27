#include "pathobject.h"
#include <QPen>

PathObject::PathObject(QGraphicsItem *parent)
    : VectorObject(parent)
{
    // Path is stored in scene coordinates, item position stays at (0,0)
    // This prevents coordinate confusion
}

// The missing piece: Implementation of the clone method
VectorObject* PathObject::clone() const
{
    PathObject* copy = new PathObject();

    // Copy Path-specific data
    copy->setPath(this->m_path);

    // Copy base VectorObject properties
    copy->setStrokeColor(this->m_strokeColor);
    copy->setFillColor(this->m_fillColor);
    copy->setStrokeWidth(this->m_strokeWidth);
    copy->setObjectOpacity(this->m_objectOpacity);

    // Copy QGraphicsItem transformation properties
    copy->setPos(this->pos());
    copy->setRotation(this->rotation());
    copy->setScale(this->scale());
    copy->setZValue(this->zValue());
    copy->setVisible(this->isVisible());

    return copy;
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
    if (m_path.isEmpty()) {
        return QRectF();
    }

    qreal penWidth = m_strokeWidth / 2.0;
    return m_path.boundingRect().adjusted(-penWidth, -penWidth, penWidth, penWidth);
}

void PathObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_path.isEmpty()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(m_objectOpacity); // Use the opacity property

    QPen pen(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);

    if (m_fillColor != Qt::transparent) {
        painter->setBrush(m_fillColor);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    painter->drawPath(m_path);
}
