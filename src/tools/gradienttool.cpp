#include "gradienttool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/gradientobject.h"
#include <QGraphicsSceneMouseEvent>
#include <cmath>

GradientTool::GradientTool(QObject *parent)
    : Tool(ToolType::Gradient, parent)
{
    setName("Gradient");
    setCursor(Qt::CrossCursor);
    // Defaults: white → transparent linear
    m_startColor = QColor(255, 255, 255, 255);
    m_endColor   = QColor(255, 255, 255, 0);
}

// Snap the end point to the nearest 45° angle from `from` if snap45 is true.
QPointF GradientTool::snapAngle(QPointF from, QPointF to, bool snap45) const
{
    if (!snap45) return to;
    QPointF d = to - from;
    qreal len = std::hypot(d.x(), d.y());
    if (len < 0.01) return to;
    qreal angle = std::atan2(d.y(), d.x());
    qreal snap  = M_PI / 4.0; // 45°
    angle = std::round(angle / snap) * snap;
    return from + QPointF(std::cos(angle) * len, std::sin(angle) * len);
}

void GradientTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (event->button() != Qt::LeftButton) { event->setAccepted(false); return; }
    Tool::mousePressEvent(event, canvas); // sets m_isDrawing=true, event accepted

    QPointF pos = event->scenePos();

    // Create a live preview gradient
    m_liveGradient = new GradientObject();
    m_liveGradient->setGradientType(
        m_gradType == Linear ? GradientObject::Linear : GradientObject::Radial);
    m_liveGradient->setStartColor(m_startColor);
    m_liveGradient->setEndColor(m_endColor);
    m_liveGradient->setRepeat(m_repeat);
    m_liveGradient->setStartPoint(pos);
    m_liveGradient->setEndPoint(pos);
    m_liveGradient->setObjectOpacity(m_strokeColor.alphaF() > 0.05 ? m_strokeColor.alphaF() : 1.0);
    m_liveGradient->setSelected(true); // show handles during drag

    m_isDragging = true;
    canvas->addObject(m_liveGradient);
}

void GradientTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)
    if (!m_isDragging || !m_liveGradient) return;

    bool snap = event->modifiers() & Qt::ShiftModifier;
    QPointF end = snapAngle(m_liveGradient->startPoint(), event->scenePos(), snap);
    m_liveGradient->setEndPoint(end);
}

void GradientTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!m_isDragging || !m_liveGradient) {
        Tool::mouseReleaseEvent(event, canvas);
        return;
    }

    bool snap = event->modifiers() & Qt::ShiftModifier;
    QPointF end = snapAngle(m_liveGradient->startPoint(), event->scenePos(), snap);
    m_liveGradient->setEndPoint(end);
    m_liveGradient->setSelected(false);

    // If the drag was too short, discard
    QPointF d = end - m_liveGradient->startPoint();
    if (std::hypot(d.x(), d.y()) < 5.0) {
        // removeObject handles undo; but this was just added — pop from undo
        canvas->removeObject(m_liveGradient);
    }

    m_isDragging   = false;
    m_liveGradient = nullptr;

    Tool::mouseReleaseEvent(event, canvas);
}
