#ifndef TEXTTOOL_H
#define TEXTTOOL_H

#include "tool.h"

class TextTool : public Tool
{
    Q_OBJECT

public:
    explicit TextTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
};

#endif // TEXTTOOL_H
