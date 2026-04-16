#include "blendtool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QtMath>
#include <QGraphicsBlurEffect>
#include <QDebug>

BlendTool::BlendTool(QObject *parent)
    : Tool(ToolType::Blend, parent)
    , m_blendStrength(0.4)
    , m_pickedColor(Qt::white)  // FIX: Default to white instead of transparent
    , m_hasPickedColor(false)
    , m_paintAmount(1.0)
    , m_strokePointCount(0)
{
    setStrokeWidth(40.0);
    setStrokeColor(QColor(255, 100, 100)); // Default red color
}

void BlendTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    Tool::mousePressEvent(event, canvas);
    m_lastPoint = event->scenePos();
    m_lastDrawPos = m_lastPoint;

    // Reset for new stroke
    m_paintAmount = 1.0;
    m_strokePointCount = 0;
    m_activeSmearPath = QPainterPath();

    // Pick starting color
    m_pickedColor = pickColorAtPoint(event->scenePos(), canvas);
    m_hasPickedColor = m_pickedColor.isValid() && m_pickedColor.alpha() > 0;

    // ===== FIX: PROPER COLOR FALLBACK CHAIN =====
    if (!m_hasPickedColor || m_pickedColor == Qt::transparent || m_pickedColor.alpha() == 0) {
        // Use the current stroke color from the color picker
        m_pickedColor = strokeColor();

        // If stroke color is also invalid, use white
        if (!m_pickedColor.isValid() || m_pickedColor.alpha() == 0) {
            m_pickedColor = Qt::white;
        }

        qDebug() << "Blend Tool: Using stroke color fallback:" << m_pickedColor.name();
    } else {
        qDebug() << "Blend Tool: Picked color from canvas:" << m_pickedColor.name();
    }

    m_activeSmear = new PathObject();

    // BLUR EFFECT (Keep this! It helps the mix look "wet")
    QGraphicsBlurEffect *blur = new QGraphicsBlurEffect();
    blur->setBlurRadius(m_strokeWidth * 0.15);
    blur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
    m_activeSmear->setGraphicsEffect(blur);

    m_activeSmear->setFillColor(m_pickedColor);
    m_activeSmear->setStrokeColor(Qt::transparent);
    m_activeSmearPath.setFillRule(Qt::WindingFill);

    if (canvas->undoStack()) {
        canvas->undoStack()->beginMacro("Blend Stroke");
    }

    canvas->addObject(m_activeSmear);
}

void BlendTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!m_isDrawing || !canvas || !m_activeSmear) {
        return;
    }

    QPointF currentPoint = event->scenePos();
    qreal distance = QLineF(m_lastPoint, currentPoint).length();

    if (distance < 1.0) {
        return;
    }

    int steps = qMax(1, qCeil(distance / 2.0));

    for (int i = 1; i <= steps; ++i) {
        qreal t = (qreal)i / steps;
        QPointF blendPoint = m_lastPoint + (currentPoint - m_lastPoint) * t;
        blendAtPoint(blendPoint, canvas);
    }

    m_lastPoint = currentPoint;

    if (m_strokePointCount % 3 == 0) {
        m_activeSmear->update();
    }
}

void BlendTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)

    if (!canvas || !m_activeSmear) {
        Tool::mouseReleaseEvent(event, canvas);
        return;
    }

    m_activeSmear->setPath(m_activeSmearPath);
    canvas->update();

    if (canvas->undoStack()) {
        canvas->undoStack()->endMacro();
    }

    m_activeSmear = nullptr;
    m_activeSmearPath = QPainterPath();
    m_strokePointCount = 0;

    Tool::mouseReleaseEvent(event, canvas);
}

void BlendTool::blendAtPoint(const QPointF &pos, VectorCanvas *canvas)
{
    if (!canvas || !m_activeSmear) return;

    m_strokePointCount++;
    qreal radius = (m_strokeWidth / 2.0) * m_paintAmount;

    // Real color mixing every few pixels
    if (m_strokePointCount % 3 == 0) {
        QColor targetColor = pickColorAtPoint(pos, canvas);

        // ===== FIX: Only mix with VALID colors that aren't transparent =====
        if (targetColor.isValid() && targetColor.alpha() > 10) {
            m_pickedColor = blendColors(m_pickedColor, targetColor, m_blendStrength * 0.5);
        }
    }

    // Smooth geometry
    QPainterPath segmentPath;
    segmentPath.moveTo(m_lastDrawPos);
    segmentPath.lineTo(pos);

    QPainterPathStroker stroker;
    stroker.setWidth(radius * 2.0);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    m_activeSmearPath.addPath(stroker.createStroke(segmentPath));
    m_activeSmear->setPath(m_activeSmearPath);
    m_lastDrawPos = pos;

    // Visuals
    qreal opacity = qBound(0.1, m_paintAmount, 1.0);
    QColor smearColor = m_pickedColor;

    // ===== FIX: Ensure color is always valid =====
    if (!smearColor.isValid() || smearColor.alpha() == 0) {
        smearColor = strokeColor();
        if (!smearColor.isValid() || smearColor.alpha() == 0) {
            smearColor = QColor(255, 255, 255);
        }
    }

    smearColor.setAlphaF(opacity);
    m_activeSmear->setFillColor(smearColor);

    m_paintAmount = qMax(0.1, m_paintAmount - 0.0005);
}

QColor BlendTool::pickColorAtPoint(const QPointF &pos, VectorCanvas *canvas)
{
    if (!canvas) return QColor();

    // FIX #16: Use pixel buffer / texture sampling instead of object lookup.
    // Temporarily hide the active smear so we sample the underlying canvas content.
    bool smearWasVisible = false;
    if (m_activeSmear) {
        smearWasVisible = m_activeSmear->isVisible();
        m_activeSmear->setVisible(false);
    }

    QImage img = canvas->currentImage();

    if (m_activeSmear && smearWasVisible)
        m_activeSmear->setVisible(true);

    if (img.isNull()) return QColor();

    // Average a small region around the sample point for stability
    QRect sceneRect = canvas->sceneRect().toRect();
    int imgW = img.width();
    int imgH = img.height();

    // Map scene pos to image coordinates
    qreal scaleX = imgW / (qreal)(sceneRect.width()  > 0 ? sceneRect.width()  : imgW);
    qreal scaleY = imgH / (qreal)(sceneRect.height() > 0 ? sceneRect.height() : imgH);
    int px = qRound((pos.x() - sceneRect.x()) * scaleX);
    int py = qRound((pos.y() - sceneRect.y()) * scaleY);

    // Sample a 5x5 kernel and average
    int sampleR = 0, sampleG = 0, sampleB = 0, sampleA = 0, count = 0;
    int radius = 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int sx = px + dx;
            int sy = py + dy;
            if (sx < 0 || sy < 0 || sx >= imgW || sy >= imgH) continue;
            QColor c = img.pixelColor(sx, sy);
            if (c.alpha() < 5) continue;
            sampleR += c.red();
            sampleG += c.green();
            sampleB += c.blue();
            sampleA += c.alpha();
            count++;
        }
    }

    if (count == 0) return QColor();
    return QColor(sampleR/count, sampleG/count, sampleB/count, sampleA/count);
}

QColor BlendTool::blendColors(const QColor &color1, const QColor &color2, qreal factor)
{
    factor = qBound(0.0, factor, 1.0);

    // ===== FIX: Ensure both colors are valid before blending =====
    if (!color1.isValid() || color1.alpha() == 0) {
        return color2.isValid() ? color2 : QColor(Qt::white);
    }
    if (!color2.isValid() || color2.alpha() == 0) {
        return color1;
    }

    qreal r = color1.redF() * (1.0 - factor) + color2.redF() * factor;
    qreal g = color1.greenF() * (1.0 - factor) + color2.greenF() * factor;
    qreal b = color1.blueF() * (1.0 - factor) + color2.blueF() * factor;

    return QColor::fromRgbF(r, g, b, color1.alphaF());
}

void BlendTool::setBlendStrength(qreal strength)
{
    m_blendStrength = qBound(0.0, strength, 1.0);
}

qreal BlendTool::distanceToPath(const QPainterPath &path, const QPointF &point) const
{
    QPointF center = path.boundingRect().center();
    return QLineF(point, center).length();
}
