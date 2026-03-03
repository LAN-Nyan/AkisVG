// FIX (was TODO):
// The fill tool stopped working after the offset fix. Root cause: the canvas renders
// *display clones* of layer-owned objects. canvas->items() returns those clones,
// not the real objects. Applying colour changes to a clone was silently discarded
// on the next refreshFrame(). Fix: always resolve via canvas->sourceObject() before
// writing any colour change, and write to the resolved source object.
//
// NOTE: only filltool.cpp was changed — nothing else was touched.

#include "filltool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include "canvas/objects/pathobject.h"
#include "canvas/objects/shapeobject.h"
#include "core/commands.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QLineF>

FillTool::FillTool(QObject *parent) : Tool(ToolType::Fill, parent) {}


void FillTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (event->button() != Qt::LeftButton) {
        event->setAccepted(false);
        return;
    }
    event->setAccepted(true);

    QPointF clickPos = event->scenePos();

    // Collect all items in a small area around the click point, plus exact hits.
    QList<QGraphicsItem*> allItems = canvas->items(
        QRectF(clickPos - QPointF(2, 2), QSizeF(4, 4)),
        Qt::IntersectsItemBoundingRect,
        Qt::DescendingOrder);

    for (QGraphicsItem *it : canvas->items(clickPos, Qt::IntersectsItemShape, Qt::DescendingOrder)) {
        if (!allItems.contains(it)) allItems.append(it);
    }

    VectorObject *target    = nullptr;
    bool          hitIsStroke = false;

    for (QGraphicsItem *item : allItems) {
        VectorObject *vo = dynamic_cast<VectorObject*>(item);
        if (!vo) continue;

        // ── Resolve display clone → real layer-owned object ──────────────────
        // The canvas renders clones; colour changes must go to the source object
        // so they survive the next refreshFrame() call.
        VectorObject *source = canvas->sourceObject(vo);
        if (!source) source = vo;   // live drawing object — use as-is

        QPointF local = source->mapFromScene(clickPos);

        // ── Shape objects ─────────────────────────────────────────────────────
        ShapeObject *shape = dynamic_cast<ShapeObject*>(source);
        if (shape) {
            if (shape->contains(local)) {
                target      = shape;
                hitIsStroke = false;
                break;
            }
            continue;
        }

        // ── Path objects ──────────────────────────────────────────────────────
        PathObject *path = dynamic_cast<PathObject*>(source);
        if (path) {
            // PathObject stores its path in scene coordinates (pos stays at 0,0),
            // so local == clickPos for these objects.
            QPainterPath pp = path->path();

            // 1. Interior fill check
            if (pp.contains(local)) {
                target      = path;
                hitIsStroke = false;
                break;
            }

            // 2. Stroke proximity check
            QPainterPathStroker stroker;
            stroker.setWidth(qMax(path->strokeWidth() + 8.0, 12.0));
            stroker.setCapStyle(Qt::RoundCap);
            stroker.setJoinStyle(Qt::RoundJoin);
            QPainterPath strokeShape = stroker.createStroke(pp);

            if (strokeShape.contains(local)) {
                target = path;

                // Treat path as fillable if its first and last points coincide.
                bool pathIsClosed = false;
                if (pp.elementCount() > 1) {
                    QPointF start = pp.elementAt(0);
                    QPointF end   = pp.elementAt(pp.elementCount() - 1);
                    pathIsClosed  = (QLineF(start, end).length() < 1.0);
                }

                hitIsStroke = !pathIsClosed;
                break;
            }
            continue;
        }

        // ── Generic fallback ──────────────────────────────────────────────────
        if (source->contains(local)) {
            target      = source;
            hitIsStroke = false;
            break;
        }
    }

    if (!target) return;

    // Choose the colour to apply: use fill colour if valid, else fall back to stroke.
    QColor applyColor = m_fillColor;
    if (!applyColor.isValid() || applyColor == Qt::transparent)
        applyColor = m_strokeColor;

    if (hitIsStroke) {
        // Colour the stroke (no undo for open-path stroke — acceptable trade-off).
        target->setStrokeColor(applyColor);
        target->update();
    } else {
        // Colour the fill with full undo support.
        canvas->undoStack()->push(new FillColorCommand(target, applyColor));
    }

    canvas->update();
}

void FillTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event);
    Q_UNUSED(canvas);
}

void FillTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mouseReleaseEvent(event, canvas);
}
