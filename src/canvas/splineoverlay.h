#ifndef SPLINEOVERLAY_H
#define SPLINEOVERLAY_H

#include <QWidget>
#include <QList>
#include <QPointF>
#include <QPainter>
#include <QMouseEvent>

/**
 * SplineOverlay — a transparent widget that sits on top of the CanvasView
 * during interpolation mode.  Users click to add control nodes; dragging
 * a node repositions it.  The curve is drawn as a cubic Bezier spline
 * through all nodes using Catmull-Rom parameterisation.
 *
 * The overlay emits splineChanged() whenever nodes move so the
 * InterpolationEngine can read them.
 */
class SplineOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit SplineOverlay(QWidget *parent = nullptr);

    void setNodes(const QList<QPointF> &nodes);
    QList<QPointF> nodes() const { return m_nodes; }
    void clearNodes();

    // Call to commit the spline as animation keyframes
    void commitSpline();

signals:
    void splineChanged(const QList<QPointF> &nodes);
    void committed(const QList<QPointF> &nodes);
    void exitRequested();           // user clicked "Done"

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    static const int NODE_RADIUS = 8;
    static const int HIT_RADIUS  = 12;

    int nodeAt(const QPointF &pos) const;
    void drawSpline(QPainter &p) const;
    QList<QPointF> catmullRomPoints(int segments = 20) const;

    QList<QPointF> m_nodes;
    int  m_draggingIdx = -1;
    bool m_dragging    = false;
};

#endif // SPLINEOVERLAY_H
