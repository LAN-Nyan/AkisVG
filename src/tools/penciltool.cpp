#include "penciltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <QGraphicsSceneMouseEvent>

PencilTool::PencilTool(QObject *parent)
    : Tool(ToolType::Pencil, parent)
    , m_currentPath(nullptr)
{
}

void PencilTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    event->setAccepted(true);

    m_currentPath = new PathObject();
    m_currentPath->setStrokeColor(m_strokeColor);
    m_currentPath->setStrokeWidth(m_strokeWidth);
    m_currentPath->setTexture(static_cast<PathTexture>(static_cast<int>(m_texture)));

    // Track the start point in a member variable (add QPointF m_lastPoint to penciltool.h)
    m_lastPoint = event->scenePos();

    // PathObject needs a moveTo wrapper just like your BrushTool fix
    m_currentPath->moveTo(m_lastPoint);

    canvas->addObject(m_currentPath);
}

void PencilTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(canvas); // Keeps the compiler happy
    if (!m_currentPath) return;

    QPointF currentPoint = event->scenePos();

    // Check distance to avoid jitter
    if (QLineF(m_lastPoint, currentPoint).length() > 1.5) {

        // Midpoint for smoothing
        QPointF midPoint = (m_lastPoint + currentPoint) / 2.0;

        // Use the smooth wrapper
        m_currentPath->quadTo(m_lastPoint, midPoint);

        m_lastPoint = currentPoint;
        // The PathObject likely calls update() internally now,
        // but if not, keep canvas->update() here.
    }
}

void PencilTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!m_currentPath) return;

    // Finalize: Draw a line to the exact point where the user lifted the mouse
    m_currentPath->lineTo(event->scenePos());

    m_currentPath = nullptr;
    Tool::mouseReleaseEvent(event, canvas);
}
