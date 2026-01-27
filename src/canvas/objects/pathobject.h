#ifndef PATHOBJECT_H
#define PATHOBJECT_H

#include "vectorobject.h"
#include <QPainterPath>
#include <QVector>
#include <QPointF>

enum class PathTexture {
    Smooth,
    Grainy,
    Chalk,
    Canvas
};

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

    // Smooth path settings
    void setSmoothPaths(bool smooth) { m_smoothPaths = smooth; }
    bool smoothPaths() const { return m_smoothPaths; }
    void setMinPointDistance(qreal distance) { m_minPointDistance = distance; }
    qreal minPointDistance() const { return m_minPointDistance; }
    
    // Texture support
    void setTexture(PathTexture texture) { m_texture = texture; }
    PathTexture texture() const { return m_texture; }

private:
    void rebuildSmoothPath(); // Rebuild the smooth path from raw points

    QPainterPath m_path;
    bool m_smoothPaths;
    qreal m_minPointDistance;
    QVector<QPointF> m_rawPoints; // Store the actual input points
    PathTexture m_texture;
};

#endif // PATHOBJECT_H
