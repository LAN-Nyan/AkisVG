#include "selecttool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include "canvas/objects/objectgroup.h"
#include "utils/debuglog.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QtMath>

SelectTool::SelectTool(QObject *parent)
    : Tool(ToolType::Select, parent)
{
}

SelectTool::~SelectTool()
{
    m_selectionRect = nullptr;
}

VectorObject *SelectTool::topObjectAt(QPointF scenePos, VectorCanvas *canvas) const
{
    for (QGraphicsItem *item : canvas->items(scenePos)) {
        if (item == m_selectionRect)
            continue;
        if (auto *vo = dynamic_cast<VectorObject *>(item))
            return canvas->sourceObject(vo);
    }
    return nullptr;
}

void SelectTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    AKIS_LOG(Tool, QStringLiteral("SelectTool press pos=(%1,%2) sel=%3")
                        .arg(event->scenePos().x(), 0, 'f', 1)
                        .arg(event->scenePos().y(), 0, 'f', 1)
                        .arg(m_selectedObjects.size()));
    if (!canvas || event->button() != Qt::LeftButton) return;

    const QPointF pos = event->scenePos();
    m_dragStart   = pos;
    m_pressPos    = pos;
    m_awaitingMove = false;
    m_isMovingObjects = false;
    m_isRubberBanding = false;

    VectorObject *clickedSrc = topObjectAt(pos, canvas);
    const bool shift = event->modifiers() & Qt::ShiftModifier;

    if (clickedSrc) {
        if (shift) {
            if (m_selectedObjects.contains(clickedSrc))
                m_selectedObjects.removeOne(clickedSrc);
            else
                m_selectedObjects.append(clickedSrc);
        } else if (!m_selectedObjects.contains(clickedSrc)) {
            clearSelection(canvas);
            m_selectedObjects.append(clickedSrc);
        }

        // Only start moving after the user drags past the threshold.
        m_awaitingMove = !m_selectedObjects.isEmpty();
        m_lastDragPos  = pos;

        canvas->showSelectionOverlays(m_selectedObjects);
        emit selectionChanged(m_selectedObjects);
        event->accept();
        return;
    }

    // Empty canvas click — deselect immediately.
    if (!shift)
        clearSelection(canvas);

    m_isRubberBanding = true;
    if (!m_selectionRect) {
        m_selectionRect = new QGraphicsRectItem();
        m_selectionRect->setPen(QPen(QColor(220, 50, 50), 1.5, Qt::DashLine));
        m_selectionRect->setBrush(QBrush(QColor(220, 50, 50, 25)));
        m_selectionRect->setZValue(10000);
        m_selectionRect->setAcceptedMouseButtons(Qt::NoButton);
        canvas->addItem(m_selectionRect);
    }
    m_selectionRect->setRect(QRectF(pos, pos));
    m_selectionRect->setVisible(true);
    event->accept();
}

void SelectTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    const QPointF pos = event->scenePos();

    if (m_awaitingMove && !m_isMovingObjects && !m_selectedObjects.isEmpty()) {
        if (QLineF(m_pressPos, pos).length() >= m_dragThreshold) {
            m_isMovingObjects = true;
            m_lastDragPos = m_pressPos;
        }
    }

    if (m_isMovingObjects && !m_selectedObjects.isEmpty()) {
        const QPointF delta = pos - m_lastDragPos;
        m_lastDragPos = pos;

        for (VectorObject *src : m_selectedObjects) {
            src->moveBy(delta.x(), delta.y());
            if (VectorObject *clone = canvas->displayCloneFor(src))
                clone->moveBy(delta.x(), delta.y());
        }
        canvas->showSelectionOverlays(m_selectedObjects);
        event->accept();
        return;
    }

    if (m_isRubberBanding) {
        if (m_selectionRect)
            m_selectionRect->setRect(QRectF(m_dragStart, pos).normalized());
        event->accept();
        return;
    }

    event->setAccepted(false);
}

void SelectTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    if (m_isMovingObjects) {
        m_isMovingObjects = false;
        m_awaitingMove = false;
        canvas->refreshFrame();
        event->accept();
        return;
    }

    if (m_isRubberBanding) {
        const QPointF rel = event->scenePos() - m_dragStart;
        if (qAbs(rel.x()) < m_dragThreshold && qAbs(rel.y()) < m_dragThreshold) {
            if (m_selectionRect) m_selectionRect->setVisible(false);
            m_isRubberBanding = false;
            canvas->showSelectionOverlays(m_selectedObjects);
            emit selectionChanged(m_selectedObjects);
            event->accept();
            return;
        }

        const QRectF selArea = QRectF(m_dragStart, event->scenePos()).normalized();
        const bool shift = event->modifiers() & Qt::ShiftModifier;
        updateSelection(canvas, selArea, shift);
        if (m_selectionRect) m_selectionRect->setVisible(false);
        m_isRubberBanding = false;
        canvas->showSelectionOverlays(m_selectedObjects);
        emit selectionChanged(m_selectedObjects);
        event->accept();
        return;
    }

    m_awaitingMove = false;
    event->accept();
}

void SelectTool::updateSelection(VectorCanvas *canvas, const QRectF &rect, bool additive)
{
    if (!additive) clearSelection(canvas);

    for (QGraphicsItem *item : canvas->items(rect, Qt::IntersectsItemShape)) {
        if (item == m_selectionRect) continue;
        VectorObject *display = dynamic_cast<VectorObject *>(item);
        if (!display) continue;
        VectorObject *src = canvas->sourceObject(display);
        if (src && !m_selectedObjects.contains(src))
            m_selectedObjects.append(src);
    }
}

void SelectTool::clearSelection(VectorCanvas *canvas)
{
    AKIS_LOG(Tool, QStringLiteral("SelectTool clearSelection"));
    m_selectedObjects.clear();
    if (canvas)
        canvas->showSelectionOverlays(m_selectedObjects);
    emit selectionChanged(m_selectedObjects);
}

void SelectTool::notifyGrouped(ObjectGroup *group, const QList<VectorObject *> &members, VectorCanvas *canvas)
{
    Q_UNUSED(members);
    m_selectedObjects.clear();
    if (group)
        m_selectedObjects.append(group);
    if (canvas)
        canvas->showSelectionOverlays(m_selectedObjects);
    emit selectionChanged(m_selectedObjects);
}

void SelectTool::notifyUngrouped(ObjectGroup *ungrouped, const QList<VectorObject *> &released, VectorCanvas *canvas)
{
    Q_UNUSED(ungrouped);
    m_selectedObjects = released;
    if (canvas)
        canvas->showSelectionOverlays(m_selectedObjects);
    emit selectionChanged(m_selectedObjects);
}

void SelectTool::setSelectedObjects(const QList<VectorObject*> &objects)
{
    m_selectedObjects = objects;
    emit selectionChanged(m_selectedObjects);
}
