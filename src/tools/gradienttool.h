#pragma once
#include "tool.h"
#include <QPointF>
#include <QColor>

class GradientObject;

// Gradient tool — like Krita's gradient tool:
//   • Click and drag to define the start → end vector.
//   • While dragging a live preview is shown.
//   • Release to commit the gradient to the layer.
//   • Hold Shift to snap to 45° increments.
//   • The tool settings panel controls gradient type (linear/radial),
//     start color, end color, and repeat mode.
class GradientTool : public Tool
{
    Q_OBJECT

public:
    explicit GradientTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    // Settings (read/written by ToolSettingsPanel)
    enum GType { Linear, Radial };
    GType   gradientType() const    { return m_gradType; }
    QColor  startColor()   const    { return m_startColor; }
    QColor  endColor()     const    { return m_endColor; }
    bool    repeat()       const    { return m_repeat; }

public slots:
    void setGradientType(GType t)         { m_gradType   = t; }
    void setStartColor(const QColor &c)   { m_startColor = c; }
    void setEndColor(const QColor &c)     { m_endColor   = c; }
    void setRepeat(bool r)                { m_repeat     = r; }

signals:
    void settingsChanged();

private:
    GType   m_gradType   = Linear;
    QColor  m_startColor = Qt::white;
    QColor  m_endColor   = Qt::transparent;
    bool    m_repeat     = false;
    bool    m_isDragging = false;

    GradientObject *m_liveGradient = nullptr;

    QPointF snapAngle(QPointF from, QPointF to, bool snap45) const;
};
