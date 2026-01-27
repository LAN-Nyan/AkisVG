#include "blendtool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>

BlendTool::BlendTool(QObject *parent)
    : Tool(ToolType::Blend, parent)
    , m_blendStrength(0.5)
{
    setStrokeWidth(20.0);  // Blend brush size
}

void BlendTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);
    m_lastPoint = event->scenePos();
    blendAtPoint(event->scenePos(), canvas);
}

void BlendTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_isDrawing) {
        blendAtPoint(event->scenePos(), canvas);
        m_lastPoint = event->scenePos();
        canvas->update();
    }
}

void BlendTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Tool::mouseReleaseEvent(event, canvas);
}

void BlendTool::blendAtPoint(const QPointF &pos, VectorCanvas *canvas)
{
    // Find all path objects near this point
    QList<QGraphicsItem*> nearbyItems = canvas->items(
        QRectF(pos.x() - m_strokeWidth, pos.y() - m_strokeWidth,
               m_strokeWidth * 2, m_strokeWidth * 2));

    QList<PathObject*> pathsToBlend;
    for (QGraphicsItem *item : nearbyItems) {
        PathObject *pathObj = dynamic_cast<PathObject*>(item);
        if (pathObj) {
            pathsToBlend.append(pathObj);
        }
    }

    // If we have multiple paths, blend their colors at intersection points
    if (pathsToBlend.size() >= 2) {
        for (int i = 0; i < pathsToBlend.size() - 1; ++i) {
            PathObject *path1 = pathsToBlend[i];
            PathObject *path2 = pathsToBlend[i + 1];

            QColor color1 = path1->strokeColor();
            QColor color2 = path2->strokeColor();
            QColor blendedColor = blendColors(color1, color2, m_blendStrength);

            // Apply blended color to both
            path1->setStrokeColor(blendedColor);
            path2->setStrokeColor(blendedColor);
        }
    }
}

QColor BlendTool::blendColors(const QColor &color1, const QColor &color2, qreal factor)
{
    // Linear interpolation between colors
    int r = color1.red() * (1.0 - factor) + color2.red() * factor;
    int g = color1.green() * (1.0 - factor) + color2.green() * factor;
    int b = color1.blue() * (1.0 - factor) + color2.blue() * factor;
    int a = color1.alpha() * (1.0 - factor) + color2.alpha() * factor;

    return QColor(r, g, b, a);
}
