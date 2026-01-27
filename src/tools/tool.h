#ifndef TOOL_H
#define TOOL_H

#include <QObject>
#include <QGraphicsSceneMouseEvent>
#include <QColor>

class VectorCanvas;

enum class ToolType {
    Select,
    Pencil,
    Brush,
    Eraser,
    Rectangle,
    Ellipse,
    Line,     // ADDED: Line tool
    Text,
    Fill,
    Blend
};

enum class ToolTexture { Smooth, Grainy, Chalk, Canvas };

class Tool : public QObject
{
    Q_OBJECT

public:
    explicit Tool(ToolType type, QObject *parent = nullptr);
    virtual ~Tool() = default;

    void setTexture(ToolTexture tex) { m_texture = tex; }
    ToolTexture texture() const { return m_texture; }

    ToolType type() const { return m_type; }
    QString name() const { return m_name; }

    // Tool settings
    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor &color);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &color);

    qreal strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(qreal width);

    // Mouse events - override in subclasses
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas);

signals:
    void settingsChanged();

protected:
    ToolType m_type;
    QString m_name;
    QColor m_strokeColor;
    QColor m_fillColor;
    qreal m_strokeWidth;
    bool m_isDrawing;
    ToolTexture m_texture = ToolTexture::Smooth;
};

#endif // TOOL_H
