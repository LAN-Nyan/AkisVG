#include "canvasview.h"
#include "vectorcanvas.h"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar>
// #include <QOpenGLWidget>  // Commented out - requires qt6-openglwidgets
#include <QtMath>

void CanvasView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // 1. Fill everything with the "Desk" color (Grey)
    painter->fillRect(rect, QColor(60, 60, 60));

    // 2. Draw the "Paper" (White)
    // This uses the sceneRect defined in your VectorCanvas
    QRectF canvasRect = sceneRect();
    painter->fillRect(canvasRect, Qt::white);

    // 3. Optional: Add a subtle border/shadow so the white stands out
    painter->setPen(QPen(QColor(40, 40, 40), 1));
    painter->drawRect(canvasRect);
}

CanvasView::CanvasView(VectorCanvas *canvas, QWidget *parent)
    : QGraphicsView(canvas, parent)
    , m_currentZoom(1.0)
    , m_zoomMin(0.01) // Ensure these members are initialized
    , m_zoomMax(50.0)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // Center on canvas initially
    if (canvas) {
        centerOn(canvas->sceneRect().center());
    }
}
// Also add this to stop panning when you let go of Space
void CanvasView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        setDragMode(QGraphicsView::NoDrag);
        event->accept();
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void CanvasView::zoomIn()
{
    setZoom(m_currentZoom * 1.2);
}

void CanvasView::zoomOut()
{
    setZoom(m_currentZoom / 1.2);
}

void CanvasView::resetZoom()
{
    setZoom(1.0);
}

void CanvasView::fitCanvas()
{
    QGraphicsView::fitInView(sceneRect(), Qt::KeepAspectRatio);
    m_currentZoom = transform().m11();
}

void CanvasView::setZoom(qreal factor)
{
    factor = qBound(m_zoomMin, factor, m_zoomMax);

    qreal scaleFactor = factor / m_currentZoom;
    scale(scaleFactor, scaleFactor);
    m_currentZoom = factor;
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + wheel = zoom
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (event->modifiers() & Qt::ControlModifier) {
            zoomIn();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Minus:
        if (event->modifiers() & Qt::ControlModifier) {
            zoomOut();
            event->accept();
            return;
        }
        break;
    case Qt::Key_0:
        if (event->modifiers() & Qt::ControlModifier) {
            resetZoom();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Space:
        // Space = pan mode (hold space and drag)
        setDragMode(QGraphicsView::ScrollHandDrag);
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}
