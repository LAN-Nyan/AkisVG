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

    // Rounded corners (rectangles only)
    void setRoundedCorners(bool rounded) { m_roundedCorners = rounded; update(); }
    bool roundedCorners() const { return m_roundedCorners; }
    void setCornerRadius(qreal radius) { m_cornerRadius = radius; update(); }
    qreal cornerRadius() const { return m_cornerRadius; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    Type m_shapeType;
    QRectF m_rect;
    bool m_roundedCorners = false;
    qreal m_cornerRadius  = 12.0;
};

#endif // SHAPEOBJECT_H
