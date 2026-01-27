#ifndef BRUSHTOOL_H
#define BRUSHTOOL_H

#include "tool.h"

class PathObject;

class BrushTool : public Tool
{
    Q_OBJECT

public:
    explicit BrushTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    PathObject *m_currentPath;
};

#endif // BRUSHTOOL_H
