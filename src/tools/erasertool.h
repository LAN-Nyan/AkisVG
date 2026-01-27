#ifndef ERASERTOOL_H
#define ERASERTOOL_H

#include "tool.h"
#include <QPointF>

class VectorObject;

class EraserTool : public Tool
{
    Q_OBJECT

public:
    explicit EraserTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    void eraseAtPoint(const QPointF &point, VectorCanvas *canvas);
    QPointF m_lastErasePoint;
};

#endif // ERASERTOOL_H
