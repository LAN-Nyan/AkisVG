#ifndef SHAPEOBJECT_H
#define SHAPEOBJECT_H

#include "vectorobject.h"
#include <QRectF>

class ShapeObject : public VectorObject
{
public:
    enum Type {
        Rectangle,
        Ellipse
    };
    virtual VectorObject* clone() const override;
    explicit ShapeObject(Type type, QGraphicsItem *parent = nullptr);

    VectorObjectType objectType() const override {
        return m_shapeType == Rectangle ? VectorObjectType::Rectangle : VectorObjectType::Ellipse;
    }

    void setRect(const QRectF &rect);
    QRectF rect() const { return m_rect; }

    Type shapeType() const { return m_shapeType; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    Type m_shapeType;
    QRectF m_rect;
};

#endif // SHAPEOBJECT_H
