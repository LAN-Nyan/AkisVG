#ifndef BRUSHTOOL_H
#define BRUSHTOOL_H

#include "tool.h"
#include <QDateTime>

class PathObject;

class BrushTool : public Tool
{
    Q_OBJECT

public:
    explicit BrushTool(QObject *parent = nullptr);

    ToolType toolType() const override { return ToolType::Brush; }

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void cancelDraw() override { m_isDrawing = false; m_currentPath = nullptr; }

private:
    PathObject *m_currentPath;
    QPointF     m_lastPoint;
    qreal       m_lastPressure;  // smoothed pressure from previous sample
    qint64      m_lastEventMs;   // timestamp of last registered point (for time-based sim)
};

#endif // BRUSHTOOL_H
