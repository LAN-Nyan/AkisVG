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
    
    // Fill by default setting
    void setFillByDefault(bool fill) { m_fillByDefault = fill; }
    bool fillByDefault() const { return m_fillByDefault; }

private:
    ShapeType m_shapeType;
    QPointF m_startPoint;
    QGraphicsItem *m_previewItem;
    bool m_fillByDefault;
};

#endif // SHAPETOOL_H
