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

    qreal radius = m_strokeWidth / 2.0;
    QRectF searchRect(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
    QList<QGraphicsItem*> itemsAtPoint = canvas->items(searchRect, Qt::IntersectsItemBoundingRect);

    PathObject *closestPath = nullptr;
    qreal closestDistance = radius * 2;
    int checkedCount = 0;

    for (QGraphicsItem *item : itemsAtPoint) {
        if (checkedCount >= 5) break;

        PathObject *pathObj = dynamic_cast<PathObject*>(item);
        if (!pathObj) continue;

        if (pathObj == m_activeSmear) continue;
        if (pathObj->opacity() < 0.99) continue;

        checkedCount++;
        QPointF pathCenter = pathObj->boundingRect().center();
        qreal distance = QLineF(pos, pathCenter).length();

        if (distance < closestDistance) {
            closestDistance = distance;
            closestPath = pathObj;
        }
    }

    if (closestPath) {
        QColor c = closestPath->strokeColor();
        if (!c.isValid() || c.alpha() < 10) {
            c = closestPath->fillColor();
        }

        // ===== FIX: Only return color if it's actually visible =====
        if (c.isValid() && c.alpha() > 10) {
            return c;
        }
    }

    return QColor();
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
