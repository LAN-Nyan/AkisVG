#ifndef BLENDTOOL_H
#define BLENDTOOL_H

#include "tool.h"
#include <QPointF>
#include <QColor>
#include <QPainterPath>
#include <QSet>

class PathObject;
class VectorCanvas;

class BlendTool : public Tool
{
    Q_OBJECT

public:
    explicit BlendTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    void setBlendStrength(qreal strength);
    qreal blendStrength() const { return m_blendStrength; }

private:
    // Single stroke object
    PathObject* m_activeSmear = nullptr;
    QPainterPath m_activeSmearPath;

    QPointF m_lastDrawPos;

    // Paint system
    qreal m_paintAmount = 1.0;
    int m_strokePointCount = 0;  // Track for aggressive simplification

    // Color management
    QColor m_pickedColor;
    bool m_hasPickedColor = false;

    // Stroke tracking
    QPointF m_lastPoint;
    qreal m_blendStrength;

    // Helper methods
    void blendAtPoint(const QPointF &pos, VectorCanvas *canvas);
    QColor pickColorAtPoint(const QPointF &pos, VectorCanvas *canvas);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal factor);

    // Optimized: Simple distance check instead of full path sampling
    qreal distanceToPath(const QPainterPath &path, const QPointF &point) const;
};

#endif // BLENDTOOL_H
