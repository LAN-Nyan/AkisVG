#ifndef SPLINEOVERLAY_H
#define SPLINEOVERLAY_H

#include <QWidget>
#include <QList>
#include <QPointF>
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsView>

/**
 * SplineOverlay — transparent widget on top of CanvasView's viewport.
 *
 * FIX #30: Nodes are now stored in SCENE (canvas) coordinates so they
 * remain correctly positioned regardless of zoom/pan.  The overlay
 * converts to/from viewport coordinates on every paint and mouse event.
 */
class SplineOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit SplineOverlay(QWidget *parent = nullptr);

    // Call this once after construction so coordinate conversions always work.
    // Eliminates fragile parent-chain walking entirely.
    void setView(QGraphicsView *view) { m_view = view; }

    void setNodes(const QList<QPointF> &nodes);
    QList<QPointF> nodes() const { return m_nodes; }
    void clearNodes();

    void commitSpline();

signals:
    void splineChanged(const QList<QPointF> &nodes);
    void committed(const QList<QPointF> &nodes);
    void exitRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    static const int NODE_RADIUS = 8;
    static const int HIT_RADIUS  = 14;

    // m_nodes stores positions in SCENE (canvas) coordinates
    QList<QPointF> m_nodes;
    int  m_draggingIdx = -1;
    bool m_dragging    = false;

    QGraphicsView *m_view = nullptr;  // set via setView() — avoids fragile parent-walking

    QRectF canvasOverlayRect() const;

    // Coordinate conversion: scene ↔ viewport pixels
    QPointF sceneToViewport(const QPointF &scenePos) const;
    QPointF viewportToScene(const QPointF &vpPos) const;

    int nodeAt(const QPointF &vpPos) const;
    void drawSpline(QPainter &p, const QList<QPointF> &vpNodes) const;
    QList<QPointF> catmullRomPoints(const QList<QPointF> &vpNodes, int segments = 20) const;
};

#endif // SPLINEOVERLAY_H
