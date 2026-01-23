#ifndef SHAPETOOL_H
#define SHAPETOOL_H

#include "tool.h"
#include <QPointF>
#include <QGraphicsItem>

enum class ShapeType {
    Rectangle,
    Ellipse
};

class ShapeTool : public Tool
{
    Q_OBJECT

public:
    explicit ShapeTool(ShapeType shapeType, QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    ShapeType m_shapeType;
    QPointF m_startPoint;
    QGraphicsItem *m_previewItem;
};

#endif // SHAPETOOL_H
