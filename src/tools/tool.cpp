#include "tool.h"
#include "canvas/vectorcanvas.h"

Tool::Tool(ToolType type, QObject *parent)
    : QObject(parent)
    , m_type(type)
    , m_strokeColor(Qt::black)
    , m_fillColor(Qt::transparent)
    , m_strokeWidth(2.0)
    , m_isDrawing(false)
{
    switch (type) {
    case ToolType::None:       m_name = ""; break;
    case ToolType::Select:     m_name = "Select"; break;
    case ToolType::Pencil:     m_name = "Pencil"; break;
    case ToolType::Brush:      m_name = "Brush"; break;
    case ToolType::Eraser:     m_name = "Eraser"; break;
    case ToolType::Rectangle:  m_name = "Rectangle"; break;
    case ToolType::Shape:      m_name = "Shape"; break;
    case ToolType::Ellipse:    m_name = "Ellipse"; break;
    case ToolType::Line:       m_name = "Line"; break;
    case ToolType::Text:       m_name = "Text"; break;
    case ToolType::Fill:       m_name = "Fill"; break;
    case ToolType::Gradient:   m_name = "Gradient"; break;
    case ToolType::Blend:      m_name = "Blend"; break;
    case ToolType::eyedropper: m_name = "Eyedropper"; break;
    case ToolType::Liquify:    m_name = "Liquify"; break;
    case ToolType::Lasso:      m_name = "Lasso";      break;
    case ToolType::MagicWand:  m_name = "Magic Wand"; break;
    }
}

void Tool::setStrokeColor(const QColor &color)
{
    if (m_strokeColor != color) { m_strokeColor = color; emit settingsChanged(); }
}
void Tool::setFillColor(const QColor &color)
{
    if (m_fillColor != color) { m_fillColor = color; emit settingsChanged(); }
}
void Tool::setStrokeWidth(qreal width)
{
    if (m_strokeWidth != width) { m_strokeWidth = qMax(0.1, width); emit settingsChanged(); }
}

// ── New-style stubs – explicitly ignore so canvasview can detect non-handling ─
// NOTE: These are only called for Lasso/MagicWand which override them.
// Legacy tools never reach these; canvasview routes them via isNewStyleTool().
void Tool::mousePressEvent  (QMouseEvent *event, QPointF) { event->ignore(); }
void Tool::mouseMoveEvent   (QMouseEvent *event, QPointF) { event->ignore(); }
void Tool::mouseReleaseEvent(QMouseEvent *event, QPointF) { event->ignore(); }
void Tool::draw(QPainter *painter)                        { Q_UNUSED(painter) }

// ── Legacy scene events (existing tools override these) ───────────────────────
void Tool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    event->setAccepted(true);
    m_isDrawing = true;
}
void Tool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event) Q_UNUSED(canvas)
}
void Tool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event) Q_UNUSED(canvas)
    m_isDrawing = false;
}
