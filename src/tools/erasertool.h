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
    ToolType toolType() const override { return ToolType::Eraser; }
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void cancelDraw() override { m_isDrawing = false; }

private:
    void eraseAtPoint(const QPointF &point, VectorCanvas *canvas);
    QPointF m_lastErasePoint;
};

#endif // ERASERTOOL_H
