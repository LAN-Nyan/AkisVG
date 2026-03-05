#ifndef FILLTOOL_H
#define FILLTOOL_H

#include "tool.h"

class FillTool : public Tool
{
public:
    explicit FillTool(QObject *parent = nullptr);

    // Tool interface implementations
    ToolType toolType() const override { return ToolType::Fill; }
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
};

#endif // FILLTOOL_H
