#ifndef PATHOBJECT_H
#define PATHOBJECT_H

#include "vectorobject.h"
#include <QPainterPath>

class PathObject : public VectorObject
{
public:
    explicit PathObject(QGraphicsItem *parent = nullptr);
    virtual VectorObject* clone() const override;

    VectorObjectType objectType() const override { return VectorObjectType::Path; }

    QPainterPath path() const { return m_path; }
    void setPath(const QPainterPath &path);
    void addPoint(const QPointF &point);
    void lineTo(const QPointF &point);

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    QPainterPath m_path;
};

#endif // PATHOBJECT_H
