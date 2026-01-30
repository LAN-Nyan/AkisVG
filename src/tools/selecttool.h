/*
 * File: src/tools/selecttool.h
 *
 * REPLACE THE ENTIRE EXISTING FILE with this content
 */

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

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    // For context menu - call this from canvas or mainwindow
    void showContextMenu(const QPoint &globalPos, VectorCanvas *canvas);

    // Get currently selected objects
    QList<VectorObject*> selectedObjects() const { return m_selectedObjects; }

signals:
    void selectionChanged(const QList<VectorObject*> &selectedObjects);
    void requestGroupForInterpolation(const QList<VectorObject*> &objects);

private:
    bool m_isDragging;
    QPointF m_dragStart;
    QGraphicsRectItem *m_selectionRect;
    QList<VectorObject*> m_selectedObjects;

    void updateSelection(VectorCanvas *canvas, const QRectF &rect, bool additive);
    void clearSelection();
};

#endif // SELECTTOOL_H
