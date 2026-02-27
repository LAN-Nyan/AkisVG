#include "liquifytool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include <cmath>

LiquifyTool::LiquifyTool(QObject *parent) : Tool(ToolType::Blend, parent) {}

void LiquifyTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    m_isDrawing = true;
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
        PathObject* pathObj = dynamic_cast<PathObject*>(item);
        if (!pathObj) continue;

        QPainterPath path = pathObj->path();
        bool changed = false;
        QPainterPath newPath;

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
}

void LiquifyTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    m_isDrawing = false;
    if (canvas->undoStack()) canvas->undoStack()->endMacro();
}
