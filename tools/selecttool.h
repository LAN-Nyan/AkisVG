#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "tool.h"

class SelectTool : public Tool
{
    Q_OBJECT

public:
    explicit SelectTool(QObject *parent = nullptr);
    
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
};

#endif // SELECTTOOL_H
