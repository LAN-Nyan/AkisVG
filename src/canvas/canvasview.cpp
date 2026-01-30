#include "canvasview.h"
#include "vectorcanvas.h"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QtMath>

void CanvasView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Fill everything with the workspace color (dark grey)
    painter->fillRect(rect, QColor(60, 60, 60));

    // Draw the canvas "paper" (white)
    QRectF canvasRect = sceneRect();
    painter->fillRect(canvasRect, Qt::white);

    // Add a subtle border so the white stands out
    painter->setPen(QPen(QColor(40, 40, 40), 1));
    painter->drawRect(canvasRect);
}

CanvasView::CanvasView(VectorCanvas *canvas, QWidget *parent)
    : QGraphicsView(canvas, parent)
    , m_currentZoom(1.0)
    , m_zoomMin(0.01)
    , m_zoomMax(50.0)
    , m_isPanning(false)
{
    // PERFORMANCE: Enable high-quality rendering
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);
    
    // PERFORMANCE: Smart viewport updates (only redraw changed areas)
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    
    // PERFORMANCE: Enable OpenGL acceleration (optional, comment out if causes issues)
    // Note: Requires Qt compiled with OpenGL support
    // setViewport(new QOpenGLWidget());  // Uncomment to enable OpenGL
    
    // PERFORMANCE: Cache background
    setCacheMode(QGraphicsView::CacheBackground);
    
    setDragMode(QGraphicsView::NoDrag);

    // Use AnchorUnderMouse for proper zoom behavior
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // Center on canvas initially
    if (canvas) {
        centerOn(canvas->sceneRect().center());
    }
}

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

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        // Middle mouse button for panning
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        // Calculate pan delta
        QPoint delta = event->pos() - m_panStartPos;
        m_panStartPos = event->pos();
        
        // Pan the view
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void CanvasView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    
    // Keep canvas centered when window is resized
    if (scene()) {
        recenterCanvas();
    }
}

void CanvasView::recenterCanvas()
{
    if (scene()) {
        centerOn(sceneRect().center());
    }
}
