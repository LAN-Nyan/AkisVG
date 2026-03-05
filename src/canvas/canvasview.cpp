#include "canvasview.h"
#include "vectorcanvas.h"
#include "objects/transformableimageobject.h"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QTabletEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QPainter>
#include <QEvent>
#include <QApplication>

// Pull in project/layer types so we can iterate objects
#include "core/project.h"
#include "core/layer.h"
#include "tools/tool.h"
#include "tools/lassotool.h"
#include <QTimer>

CanvasView::CanvasView(VectorCanvas *canvas, QWidget *parent)
    : QGraphicsView(canvas, parent)
    , m_currentZoom(1.0)
    , m_antTimer(nullptr)
{
    setRenderHints(QPainter::Antialiasing |
                   QPainter::SmoothPixmapTransform |
                   QPainter::TextAntialiasing);
    // Ensure antialiasing survives zoom transformations.
    // Qt can drop render hints during scale operations; re-assert here.
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
    setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setDragMode(QGraphicsView::NoDrag);

    // Marching-ants animation: repaint the foreground at ~30fps while a
    // closed lasso selection is visible.
    m_antTimer = new QTimer(this);
    m_antTimer->setInterval(33); // ~30fps
    connect(m_antTimer, &QTimer::timeout, this, [this]() {
        if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
            if (auto *lasso = dynamic_cast<LassoTool*>(canvas->currentTool())) {
                if (lasso->hasSelection()) {
                    viewport()->update();
                    return;
                }
            }
        }
        m_antTimer->stop(); // stop when no active selection
    });
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // Clear m_selectedImage BEFORE display clones are deleted, preventing use-after-free crash.
    // Project::modified fires AFTER refreshFrame, which is too late — we need aboutToRefreshFrame.
    if (canvas) {
        connect(canvas, &VectorCanvas::aboutToRefreshFrame, this, [this]() {
            // Null the pointer before the item it points to is deleted
            m_selectedImage = nullptr;
        });
    }

    viewport()->installEventFilter(this);

    if (canvas)
        centerOn(canvas->sceneRect().center());
}

// ─── helpers ──────────────────────────────────────────────────────────────────

void CanvasView::startAntTimer()
{
    if (m_antTimer && !m_antTimer->isActive())
        m_antTimer->start();
}


void CanvasView::handleFrameNavKey(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Right:
        emit frameNavigationRequested(+1);
        event->accept();
        break;
    case Qt::Key_Left:
        emit frameNavigationRequested(-1);
        event->accept();
        break;
    default:
        break;
    }
}

QVector<TransformableImageObject*> CanvasView::currentFrameImages()
{
    QVector<TransformableImageObject*> result;
    // Walk all items in the scene looking for TransformableImageObject instances.
    // These are display clones — callers that need to edit must resolve to source.
    for (QGraphicsItem *item : scene()->items()) {
        if (auto *img = dynamic_cast<TransformableImageObject*>(item))
            result << img;
    }
    return result;
}

void CanvasView::setSelectedImage(TransformableImageObject *img)
{
    // 1. If it's already selected, do nothing
    if (m_currentZoom && m_selectedImage == img)
        return;

    // 2. Deselect the old one
    if (m_selectedImage)
        m_selectedImage->setSelected(false);

    // 3. Assign the new one
    m_selectedImage = img;

    // 4. Select the new one if it exists
    if (m_selectedImage)
        m_selectedImage->setSelected(true);

    // 5. Redraw
    viewport()->update();
}

void CanvasView::syncSelectedImage()
{
    // After refreshFrame(), all display clones are new objects.
    // The old m_selectedImage pointer is now dangling (the item was deleted).
    // Check if it's still in a scene; if not, clear the pointer so we don't crash.
    if (m_selectedImage && !m_selectedImage->scene()) {
        m_selectedImage = nullptr;
    }
    viewport()->update();
}
// g

// ─── event filter ─────────────────────────────────────────────────────────────

bool CanvasView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == viewport() && event->type() == QEvent::KeyPress) {
        if (m_splineOverlay && m_splineOverlay->isVisible()) {
            QApplication::sendEvent(m_splineOverlay, event);
            return true;
        }
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        int key = ke->key();
        if (key == Qt::Key_Return || key == Qt::Key_Enter ||
            key == Qt::Key_Left   || key == Qt::Key_Right) {
            handleFrameNavKey(ke);
            return true;
        }
    }
    return QGraphicsView::eventFilter(obj, event);
}

// ─── drawing ──────────────────────────────────────────────────────────────────

void CanvasView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(60, 60, 60));
    QRectF canvasRect = sceneRect();
    painter->fillRect(canvasRect, Qt::white);
    painter->setPen(QPen(QColor(40, 40, 40), 1));
    painter->drawRect(canvasRect);
}

void CanvasView::drawForeground(QPainter *painter, const QRectF & /*rect*/)
{
    // FIX #19: Draw transform handles ONLY when Select tool is active
    if (m_selectedImage) {
        bool isSelectTool = false;
        if (auto *canvas = qobject_cast<VectorCanvas*>(scene()))
            if (auto *tool = canvas->currentTool())
                isSelectTool = (tool->type() == ToolType::Select);
        if (isSelectTool)
            m_selectedImage->drawHandles(painter);
    }

    // Let the active tool draw its overlay (Lasso, MagicWand, etc.)
    // VectorCanvas exposes the current tool via currentTool()
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
        if (auto *tool = canvas->currentTool())
            tool->draw(painter);
    }
}

// ─── zoom ─────────────────────────────────────────────────────────────────────

void CanvasView::zoomIn()    { setZoom(m_currentZoom * 1.2); }
void CanvasView::zoomOut()   { setZoom(m_currentZoom / 1.2); }
void CanvasView::resetZoom() { setZoom(1.0); }

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
    // Re-assert antialiasing after scale — Qt6 can clear render hints on transform change
    setRenderHints(QPainter::Antialiasing |
                   QPainter::SmoothPixmapTransform |
                   QPainter::TextAntialiasing);
}

void CanvasView::recenterCanvas()
{
    if (scene())
        centerOn(sceneRect().center());
}

void CanvasView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    emit viewportResized();
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        event->angleDelta().y() > 0 ? zoomIn() : zoomOut();
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

// ─── key events ───────────────────────────────────────────────────────────────

void CanvasView::keyPressEvent(QKeyEvent *event)
{
    if (m_splineOverlay && m_splineOverlay->isVisible()) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    // Forward to current tool first (e.g. Lasso Del key in fill mode)
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
        if (auto *tool = canvas->currentTool()) {
            tool->keyPressEvent(event);
            if (event->isAccepted()) return;
        }
    }

    int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter ||
        key == Qt::Key_Left   || key == Qt::Key_Right) {
        handleFrameNavKey(event);
        return;
    }

    switch (key) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (event->modifiers() & Qt::ControlModifier) { zoomIn();    event->accept(); return; }
        break;
    case Qt::Key_Minus:
        if (event->modifiers() & Qt::ControlModifier) { zoomOut();   event->accept(); return; }
        break;
    case Qt::Key_0:
        if (event->modifiers() & Qt::ControlModifier) { resetZoom(); event->accept(); return; }
        break;
    case Qt::Key_Space:
        setDragMode(QGraphicsView::ScrollHandDrag);
        event->accept();
        return;
    default:
        break;
    }

    QGraphicsView::keyPressEvent(event);
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

// ─── tablet events ────────────────────────────────────────────────────────────

void CanvasView::tabletEvent(QTabletEvent *event)
{
    // Store pressure so mouseMoveEvent (which fires right after) can use it.
    // Qt routes tablet stylus events here first, then generates synthetic mouse
    // events — so this is always called before the corresponding mouse handler.
    m_tabletPressure = qBound(0.05, event->pressure(), 1.0);
    m_tabletActive   = (event->type() != QEvent::TabletRelease);

    // Notify the active tool with the pressure value
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
        if (auto *tool = canvas->currentTool()) {
            QPointF scenePos = mapToScene(event->position().toPoint());
            tool->setCurrentPressure(m_tabletPressure);
            Q_UNUSED(scenePos);
        }
    }

    // Accept but don't consume — let Qt also generate the mouse event so existing
    // tool logic keeps working without any changes.
    event->accept();
}

// ─── mouse events ─────────────────────────────────────────────────────────────

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    // Right-clicks are handled by contextMenuEvent() — don't forward to scene.
    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }

    // ── Image transform hit test (Select tool only) ─────────────────────────
    // Images can only be moved/resized when the Select tool is active.
    bool isSelectTool = false;
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene()))
        if (auto *tool = canvas->currentTool())
            isSelectTool = (tool->type() == ToolType::Select);

    if (isSelectTool && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());

        // Check handles on the already-selected image first
        if (m_selectedImage) {
            auto h = m_selectedImage->hitTestHandle(scenePos);
            if (h.has_value()) {
                m_activeHandle = h;
                m_selectedImage->beginTransform(*h, scenePos);
                // Also begin transform on source
                if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
                    VectorObject *src = canvas->sourceObject(m_selectedImage);
                    if (auto *srcImg = dynamic_cast<TransformableImageObject*>(src))
                        if (srcImg != m_selectedImage) srcImg->beginTransform(*h, scenePos);
                }
                return; // swallow — don't pass to scene
            }
        }

        // Check if any image body was clicked
        for (auto *img : currentFrameImages()) {
            if (img->hitTestImage(scenePos)) {
                if (m_selectedImage && m_selectedImage != img)
                    m_selectedImage->setSelected(false);
                m_selectedImage = img;
                m_selectedImage->setSelected(true);
                m_movingImage    = true;
                m_activeHandle   = std::nullopt;
                m_lastMouseScene = scenePos;
                viewport()->update();
                return;
            }
        }

        // Clicked empty space → deselect
        if (m_selectedImage) {
            m_selectedImage->setSelected(false);
            m_selectedImage = nullptr;
            viewport()->update();
        }
    } else if (!isSelectTool && m_selectedImage) {
        // User switched to a non-select tool: clear image selection so handles disappear
        m_selectedImage->setSelected(false);
        m_selectedImage = nullptr;
        viewport()->update();
    }
    // ── End image hit test ────────────────────────────────────────────────────

    // ── Forward to new-style tools (Lasso / MagicWand) ───────────────────────
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
        if (auto *tool = canvas->currentTool()) {
            QPointF scenePos = mapToScene(event->pos());
            tool->mousePressEvent(event, scenePos);
            if (event->isAccepted()) {
                viewport()->update();
                return;
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    // Finish image transform drag
    if (m_selectedImage && (m_activeHandle.has_value() || m_movingImage)) {
        // Also end transform on source object so the change persists
        if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
            VectorObject *src = canvas->sourceObject(m_selectedImage);
            if (auto *srcImg = dynamic_cast<TransformableImageObject*>(src)) {
                if (srcImg != m_selectedImage)
                    srcImg->endTransform();
            }
        }
        m_selectedImage->endTransform();
        m_activeHandle = std::nullopt;
        m_movingImage  = false;
        viewport()->update();
        return;
    }

    // ── Forward release to new-style tools (Lasso / MagicWand) ───────────────
    // This was the missing link — without it the lasso never received the
    // mouse-release that closes the loop.
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
        if (auto *tool = canvas->currentTool()) {
            QPointF scenePos = mapToScene(event->pos());
            tool->mouseReleaseEvent(event, scenePos);
            if (event->isAccepted()) {
                viewport()->update();
                return;
            }
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_panStartPos;
        m_panStartPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    // Image transform move / resize — only active when Select tool is chosen
    // FIX #19: Without this guard the transform cursor appears for ALL tools.
    bool isSelectToolNow = false;
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene()))
        if (auto *tool = canvas->currentTool())
            isSelectToolNow = (tool->type() == ToolType::Select);

    if (m_selectedImage && isSelectToolNow) {
        auto *canvas = qobject_cast<VectorCanvas*>(scene());
        // Resolve display clone → source so edits persist through refreshFrame
        TransformableImageObject *srcImg = nullptr;
        if (canvas) {
            VectorObject *src = canvas->sourceObject(m_selectedImage);
            srcImg = dynamic_cast<TransformableImageObject*>(src);
        }

        if (m_activeHandle.has_value()) {
            m_selectedImage->continueTransform(scenePos);
            if (srcImg && srcImg != m_selectedImage) {
                srcImg->continueTransform(scenePos);
            }
            viewport()->update();
            return;
        }
        if (m_movingImage) {
            QPointF delta = scenePos - m_lastMouseScene;
            m_selectedImage->setPosition(m_selectedImage->position() + delta);
            if (srcImg && srcImg != m_selectedImage) {
                srcImg->setPosition(srcImg->position() + delta);
            }
            m_lastMouseScene = scenePos;
            viewport()->update();
            return;
        }

        // Cursor hint for handles
        auto h = m_selectedImage->hitTestHandle(scenePos);
        if (h.has_value()) {
            switch (*h) {
            case HandleRole::Rotate:
                viewport()->setCursor(Qt::SizeAllCursor); break;
            case HandleRole::TopLeft:
            case HandleRole::BottomRight:
                viewport()->setCursor(Qt::SizeFDiagCursor); break;
            case HandleRole::TopRight:
            case HandleRole::BottomLeft:
                viewport()->setCursor(Qt::SizeBDiagCursor); break;
            case HandleRole::TopCenter:
            case HandleRole::BottomCenter:
                viewport()->setCursor(Qt::SizeVerCursor); break;
            case HandleRole::MiddleLeft:
            case HandleRole::MiddleRight:
                viewport()->setCursor(Qt::SizeHorCursor); break;
            }
            return;
        } else if (m_selectedImage->hitTestImage(scenePos)) {
            viewport()->setCursor(Qt::SizeAllCursor);
        } else {
            viewport()->setCursor(Qt::ArrowCursor);
        }
    }

    // Forward move to new-style tools
    if (auto *canvas = qobject_cast<VectorCanvas*>(scene())) {
        if (auto *tool = canvas->currentTool())
            tool->mouseMoveEvent(event, scenePos);
    }

    viewport()->update(); // repaint tool overlay (lasso preview)
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::contextMenuEvent(QContextMenuEvent *event)
{
    // Qt calls contextMenuEvent AFTER the right mouse button has been fully
    // released and all implicit mouse grabs have been cleared.  This is the
    // only correct place to open a QMenu::exec() — every other approach
    // (mousePressEvent, mouseReleaseEvent, QTimer::singleShot) still has
    // the native grab active, causing menu items to be unresponsive.
    auto *canvas = qobject_cast<VectorCanvas*>(scene());
    if (!canvas) return;

    QPointF scenePos = mapToScene(event->pos());
    emit canvas->contextMenuRequestedAt(event->globalPos(), scenePos);
    event->accept();
}
