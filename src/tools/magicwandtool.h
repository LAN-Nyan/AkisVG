#pragma once
#include "tool.h"
#include <QImage>
#include <QPolygonF>
#include <QPoint>
#include <QVector>

class MagicWandTool : public Tool
{
    Q_OBJECT

public:
    explicit MagicWandTool(QObject *parent = nullptr);
    ~MagicWandTool() override = default;

    int  tolerance()   const { return m_tolerance; }
    void setTolerance(int t)  { m_tolerance = qBound(0, t, 255); }

    bool contiguous()  const { return m_contiguous; }
    void setContiguous(bool c){ m_contiguous = c; }

    void mousePressEvent  (QMouseEvent *event, QPointF scenePos) override;
    void mouseMoveEvent   (QMouseEvent *event, QPointF scenePos) override;
    void mouseReleaseEvent(QMouseEvent *event, QPointF scenePos) override;
    void draw(QPainter *painter) override;

    void setCanvasSnapshot(const QImage &img, QPointF sceneOrigin);

    bool      hasSelection()     const { return !m_selectionMask.isNull(); }
    QPolygonF selectionPolygon() const { return m_outlinePolygon; }

    void cancel();

signals:
    void actionFill(const QPolygonF &polygon, const QColor &color);
    void actionCut (const QPolygonF &polygon);
    void actionCopy(const QPolygonF &polygon);

private:
    int   m_tolerance  = 32;
    bool  m_contiguous = true;

    QImage    m_snapshot;
    QPointF   m_sceneOrigin;
    QImage    m_selectionMask;
    QPolygonF m_outlinePolygon;

    bool      colorInRange(QRgb candidate, QRgb seed) const;
    QImage    floodFill   (QPoint seed) const;
    QImage    globalSelect(QRgb seed)   const;
    QPolygonF buildOutline(const QImage &mask) const;
    void      showActionMenu();   // Qt6: no widget param, uses QApplication::activeWindow()
};
