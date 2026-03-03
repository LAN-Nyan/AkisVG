#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <optional>

class VectorCanvas;
class TransformableImageObject;
class QTimer;
enum class HandleRole;

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

    // Set to the spline overlay widget so Enter is passed through when it's active.
    void setSplineOverlay(QWidget *overlay) { m_splineOverlay = overlay; }

    // Start the marching-ants animation timer (called when lasso has a selection)
    void startAntTimer();

    // ── Image transform API ───────────────────────────────────────────────────
    /// Select a TransformableImageObject and show its handles.
    void setSelectedImage(TransformableImageObject *img);

signals:
    void viewportResized();
    void frameNavigationRequested(int delta);
    void wandAboutToClick();  // emitted before MagicWand processes a click so MainWindow can refresh snapshot
    void syncSelectedImage();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void tabletEvent(QTabletEvent *event) override;

private:
    void handleFrameNavKey(QKeyEvent *event);
    void setZoom(qreal factor);

    // Helper: all TransformableImageObjects in the current frame
    QVector<TransformableImageObject*> currentFrameImages();

    qreal m_currentZoom;
    const qreal m_zoomMin = 0.1;
    const qreal m_zoomMax = 10.0;
    bool m_isPanning = false;
    QPoint m_panStartPos;
    QWidget *m_splineOverlay = nullptr;

    // Tablet pressure: updated by tabletEvent, read by tools on mouseMoveEvent.
    // 1.0 = full pressure (also used when no tablet is connected).
    qreal m_tabletPressure = 1.0;
    bool  m_tabletActive   = false;

    // ── Image transform state ─────────────────────────────────────────────────
    TransformableImageObject  *m_selectedImage   = nullptr;
    std::optional<HandleRole>  m_activeHandle;
    bool                       m_movingImage     = false;
    QPointF                    m_lastMouseScene;
    QTimer                    *m_antTimer;
};

#endif // CANVASVIEW_H
