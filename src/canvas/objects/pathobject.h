#ifndef PATHOBJECT_H
#define PATHOBJECT_H

#include "vectorobject.h"
#include <QPainterPath>
#include <QVector>
#include <QPointF>
#include <QPen>

enum class PathTexture {
    Smooth,
    Grainy,
    Chalk,
    Canvas
};

enum class PathDashStyle {
    Solid,
    Dashed,
    Dotted
};

struct PressurePoint {
    QPointF pos;
    qreal   pressure;
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

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    void setSmoothPaths(bool smooth) { m_smoothPaths = smooth; }
    bool smoothPaths() const { return m_smoothPaths; }
    void setMinPointDistance(qreal d) { m_minPointDistance = d; }
    qreal minPointDistance() const { return m_minPointDistance; }

    void moveTo(const QPointF &p);
    void quadTo(const QPointF &control, const QPointF &end);
    void moveBy(qreal dx, qreal dy) override;

    void setTexture(PathTexture t) { m_texture = t; }
    PathTexture texture() const { return m_texture; }

    void setDashStyle(PathDashStyle s) { m_dashStyle = s; update(); }
    PathDashStyle dashStyle() const { return m_dashStyle; }

    void setArrowAtEnd(bool a) { m_arrowAtEnd = a; update(); }
    bool arrowAtEnd() const { return m_arrowAtEnd; }

    void addPressurePoint(const QPointF &pos, qreal pressure);
    bool hasPressureData() const { return !m_pressurePoints.isEmpty(); }

    /** When true, pressure strokes render straight segments between anchor samples only (Line-tool style). */
    void setPressureConnectAnchors(bool on) { m_pressureConnectAnchors = on; update(); }
    bool pressureConnectAnchors() const { return m_pressureConnectAnchors; }
    void setPressureConnectionWidthScale(qreal s) { m_pressureConnWidthScale = qBound(0.05, s, 10.0); update(); }
    qreal pressureConnectionWidthScale() const { return m_pressureConnWidthScale; }

private:
    void rebuildSmoothPath();
    void drawArrowHead(QPainter *painter, const QPainterPath &path) const;

    // Core pressure renderer: per-segment drawLine with RoundCap.
    // Simple, correct, seamless — no tangent math, no polygon artifacts.
    void paintPressureStroke(QPainter *painter, qreal baseWidth,
                             qreal minFraction, qreal opacityMul) const;

    QPen buildStrokePen(qreal width, QColor color) const;

    void paintSmooth(QPainter *painter) const;
    void paintGrainy(QPainter *painter) const;
    void paintChalk (QPainter *painter) const;
    void paintCanvas(QPainter *painter) const;

    QPainterPath m_path;
    bool   m_smoothPaths;
    qreal  m_minPointDistance;
    QVector<QPointF>       m_rawPoints;
    QVector<PressurePoint> m_pressurePoints;
    bool   m_pressureConnectAnchors = false;
    qreal  m_pressureConnWidthScale = 1.0;
    PathTexture   m_texture   = PathTexture::Smooth;
    PathDashStyle m_dashStyle = PathDashStyle::Solid;
    bool  m_arrowAtEnd = false;

    mutable QVector<PressurePoint> m_smoothedPressure;
    mutable bool m_smoothedDirty = true;
    void rebuildSmoothedPressure() const;
};

#endif // PATHOBJECT_H
