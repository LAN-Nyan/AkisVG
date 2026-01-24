#ifndef VECTOROBJECT_H
#define VECTOROBJECT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QColor>

enum class VectorObjectType {
    Path,
    Rectangle,
    Ellipse,
    Text,
    Image
};

class VectorObject : public QGraphicsItem
{
public:
    explicit VectorObject(QGraphicsItem *parent = nullptr);
    virtual ~VectorObject() = default;
    
    // Type identification
    virtual VectorObjectType objectType() const = 0;
    
    // QGraphicsItem interface
    QRectF boundingRect() const override = 0;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override = 0;
    
    // Common properties
    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor &color);
    
    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &color);
    
    qreal strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(qreal width);
    
    qreal objectOpacity() const { return m_objectOpacity; }
    void setObjectOpacity(qreal opacity);

protected:
    QColor m_strokeColor;
    QColor m_fillColor;
    qreal m_strokeWidth;
    qreal m_objectOpacity;
};

#endif // VECTOROBJECT_H
