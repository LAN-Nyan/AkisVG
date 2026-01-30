/*
 * File: src/tools/selecttool.cpp
 *
 * REPLACE THE ENTIRE EXISTING FILE with this content
 */

#include "selecttool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/vectorobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QMenu>
#include <QAction>
#include <QPen>
#include <QBrush>

SelectTool::SelectTool(QObject *parent)
    : Tool(ToolType::Select, parent)
    , m_isDragging(false)
    , m_selectionRect(nullptr)
{
}

SelectTool::~SelectTool()
{
    if (m_selectionRect) {
        delete m_selectionRect;
    }
}

void SelectTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    m_dragStart = event->scenePos();

    // Check if clicking on an object
    QList<QGraphicsItem*> itemsAtPos = canvas->items(event->scenePos());
    VectorObject *clickedObj = nullptr;

    for (QGraphicsItem *item : itemsAtPos) {
        clickedObj = dynamic_cast<VectorObject*>(item);
        if (clickedObj) break;
    }

    // If Shift is NOT held, and we clicked on empty space or a non-selected object
    if (!(event->modifiers() & Qt::ShiftModifier)) {
        if (!clickedObj || !m_selectedObjects.contains(clickedObj)) {
            clearSelection();
        }
    }

    // If clicked on an object, toggle its selection
    if (clickedObj) {
        if (m_selectedObjects.contains(clickedObj)) {
            if (event->modifiers() & Qt::ShiftModifier) {
                // Deselect if Shift is held
                m_selectedObjects.removeOne(clickedObj);
                clickedObj->setSelected(false);
            }
        } else {
            // Add to selection
            m_selectedObjects.append(clickedObj);
            clickedObj->setSelected(true);
        }
        emit selectionChanged(m_selectedObjects);
        event->accept();
    } else {
        // Start drag selection
        m_isDragging = true;

        if (!m_selectionRect) {
            m_selectionRect = new QGraphicsRectItem();
            m_selectionRect->setPen(QPen(QColor(100, 150, 255), 2, Qt::DashLine));
            m_selectionRect->setBrush(QBrush(QColor(100, 150, 255, 30)));
            m_selectionRect->setZValue(10000);  // Draw on top
            canvas->addItem(m_selectionRect);
        }

        m_selectionRect->setRect(QRectF(m_dragStart, m_dragStart));
        m_selectionRect->setVisible(true);
        event->accept();
    }
}

void SelectTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas || !m_isDragging) {
        event->setAccepted(false);
        return;
    }

    QPointF currentPos = event->scenePos();
    QRectF rect = QRectF(m_dragStart, currentPos).normalized();

    if (m_selectionRect) {
        m_selectionRect->setRect(rect);
    }

    event->accept();
}

void SelectTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    if (!canvas) return;

    if (m_isDragging) {
        QPointF endPos = event->scenePos();
        QRectF selectionArea = QRectF(m_dragStart, endPos).normalized();

        // Update selection based on rectangle
        bool additive = (event->modifiers() & Qt::ShiftModifier);
        updateSelection(canvas, selectionArea, additive);

        if (m_selectionRect) {
            m_selectionRect->setVisible(false);
        }

        m_isDragging = false;
        emit selectionChanged(m_selectedObjects);
    }

    event->accept();
}

void SelectTool::updateSelection(VectorCanvas *canvas, const QRectF &rect, bool additive)
{
    if (!additive) {
        clearSelection();
    }

    // Find all vector objects within the selection rectangle
    QList<QGraphicsItem*> itemsInRect = canvas->items(rect, Qt::IntersectsItemShape);

    for (QGraphicsItem *item : itemsInRect) {
        VectorObject *obj = dynamic_cast<VectorObject*>(item);
        if (obj && !m_selectedObjects.contains(obj)) {
            m_selectedObjects.append(obj);
            obj->setSelected(true);
        }
    }
}

void SelectTool::clearSelection()
{
    for (VectorObject *obj : m_selectedObjects) {
        if (obj) {
            obj->setSelected(false);
        }
    }
    m_selectedObjects.clear();
}

void SelectTool::showContextMenu(const QPoint &globalPos, VectorCanvas *canvas)
{
    Q_UNUSED(canvas);

    if (m_selectedObjects.isEmpty()) {
        return;  // No context menu if nothing selected
    }

    QMenu contextMenu;

    QAction *groupAction = contextMenu.addAction("Group for Interpolation");
    contextMenu.addSeparator();
    QAction *duplicateAction = contextMenu.addAction("Duplicate Selected");
    QAction *deleteAction = contextMenu.addAction("Delete Selected");

    // Disable grouping if less than 1 object selected
    groupAction->setEnabled(m_selectedObjects.size() >= 1);

    QAction *selected = contextMenu.exec(globalPos);

    if (selected == groupAction) {
        emit requestGroupForInterpolation(m_selectedObjects);
    } else if (selected == deleteAction) {
        // TODO: Implement delete functionality
    } else if (selected == duplicateAction) {
        // TODO: Implement duplicate functionality
    }
}
