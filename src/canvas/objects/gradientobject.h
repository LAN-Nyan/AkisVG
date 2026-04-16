#pragma once
#include "vectorobject.h"
#include <QGradient>
#include <QPointF>
#include <QSizeF>

// GradientObject: renders a linear or radial gradient fill over a rectangular
// area on the canvas. The gradient is defined by two handles (start/end) that
// the user drags interactively while the GradientTool is active.
class GradientObject : public VectorObject
{
public:
    enum GradientType { Linear, Radial };

    explicit GradientObject(QGraphicsItem *parent = nullptr);
    VectorObject* clone() const override;
    VectorObjectType objectType() const override { return VectorObjectType::Path; } // reuse Path slot

    // Gradient definition
    void setGradientType(GradientType t) { m_gradType = t; update(); }
    GradientType gradientType() const    { return m_gradType; }

    void setStartPoint(const QPointF &p) { m_start = p; updateBounds(); }
    void setEndPoint(const QPointF &p)   { m_end   = p; updateBounds(); }
    QPointF startPoint() const { return m_start; }
    QPointF endPoint()   const { return m_end;   }

    void setStartColor(const QColor &c) { m_startColor = c; update(); }
    void setEndColor(const QColor &c)   { m_endColor   = c; update(); }
    QColor startColor() const { return m_startColor; }
    QColor endColor()   const { return m_endColor;   }

    void setRepeat(bool r) { m_repeat = r; update(); }
    bool repeat() const    { return m_repeat; }

    // QGraphicsItem
    QRectF boundingRect() const override;
    void   paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *widget = nullptr) override;

private:
    void updateBounds();

    GradientType m_gradType   = Linear;
    QPointF      m_start      = QPointF(0, 0);
    QPointF      m_end        = QPointF(200, 0);
    QColor       m_startColor = Qt::white;
    QColor       m_endColor   = Qt::transparent;
    bool         m_repeat     = false;
    QRectF       m_bounds;   // cache: bounding rect encompassing both handles
};
