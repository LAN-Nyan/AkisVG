#ifndef SHAPETOOL_H
#define SHAPETOOL_H

#include "tool.h"
#include <QPointF>
#include <QGraphicsItem>
#include <QColor>

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

    // FIX #18: Separate fill color from stroke color
    void setShapeFillColor(const QColor &color) { m_shapeFillColor = color; }
    QColor shapeFillColor() const { return m_shapeFillColor; }

    void setRoundedCorners(bool r) { m_roundedCorners = r; }
    void setCornerRadius(double r) { m_cornerRadius = r; }

private:
    ShapeType m_shapeType;
    QPointF m_startPoint;
    bool m_roundedCorners = false;
    double m_cornerRadius = 10.0;
    QGraphicsItem *m_previewItem;
    bool m_fillByDefault;
    QColor m_shapeFillColor = QColor(100, 149, 237); // Default cornflower blue fill
};

#endif // SHAPETOOL_H
