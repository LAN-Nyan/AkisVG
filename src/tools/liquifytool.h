#ifndef LIQUIFYTOOL_H
#define LIQUIFYTOOL_H

#include "tool.h"
#include <QPointF>
#include <QSet>

class PathObject;

class LiquifyTool : public Tool
{
public:
    explicit LiquifyTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    QPointF m_lastPoint;
    void warpAtPoint(const QPointF &pos, VectorCanvas *canvas);
    qreal m_influenceRadius = 50.0;
    qreal m_pushStrength = 0.5;
    QSet<PathObject*> m_affectedObjects;
};

#endif
