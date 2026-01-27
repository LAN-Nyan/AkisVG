#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <QGraphicsView>

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

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void keyReleaseEvent(QKeyEvent *event) override; // To fix the spacebar panning
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setZoom(qreal factor);
    
    qreal m_currentZoom;
    const qreal m_zoomMin = 0.1;
    const qreal m_zoomMax = 10.0;
};

#endif // CANVASVIEW_H
