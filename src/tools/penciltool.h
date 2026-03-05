#ifndef PENCILTOOL_H
#define PENCILTOOL_H

#include "tool.h"

class PathObject;

class PencilTool : public Tool
{
    Q_OBJECT

public:
    explicit PencilTool(QObject *parent = nullptr);
    ToolType toolType() const override { return ToolType::Pencil; }
    void mousePressEvent  (QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent   (QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    PathObject *m_currentPath  = nullptr;
    QPointF     m_lastPoint;

    // FIX #27: Pressure-sensitive drawing state (mirrors BrushTool pattern)
    qreal       m_lastPressure = 0.5;
    qint64      m_lastEventMs  = 0;
};

#endif // PENCILTOOL_H
