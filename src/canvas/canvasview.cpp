#include "canvasview.h"
#include "vectorcanvas.h"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar>
// #include <QOpenGLWidget>  // Commented out - requires qt6-openglwidgets
#include <QtMath>

CanvasView::CanvasView(VectorCanvas *canvas, QWidget *parent)
    : QGraphicsView(canvas, parent)
    , m_currentZoom(1.0)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    
    // Enable OpenGL acceleration for better performance
    // setViewport(new QOpenGLWidget());  // Uncomment if OpenGL is available
    
    // Center on canvas
    centerOn(canvas->sceneRect().center());
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

void CanvasView::fitInView()
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
