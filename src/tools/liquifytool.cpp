#include "liquifytool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include "canvas/objects/vectorobject.h"
#include <cmath>

LiquifyTool::LiquifyTool(QObject *parent) : Tool(ToolType::Blend, parent) {}

void LiquifyTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    m_isDrawing = true;
    m_lastPoint = event->scenePos();
    m_affectedObjects.clear();
    // Start an undo macro so all point movements are one "Undo" action
    if (canvas->undoStack()) canvas->undoStack()->beginMacro("Liquify Warp");
}

void LiquifyTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!m_isDrawing) return;
    warpAtPoint(event->scenePos(), canvas);
}

void LiquifyTool::warpAtPoint(const QPointF &pos, VectorCanvas *canvas)
{
    // 1. Find everything under the brush radius
    qreal r = m_strokeWidth * 2.0;
    QRectF brushArea(pos.x() - r, pos.y() - r, r * 2, r * 2);
    auto items = canvas->items(brushArea);

    QPointF delta = pos - m_lastPoint; // Direction of the mouse swipe

    for (auto* item : items) {
        // FIX #17: Resolve display clones to their source objects so changes
        // persist after refreshFrame(). Previously we were modifying display
        // clones which get discarded on the next canvas refresh.
        VectorObject *displayVO = dynamic_cast<VectorObject*>(item);
        if (!displayVO) continue;

        VectorObject *sourceVO = canvas->sourceObject(displayVO);
        if (!sourceVO) sourceVO = displayVO; // fallback if already source

        PathObject* pathObj = dynamic_cast<PathObject*>(sourceVO);
        if (!pathObj) continue;

        QPainterPath path = pathObj->path();
        bool changed = false;

        // 2. Iterate through path elements and "push" them
        for (int i = 0; i < path.elementCount(); ++i) {
            QPainterPath::Element el = path.elementAt(i);
            QPointF pointInScene = pathObj->mapToScene(el.x, el.y);

            qreal dist = QLineF(pos, pointInScene).length();

            if (dist < r) {
                // Calculate "Pressure" (stronger at center of brush)
                qreal falloff = 1.0 - (dist / r);
                QPointF movement = delta * falloff * m_pushStrength;

                QPointF newPos = pathObj->mapFromScene(pointInScene + movement);
                path.setElementPositionAt(i, newPos.x(), newPos.y());
                changed = true;
            }
        }

        if (changed) {
            pathObj->setPath(path);
            m_affectedObjects.insert(pathObj);
        }
    }
    m_lastPoint = pos;
    // Refresh canvas so user can see the warp in real time
    canvas->update();
}

void LiquifyTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    m_isDrawing = false;
    // Refresh to ensure source objects are synced to display
    canvas->refreshFrame();
    if (canvas->undoStack()) canvas->undoStack()->endMacro();
}
