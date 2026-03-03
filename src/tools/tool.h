#ifndef TOOL_H
#define TOOL_H

#include <QObject>
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QColor>
#include <QCursor>
#include <QPainter>

class VectorCanvas;

enum class ToolType {
    None,
    Select,
    Pencil,
    Brush,
    Eraser,
    Rectangle,
    Shape,
    Ellipse,
    Line,
    Text,
    Fill,
    Gradient,   // ← NEW
    Blend,
    Liquify,
    eyedropper,
    Lasso,
    MagicWand,
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

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor &color);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &color);

    qreal strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(qreal width);

    QCursor cursor() const { return m_cursor; }

    // Pressure sensitivity — called by CanvasView on every tablet event.
    // Tools that support pressure call currentPressure() in mouseMoveEvent.
    void setCurrentPressure(qreal p) { m_currentPressure = qBound(0.05, p, 1.0); }
    qreal currentPressure() const { return m_currentPressure; }
    void setPressureSensitivity(bool on) { m_pressureSensitive = on; }
    bool pressureSensitive() const { return m_pressureSensitive; }

    // Stroke opacity (0.0-1.0) - applied to PathObject::m_objectOpacity
    void   setStrokeOpacity(qreal op) { m_strokeOpacity = qBound(0.01, op, 1.0); }
    qreal  strokeOpacity() const { return m_strokeOpacity; }

    // Smoothing amount (0-100)
    void  setSmoothingAmount(int v) { m_smoothingAmount = qBound(0, v, 100); }
    int   smoothingAmount() const { return m_smoothingAmount; }

    // ── New-style events (Lasso, MagicWand) – CanvasView calls these ──────────
    virtual void mousePressEvent  (QMouseEvent *event, QPointF scenePos);
    virtual void mouseMoveEvent   (QMouseEvent *event, QPointF scenePos);
    virtual void mouseReleaseEvent(QMouseEvent *event, QPointF scenePos);
    virtual void draw(QPainter *painter);

    // ── Legacy scene events (existing tools) ─────────────────────────────────
    virtual void mousePressEvent  (QGraphicsSceneMouseEvent *event, VectorCanvas *canvas);
    virtual void mouseMoveEvent   (QGraphicsSceneMouseEvent *event, VectorCanvas *canvas);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas);

signals:
    void settingsChanged();

protected:
    void setName  (const QString &name)   { m_name   = name; }
    void setCursor(const QCursor  &cur)   { m_cursor = cur; }
    void setCursor(Qt::CursorShape shape) { m_cursor = QCursor(shape); }

    ToolType    m_type;
    QString     m_name;
    QColor      m_strokeColor;
    QColor      m_fillColor;
    qreal       m_strokeWidth;
    bool        m_isDrawing;
    ToolTexture m_texture = ToolTexture::Smooth;
    QCursor     m_cursor  = Qt::ArrowCursor;
    qreal       m_currentPressure = 1.0;   // updated by CanvasView from tablet events
    bool        m_pressureSensitive = true; // user toggle
    qreal       m_strokeOpacity    = 1.0;   // 0.0-1.0, applied to stroke alpha
    int         m_smoothingAmount  = 50;    // 0-100, controls point filtering
};

#endif // TOOL_H
