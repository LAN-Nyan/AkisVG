#include "gradientobject.h"
#include <QPainter>
#include <QPen>
#include <cmath>

GradientObject::GradientObject(QGraphicsItem *parent)
    : VectorObject(parent)
{
    m_bounds = QRectF(0, 0, 1, 1);
}

VectorObject* GradientObject::clone() const
{
    GradientObject *c = new GradientObject();
    c->m_gradType   = m_gradType;
    c->m_start      = m_start;
    c->m_end        = m_end;
    c->m_startColor = m_startColor;
    c->m_endColor   = m_endColor;
    c->m_repeat     = m_repeat;
    c->m_bounds     = m_bounds;
    c->setPos(pos());
    c->setRotation(rotation());
    c->setScale(scale());
    c->setObjectOpacity(objectOpacity());
    // NOTE: do NOT copy zValue() — display clones must use natural stacking (z=0)
    return c;
}

void GradientObject::updateBounds()
{
    prepareGeometryChange();
    qreal minX = qMin(m_start.x(), m_end.x());
    qreal minY = qMin(m_start.y(), m_end.y());
    qreal maxX = qMax(m_start.x(), m_end.x());
    qreal maxY = qMax(m_start.y(), m_end.y());
    // Ensure some minimum size so the gradient always renders
    qreal w = qMax(maxX - minX, 1.0);
    qreal h = qMax(maxY - minY, 1.0);
    m_bounds = QRectF(minX, minY, w, h).adjusted(-1, -1, 1, 1);
    update();
}

QRectF GradientObject::boundingRect() const
{
    return m_bounds.adjusted(-12, -12, 12, 12); // extra space for handles
}

void GradientObject::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem * /*option*/,
                           QWidget * /*widget*/)
{
    if (m_bounds.isEmpty()) return;
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->save();

    painter->setOpacity(m_objectOpacity);

    if (m_gradType == Linear) {
        QLinearGradient grad(m_start, m_end);
        grad.setColorAt(0.0, m_startColor);
        grad.setColorAt(1.0, m_endColor);
        if (m_repeat)
            grad.setSpread(QGradient::RepeatSpread);
        else
            grad.setSpread(QGradient::PadSpread);

        // Fill the whole gradient bounding box
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(grad));
        painter->drawRect(m_bounds);

    } else { // Radial
        qreal radius = std::hypot(m_end.x() - m_start.x(), m_end.y() - m_start.y());
        QRadialGradient grad(m_start, radius, m_start);
        grad.setColorAt(0.0, m_startColor);
        grad.setColorAt(1.0, m_endColor);
        if (m_repeat)
            grad.setSpread(QGradient::RepeatSpread);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(grad));
        painter->drawEllipse(m_start, radius, radius);
    }

    // Draw the handles (only visible when selected / while tool is active)
    if (isSelected()) {
        // Line between handles
        painter->setPen(QPen(QColor(255,255,255,200), 1.5, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawLine(m_start, m_end);

        // Start handle (filled white circle)
        painter->setPen(QPen(Qt::black, 1.5));
        painter->setBrush(m_startColor);
        painter->drawEllipse(m_start, 7.0, 7.0);

        // End handle (filled white square rotated 45° = diamond)
        painter->setBrush(m_endColor);
        painter->drawEllipse(m_end, 7.0, 7.0);

        // Small labels
        painter->setFont(QFont("sans-serif", 8));
        painter->setPen(Qt::white);
        painter->drawText(m_start + QPointF(9, -9), "Start");
        painter->drawText(m_end   + QPointF(9, -9), "End");
    }

    painter->restore();
}
