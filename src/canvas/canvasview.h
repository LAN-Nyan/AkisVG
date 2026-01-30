#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>

class VectorCanvas;

class CanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CanvasView(VectorCanvas *canvas, QWidget *parent = nullptr);
    
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitCanvas();
    void recenterCanvas();

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setZoom(qreal factor);
    
    qreal m_currentZoom;
    const qreal m_zoomMin = 0.1;
    const qreal m_zoomMax = 10.0;
    
    // Middle mouse panning
    bool m_isPanning;
    QPoint m_panStartPos;
};

#endif // CANVASVIEW_H
