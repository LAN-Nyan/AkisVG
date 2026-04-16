#include "selecttool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>

SelectTool::SelectTool(QObject *parent)
    : Tool(ToolType::Select, parent)
{
}

SelectTool::~SelectTool()
{
    // m_selectionRect is scene-owned — don't delete here, just clear the pointer.
    m_selectionRect = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Returns true if scenePos lands on any currently selected object's display clone.
bool SelectTool::hitTestSelected(QPointF scenePos, VectorCanvas *canvas) const
{
    if (m_selectedObjects.isEmpty()) return false;
    for (QGraphicsItem *item : canvas->items(scenePos)) {
        VectorObject *vo = dynamic_cast<VectorObject*>(item);
        if (!vo) continue;
        VectorObject *src = canvas->sourceObject(vo);
        if (m_selectedObjects.contains(src)) return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse events
// ─────────────────────────────────────────────────────────────────────────────

void SelectTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas || event->button() != Qt::LeftButton) return;

    QPointF pos = event->scenePos();
    m_dragStart = pos;

    // ── Clicked on an already-selected object → start moving them ────────────
    if (hitTestSelected(pos, canvas)) {
        m_isMovingObjects = true;
        m_lastDragPos     = pos;
        event->accept();
        return;
    }

    // ── Find topmost object at click ─────────────────────────────────────────
    VectorObject *clickedDisplay = nullptr;
    for (QGraphicsItem *item : canvas->items(pos)) {
        if (auto *vo = dynamic_cast<VectorObject*>(item)) {
            clickedDisplay = vo;
            break;
        }
    }
    VectorObject *clickedSrc = clickedDisplay
                                   ? canvas->sourceObject(clickedDisplay) : nullptr;

    bool shift = event->modifiers() & Qt::ShiftModifier;

    if (clickedSrc) {
        if (!shift) {
            // Replace selection unless clicking something already selected
            if (!m_selectedObjects.contains(clickedSrc))
                clearSelection();
        }

        if (m_selectedObjects.contains(clickedSrc)) {
            if (shift) {
                m_selectedObjects.removeOne(clickedSrc);
            }
            // else: already selected, prepare to move
        } else {
            m_selectedObjects.append(clickedSrc);
        }

        // Start move immediately
        m_isMovingObjects = true;
        m_lastDragPos     = pos;

        canvas->showSelectionOverlays(m_selectedObjects);
        emit selectionChanged(m_selectedObjects);
        event->accept();

    } else {
        // ── Empty space → rubber-band ─────────────────────────────────────────
        if (!shift) clearSelection();

        m_isRubberBanding = true;

        if (!m_selectionRect) {
            m_selectionRect = new QGraphicsRectItem();
            m_selectionRect->setPen(QPen(QColor(220, 50, 50), 1.5, Qt::DashLine));
            m_selectionRect->setBrush(QBrush(QColor(220, 50, 50, 25)));
            m_selectionRect->setZValue(10000);
            canvas->addItem(m_selectionRect);
        }
        m_selectionRect->setRect(QRectF(pos, pos));
        m_selectionRect->setVisible(true);
        event->accept();
    }
}

void SelectTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    QPointF pos = event->scenePos();

    // ── Move selected objects ─────────────────────────────────────────────────
    if (m_isMovingObjects && !m_selectedObjects.isEmpty()) {
        QPointF delta = pos - m_lastDragPos;
        m_lastDragPos = pos;

        for (VectorObject *src : m_selectedObjects) {
            // Move the source (authoritative position, persists to layer).
            src->moveBy(delta.x(), delta.y());
            // Also move its display clone so the visual feedback is immediate.
            // Without this, the clone stays in the old position until the
            // refreshFrame() on mouseRelease, causing a jarring snap.
            // We deliberately avoid refreshFrame() here because it destroys
            // and recreates all display clones — when a right-click QMenu is
            // open its event loop receives those queued scene-rebuild events,
            // which steal focus and make every menu item unclickable.
            VectorObject *clone = canvas->displayCloneFor(src);
            if (clone) clone->moveBy(delta.x(), delta.y());
        }
        canvas->showSelectionOverlays(m_selectedObjects);
        event->accept();
        return;
    }

    // ── Update rubber-band rect ───────────────────────────────────────────────
    if (m_isRubberBanding) {
        QRectF rect = QRectF(m_dragStart, pos).normalized();
        if (m_selectionRect) m_selectionRect->setRect(rect);
        event->accept();
        return;
    }

    event->setAccepted(false);
}

void SelectTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    // ── Finish object move — commit positions to layer ────────────────────────
    if (m_isMovingObjects) {
        m_isMovingObjects = false;
        // Objects were moved directly on source objects; refresh display clones
        canvas->refreshFrame();
        event->accept();
        return;
    }

    // ── Finish rubber-band selection ──────────────────────────────────────────
    if (m_isRubberBanding) {
        QRectF selArea = QRectF(m_dragStart, event->scenePos()).normalized();
        bool   shift   = event->modifiers() & Qt::ShiftModifier;
        updateSelection(canvas, selArea, shift);
        if (m_selectionRect) m_selectionRect->setVisible(false);
        m_isRubberBanding = false;
        // Show dashed highlight boxes around every selected object.
        canvas->showSelectionOverlays(m_selectedObjects);
        emit selectionChanged(m_selectedObjects);
        event->accept();
        return;
    }

    event->accept();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Selection management
// ─────────────────────────────────────────────────────────────────────────────

void SelectTool::updateSelection(VectorCanvas *canvas, const QRectF &rect, bool additive)
{
    if (!additive) clearSelection();

    for (QGraphicsItem *item : canvas->items(rect, Qt::IntersectsItemShape)) {
        VectorObject *display = dynamic_cast<VectorObject*>(item);
        if (!display || item == m_selectionRect) continue;
        VectorObject *src = canvas->sourceObject(display);
        if (src && !m_selectedObjects.contains(src))
            m_selectedObjects.append(src);
    }
}

void SelectTool::clearSelection()
{
    m_selectedObjects.clear();
    // Visual deselect happens naturally on next refreshFrame.
}

void SelectTool::setSelectedObjects(const QList<VectorObject*> &objects)
{
    m_selectedObjects = objects;
    emit selectionChanged(m_selectedObjects);
}
