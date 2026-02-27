#include "brushtool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>

BrushTool::BrushTool(QObject *parent)
    : Tool(ToolType::Brush, parent)
    , m_currentPath(nullptr)
{
    setStrokeWidth(8.0); // Brushes are thicker by default
}

void BrushTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);

    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);
    m_currentPath->setTexture(static_cast<PathTexture>(static_cast<int>(m_texture)));

    m_lastPoint = event->scenePos();
    m_currentPath->moveTo(m_lastPoint); // Start the path

    canvas->addObject(m_currentPath);
}

void BrushTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas)

    if (m_isDrawing && m_currentPath) {
        QPointF currentPoint = event->scenePos();

        // 1. Calculate the distance since the last recorded point
        qreal distance = (currentPoint - m_lastPoint).manhattanLength();

        // 2. Only add a segment if the mouse has moved at least 2-3 pixels
        // This "denoises" the input from shaky hands or high-DPI mice
        if (distance > 3.0) {
            QPointF midPoint = (m_lastPoint + currentPoint) / 2.0;

            // Curve toward the midpoint using the last point as a control
            m_currentPath->quadTo(m_lastPoint, midPoint);

            m_lastPoint = currentPoint;
        }
    }
}
void BrushTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (m_isDrawing && m_currentPath) {
        // Draw the final bit to the actual release position
        m_currentPath->lineTo(event->scenePos());
    }

    Tool::mouseReleaseEvent(event, canvas);
    m_currentPath = nullptr;
}
