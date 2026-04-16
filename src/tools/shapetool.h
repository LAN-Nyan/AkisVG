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
    // FIXED: Only one constructor.
    // It passes the correct ToolType up to the base Tool class immediately.
    // Change the order to match the member variables at the bottom of the file
    explicit ShapeTool(ShapeType shapeType, QObject *parent = nullptr)
        : Tool(shapeType == ShapeType::Rectangle ? ToolType::Rectangle : ToolType::Ellipse, parent)
        , m_shapeType(shapeType) // m_shapeType is declared first
        , m_previewItem(nullptr) // previewItem is declared before fillByDefault
        , m_fillByDefault(false)
    {}

    // Mandatory override to satisfy the pure virtual function in Tool
    ToolType toolType() const override {
        return (m_shapeType == ShapeType::Rectangle) ? ToolType::Rectangle : ToolType::Ellipse;
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    void setFillByDefault(bool fill) { m_fillByDefault = fill; }
    bool fillByDefault() const { return m_fillByDefault; }

    void setShapeFillColor(const QColor &color) { m_shapeFillColor = color; }
    QColor shapeFillColor() const { return m_shapeFillColor; }

    void setRoundedCorners(bool r) { m_roundedCorners = r; }
    void setCornerRadius(double r) { m_cornerRadius = r; }

private:
    ShapeType m_shapeType; // Stores whether this instance is a Rect or Ellipse
    QPointF m_startPoint;
    bool m_roundedCorners = false;
    double m_cornerRadius = 10.0;
    QGraphicsItem *m_previewItem;
    bool m_fillByDefault;
    QColor m_shapeFillColor = QColor(100, 149, 237);
};

#endif // SHAPETOOL_H
