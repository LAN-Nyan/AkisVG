#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "tool.h"
#include <QPointF>
#include <QList>
#include <QRectF>

class VectorObject;
class QGraphicsRectItem;

class SelectTool : public Tool
{
    Q_OBJECT

public:
    explicit SelectTool(QObject *parent = nullptr);
    ~SelectTool();
    ToolType toolType() const override { return ToolType::Select; }

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    void clearSelection();
    QList<VectorObject*> selectedObjects() const { return m_selectedObjects; }

    // Programmatically pre-select objects (used by Lasso Pull mode).
    // The caller must call canvas->refreshFrame() beforehand so display clones exist.
    void setSelectedObjects(const QList<VectorObject*> &objects);

signals:
    void selectionChanged(const QList<VectorObject*> &selectedObjects);
    void requestGroupForInterpolation(const QList<VectorObject*> &objects);

private:
    // Rubber-band drag state
    bool              m_isRubberBanding = false;
    QPointF           m_dragStart;
    QGraphicsRectItem *m_selectionRect = nullptr;

    // Object-move drag state
    bool    m_isMovingObjects = false;
    QPointF m_lastDragPos;

    QList<VectorObject*> m_selectedObjects; // always SOURCE pointers

    void updateSelection(VectorCanvas *canvas, const QRectF &rect, bool additive);
    bool hitTestSelected(QPointF scenePos, VectorCanvas *canvas) const;
};

#endif // SELECTTOOL_H
