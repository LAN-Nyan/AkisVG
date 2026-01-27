#ifndef BLENDTOOL_H
#define BLENDTOOL_H

#include "tool.h"

class BlendTool : public Tool
{
    Q_OBJECT

public:
    explicit BlendTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    void setBlendStrength(qreal strength) { m_blendStrength = strength; }
    qreal blendStrength() const { return m_blendStrength; }

private:
    void blendAtPoint(const QPointF &pos, VectorCanvas *canvas);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal factor);

    qreal m_blendStrength;  // 0.0 to 1.0
    QPointF m_lastPoint;
};

#endif // BLENDTOOL_H
