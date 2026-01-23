#include "tool.h"

Tool::Tool(ToolType type, QObject *parent)
    : QObject(parent)
    , m_type(type)
    , m_strokeColor(Qt::black)
    , m_fillColor(Qt::transparent)
    , m_strokeWidth(2.0)
    , m_isDrawing(false)
{
    switch (type) {
    case ToolType::Select:   m_name = "Select"; break;
    case ToolType::Pencil:   m_name = "Pencil"; break;
    case ToolType::Brush:    m_name = "Brush"; break;
    case ToolType::Eraser:   m_name = "Eraser"; break;
    case ToolType::Rectangle: m_name = "Rectangle"; break;
    case ToolType::Ellipse:  m_name = "Ellipse"; break;
    case ToolType::Text:     m_name = "Text"; break;
    }
}

void Tool::setStrokeColor(const QColor &color)
{
    if (m_strokeColor != color) {
        m_strokeColor = color;
        emit settingsChanged();
    }
}

void Tool::setFillColor(const QColor &color)
{
    if (m_fillColor != color) {
        m_fillColor = color;
        emit settingsChanged();
    }
}

void Tool::setStrokeWidth(qreal width)
{
    if (m_strokeWidth != width) {
        m_strokeWidth = qMax(0.1, width);
        emit settingsChanged();
    }
}

void Tool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    m_isDrawing = true;
}

void Tool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
}

void Tool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    m_isDrawing = false;
}
