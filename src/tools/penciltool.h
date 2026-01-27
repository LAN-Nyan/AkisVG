#ifndef PENCILTOOL_H
#define PENCILTOOL_H

#include "tool.h"

class PathObject;

class PencilTool : public Tool
{
    Q_OBJECT

public:
    explicit PencilTool(QObject *parent = nullptr);
    
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    PathObject *m_currentPath;
};

#endif // PENCILTOOL_H

