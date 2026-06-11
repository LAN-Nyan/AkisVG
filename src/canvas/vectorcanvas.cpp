#include "vectorcanvas.h"
#include "core/project.h"
#include "core/frame.h"
#include "core/layer.h"
#include "tools/tool.h"
#include "objects/vectorobject.h"
#include "objects/pathobject.h"
#include "objects/objectgroup.h"
#include "objects/shapeobject.h"
#include "core/commands.h"
#include "utils/debuglog.h"

#include <QPainter>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QBitmap>
#include <QPen>
#include <QDateTime>
#include <QGraphicsView>
#include <QUuid>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QTimer>

namespace {
QPainterPath objectClipShape(VectorObject *obj)
{
    if (!obj) return QPainterPath();

    if (auto *pathObj = dynamic_cast<PathObject *>(obj)) {
        if (!pathObj->path().isEmpty())
            return pathObj->mapToScene(pathObj->path());
    }

    if (auto *shape = dynamic_cast<ShapeObject *>(obj)) {
        QPainterPath p;
        const QRectF r = shape->rect();
        if (shape->shapeType() == ShapeObject::Ellipse)
            p.addEllipse(r);
        else
            p.addRect(r);
        return shape->mapToScene(p);
    }

    if (auto *group = dynamic_cast<ObjectGroup *>(obj)) {
        QPainterPath united;
        for (VectorObject *child : group->children())
            united = united.united(objectClipShape(child));
        if (!united.isEmpty())
            return united;
    }

    const QRectF r = obj->mapRectToScene(obj->boundingRect());
    if (r.isEmpty()) return QPainterPath();
    QPainterPath p;
    p.addRect(r);
    return p;
}

bool undoStackContainsAddForObject(const QUndoStack *stack, VectorObject *obj, int depth = 0)
{
    if (!stack || !obj || !stack->canUndo() || depth > 4)
        return false;
    const QUndoCommand *cmd = stack->command(stack->index() - 1 - depth);
    if (!cmd) return false;
    if (const auto *add = dynamic_cast<const AddObjectCommand *>(cmd)) {
        if (add->object() == obj) return true;
    }
    for (int i = 0; i < cmd->childCount(); ++i) {
        if (const auto *add = dynamic_cast<const AddObjectCommand *>(cmd->child(i))) {
            if (add->object() == obj) return true;
        }
    }
    return false;
}
} // namespace

VectorCanvas::VectorCanvas(Project *project, QUndoStack *undoStack, QObject *parent)
    : QGraphicsScene(parent)
    , m_project(project)
    , m_undoStack(undoStack)
    , m_currentTool(nullptr)
    , m_onionSkinEnabled(true)
    , m_isDrawing(false)
    , m_isCancelingDrawing(false)
{
    if (!m_project) return;

    setSceneRect(0, 0, project->width(), project->height());
    setBackgroundBrush(QBrush(Qt::white));
    setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    connect(project, &Project::currentFrameChanged, this, &VectorCanvas::onFrameChanged);
    connect(project, &Project::onionSkinSettingsChanged, this, &VectorCanvas::refreshFrame);
    setupLayerConnections();

    connect(project, &Project::layersChanged, this, [this]() {
        setupLayerConnections();
        refreshFrame();
    }, Qt::QueuedConnection);

    refreshFrame();
}

VectorCanvas::~VectorCanvas() {}

void VectorCanvas::rebindProject(Project *project, QUndoStack *undoStack)
{
    AKIS_LOG(Canvas, QStringLiteral("rebindProject project=%1 undo=%2")
                        .arg(reinterpret_cast<quintptr>(project), 0, 16)
                        .arg(reinterpret_cast<quintptr>(undoStack), 0, 16));
    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }
    for (Layer *layer : m_connectedLayers) {
        if (layer)
            disconnect(layer, nullptr, this, nullptr);
    }
    m_connectedLayers.clear();

    m_project = project;
    m_undoStack = undoStack;

    if (!m_project) {
        clearDisplay();
        return;
    }

    setSceneRect(0, 0, m_project->width(), m_project->height());
    connect(m_project, &Project::currentFrameChanged, this, &VectorCanvas::onFrameChanged);
    connect(m_project, &Project::onionSkinSettingsChanged, this, &VectorCanvas::refreshFrame);
    connect(m_project, &Project::layersChanged, this, [this]() {
        setupLayerConnections();
        refreshFrame();
    }, Qt::QueuedConnection);
    setupLayerConnections();
    refreshFrame();
}

void VectorCanvas::setupLayerConnections()
{
    if (!m_project) return;
    for (Layer *layer : m_project->layers()) {
        if (!layer || m_connectedLayers.contains(layer)) continue;
        m_connectedLayers.insert(layer);
        connect(layer, &Layer::visibilityChanged, this, [this](bool) { refreshFrame(); });
        connect(layer, &Layer::clipModeChanged,   this, [this](bool) { refreshFrame(); });
        connect(layer, &Layer::modified,          this, [this]()      { refreshFrame(); });
        connect(layer, &QObject::destroyed, this, [this, layer]() { m_connectedLayers.remove(layer); });
    }
}

void VectorCanvas::clearDisplay()
{
    AKIS_LOG(Canvas, QStringLiteral("clearDisplay"));
    m_isDrawing = false;
    m_isCancelingDrawing = false;

    if (m_liveDrawingItem) {
        if (m_liveDrawingItem->scene() == this)
            removeItem(m_liveDrawingItem);
        m_liveDrawingItem = nullptr;
    }

    for (QGraphicsRectItem *r : m_selectionOverlays) {
        if (r->scene() == this) removeItem(r);
        delete r;
    }
    m_selectionOverlays.clear();

    for (QGraphicsPathItem *clip : m_clipContainers) {
        if (clip->scene() == this) removeItem(clip);
        delete clip;
    }
    m_clipContainers.clear();

    for (VectorObject *item : m_displayItems) {
        if (item->parentItem())
            continue;
        m_cloneToSource.remove(item);
        if (item->scene() == this)
            removeItem(item);
        delete item;
    }
    m_displayItems.clear();
    m_cloneToSource.clear();
    m_connectedLayers.clear();
}


void VectorCanvas::cancelLiveDrawing()
{
    AKIS_LOG(Canvas, QStringLiteral("cancelLiveDrawing isDrawing=%1 live=%2")
                        .arg(m_isDrawing)
                        .arg(reinterpret_cast<quintptr>(m_liveDrawingItem), 0, 16));
    if (!m_isDrawing && !m_liveDrawingItem)
        return;

    m_isCancelingDrawing = true;
    m_isDrawing = false;

    VectorObject *live = m_liveDrawingItem;
    m_liveDrawingItem = nullptr;

    if (m_currentTool)
        m_currentTool->cancelDraw();

    if (live) {
        if (live->scene() == this)
            removeItem(live);

        // addObject() already pushed AddObjectCommand — the object is layer-owned.
        // Never delete it here; undo/remove via the command stack or we double-free.
        if (m_undoStack && m_project && m_project->currentLayer()) {
            if (m_undoStack->isActive())
                m_undoStack->endMacro();

            bool reverted = false;
            if (undoStackContainsAddForObject(m_undoStack, live)) {
                m_undoStack->undo();
                reverted = true;
            }
            if (!reverted) {
                m_undoStack->push(new RemoveObjectCommand(live, m_project->currentLayer(),
                                                          m_project->currentFrame()));
            }
        } else {
            delete live;
        }
    }

    refreshFrame();
    update();

    QTimer::singleShot(100, this, [this]() {
        m_isCancelingDrawing = false;
    });
}


void VectorCanvas::refreshFrame()
{
    if (m_batchUpdating) {
        AKIS_LOG(Canvas, QStringLiteral("refreshFrame skipped (batch update)"));
        return;
    }
    AKIS_LOG(Canvas, QStringLiteral("refreshFrame START frame=%1 drawing=%2 live=%3")
                        .arg(m_project ? m_project->currentFrame() : -1)
                        .arg(m_isDrawing)
                        .arg(reinterpret_cast<quintptr>(m_liveDrawingItem), 0, 16));

    // Clear selection overlays — SelectTool will re-add them after refreshFrame if needed.
    for (QGraphicsRectItem *r : m_selectionOverlays) {
        if (r->scene() == this) removeItem(r);
        delete r;
    }
    m_selectionOverlays.clear();

    // If not actively drawing, evict any stale live item — it reappears as a clone below.
    if (!m_isDrawing && m_liveDrawingItem) {
        if (m_liveDrawingItem->scene() == this)
            removeItem(m_liveDrawingItem);
        m_liveDrawingItem = nullptr;
    }

    // Notify listeners (e.g. CanvasView) that display clones are about to be
    // destroyed, so they can null any raw pointers to them before we delete.
    emit aboutToRefreshFrame();

    // Tear down clip containers (also deletes parented display clones).
    for (QGraphicsPathItem *clip : m_clipContainers) {
        if (clip->scene() == this)
            removeItem(clip);
        delete clip;
    }
    m_clipContainers.clear();

    // Tear down standalone display items (onion skin / unclipped).
    for (VectorObject *item : m_displayItems) {
        if (item->parentItem())
            continue;
        m_cloneToSource.remove(item);
        if (item->scene() == this)
            removeItem(item);
        delete item;
    }
    m_displayItems.clear();
    m_cloneToSource.clear();

    if (!m_project) return;

    int currentFrame = m_project->currentFrame();

    // Add display items for a frame.
    // objectsAtFrame returns raw layer-owned pointers for keyframes/extended frames,
    // but returns NEWLY ALLOCATED clones for interpolated in-between frames.
    // We must not clone the interpolated results again or the offset doubles.
    auto addForFrame = [&](Layer *layer, int frame, qreal opacity, qreal objOpacity) {
        bool isInBetween = layer->isInterpolated(frame);

        for (VectorObject *obj : layer->objectsAtFrame(frame)) {
            if (obj == m_liveDrawingItem) {
                // Live item is already in scene — but if it was a new clone from
                // objectsAtFrame (interpolated frame) we must still delete it to avoid leak.
                if (isInBetween) delete obj;
                continue;
            }
            // Interpolated frames: objectsAtFrame already gave us a fresh clone, use it directly.
            // Keyframes: objectsAtFrame gave us the raw owned object, clone it for display.
            VectorObject *display = isInBetween ? obj : obj->clone();
            display->setOpacity(opacity);
            display->setObjectOpacity(objOpacity);
            // Display clones are render-only; strip interactive flags so Qt's
            // scene drag-move logic never intercepts mouse events over filled
            // shapes or images while a drawing tool is active.
            display->setFlag(QGraphicsItem::ItemIsMovable,   false);
            display->setFlag(QGraphicsItem::ItemIsSelectable, false);
            display->setAcceptedMouseButtons(Qt::NoButton);
            // DeviceCoordinateCache caches at paint time — disable it on the
            // live path so incremental strokes show immediately without stale tiles.
            display->setCacheMode(QGraphicsItem::NoCache);
            if (!isInBetween) m_cloneToSource[display] = obj;
            addItem(display);
            m_displayItems.append(display);
        }
    };

    if (m_project->onionSkinEnabled()) {
        for (int i = 1; i <= m_project->onionSkinBefore(); ++i) {
            int f = currentFrame - i;
            if (f < 1) continue;
            qreal op = m_project->onionSkinOpacity() * (1.0 - (i - 1) * 0.3);
            if (op <= 0.05) continue;
            for (Layer *layer : m_project->layers())
                if (layer->isVisible()) addForFrame(layer, f, op * layer->opacity(), op);
        }
        for (int i = 1; i <= m_project->onionSkinAfter(); ++i) {
            int f = currentFrame + i;
            if (f > m_project->totalFrames()) continue;
            qreal op = m_project->onionSkinOpacity() * 0.6 * (1.0 - (i - 1) * 0.3);
            if (op <= 0.05) continue;
            for (Layer *layer : m_project->layers())
                if (layer->isVisible()) addForFrame(layer, f, op * 0.7 * layer->opacity(), op * 0.7);
        }
    }

    struct LayerDisplay {
        VectorObject *display = nullptr;
        VectorObject *source  = nullptr;
    };
    QMap<Layer *, QList<LayerDisplay>> currentDisplays;
    const QList<Layer *> layers = m_project->layers();

    for (Layer *layer : layers) {
        if (!layer->isVisible()) continue;
        const bool isInBetween = layer->isInterpolated(currentFrame);
        for (VectorObject *obj : layer->objectsAtFrame(currentFrame)) {
            if (!obj) {
                AKIS_LOG(Crash, QStringLiteral("refreshFrame: null object on layer '%1' frame %2")
                                    .arg(layer->name()).arg(currentFrame));
                continue;
            }
            if (obj == m_liveDrawingItem) {
                if (isInBetween) delete obj;
                continue;
            }
            VectorObject *display = isInBetween ? obj : obj->clone();
            if (!display) {
                AKIS_LOG(Crash, QStringLiteral("refreshFrame: clone failed for obj %1")
                                    .arg(reinterpret_cast<quintptr>(obj), 0, 16));
                continue;
            }
            display->setOpacity(layer->opacity());
            display->setObjectOpacity(1.0);
            display->setFlag(QGraphicsItem::ItemIsMovable, false);
            display->setFlag(QGraphicsItem::ItemIsSelectable, false);
            display->setAcceptedMouseButtons(Qt::NoButton);
            display->setCacheMode(QGraphicsItem::NoCache);
            LayerDisplay ld;
            ld.display = display;
            ld.source  = isInBetween ? nullptr : obj;
            currentDisplays[layer].append(ld);
        }
    }

    for (int i = 0; i < layers.size(); ++i) {
        Layer *layer = layers[i];
        if (!currentDisplays.contains(layer))
            continue;

        QGraphicsPathItem *clipHost = nullptr;
        if (layer->clipsToLayerBelow() && i > 0
            && layer->layerType() != LayerType::Audio) {
            Layer *below = layers[i - 1];
            if (below->layerType() != LayerType::Audio
                && currentDisplays.contains(below)) {
                QPainterPath clipPath;
                for (const LayerDisplay &bd : currentDisplays[below]) {
                    clipPath = clipPath.united(objectClipShape(bd.display));
                }
                if (!clipPath.isEmpty()) {
                    clipHost = new QGraphicsPathItem(clipPath);
                    clipHost->setPen(Qt::NoPen);
                    clipHost->setBrush(Qt::NoBrush);
                    clipHost->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
                    addItem(clipHost);
                    m_clipContainers.append(clipHost);
                }
            }
        }

        for (const LayerDisplay &ld : currentDisplays[layer]) {
            VectorObject *display = ld.display;
            if (clipHost)
                display->setParentItem(clipHost);
            else
                addItem(display);
            m_displayItems.append(display);
            if (ld.source)
                m_cloneToSource[display] = ld.source;
        }
    }

    // SAFETY: After rebuilding all display clones, the live stroke must sit above
    // everything else. Clones no longer copy z-values, so they all land at z=0.
    // Re-asserting z=9999 here guarantees the active stroke is always on top,
    // regardless of how many refreshFrame() calls happen during a single stroke.
    if (m_isDrawing && m_liveDrawingItem) {
        m_liveDrawingItem->setZValue(9999);
    }
    AKIS_LOG(Canvas, QStringLiteral("refreshFrame END displays=%1 clips=%2 clones=%3")
                        .arg(m_displayItems.size())
                        .arg(m_clipContainers.size())
                        .arg(m_cloneToSource.size()));
}

VectorObject* VectorCanvas::sourceObject(VectorObject *item) const
{
    return m_cloneToSource.value(item, item);
}

// Reverse of sourceObject(): given a layer-owned source, find its current display clone.
// Returns nullptr if no clone exists (e.g. object is on a different frame).
VectorObject* VectorCanvas::displayCloneFor(VectorObject *source) const
{
    for (auto it = m_cloneToSource.cbegin(); it != m_cloneToSource.cend(); ++it) {
        if (it.value() == source) return it.key();
    }
    return nullptr;
}

void VectorCanvas::showSelectionOverlays(const QList<VectorObject*> &sourceObjects)
{
    // Clear any existing overlays first.
    for (QGraphicsRectItem *r : m_selectionOverlays) {
        if (r->scene() == this) removeItem(r);
        delete r;
    }
    m_selectionOverlays.clear();

    // Draw a dashed bounding-box highlight around each selected source object.
    // We iterate the display items map to find the clone whose source matches.
    for (VectorObject *src : sourceObjects) {
        if (!src) continue;

        // Find the matching display clone to get the correct scene-space bounds.
        QRectF bounds;
        for (auto it = m_cloneToSource.begin(); it != m_cloneToSource.end(); ++it) {
            if (it.value() == src) {
                bounds = it.key()->mapRectToScene(it.key()->boundingRect());
                break;
            }
        }
        // Fallback: use source object's own bounding rect.
        if (bounds.isNull())
            bounds = src->mapRectToScene(src->boundingRect());
        if (bounds.isNull()) continue;

        QGraphicsRectItem *rect = new QGraphicsRectItem(bounds.adjusted(-3, -3, 3, 3));
        rect->setPen(QPen(QColor(100, 180, 255), 1.5, Qt::DashLine));
        rect->setBrush(QBrush(QColor(100, 180, 255, 18)));
        rect->setZValue(9998); // just below the live drawing item
        rect->setFlag(QGraphicsItem::ItemIsMovable, false);
        rect->setFlag(QGraphicsItem::ItemIsSelectable, false);
        rect->setAcceptedMouseButtons(Qt::NoButton);
        addItem(rect);
        m_selectionOverlays.append(rect);
    }
}

void VectorCanvas::onFrameChanged(int) { refreshFrame(); }

void VectorCanvas::connectLayerSignals(Layer *) {}  // handled in setupLayerConnections

void VectorCanvas::setOnionSkinEnabled(bool enabled)
{
    m_onionSkinEnabled = enabled;
    update();
}

QBrush VectorCanvas::getTextureBrush(ToolTexture texture, const QColor &color)
{
    QBrush brush(color);
    static const uchar grainBits[]  = { 0x11,0x44,0x11,0x44,0x11,0x44,0x11,0x44 };
    static const uchar chalkBits[]  = { 0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA };
    static const uchar canvasBits[] = { 0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00 };
    switch (texture) {
    case ToolTexture::Grainy: brush.setTexture(QBitmap::fromData(QSize(8,8), grainBits));  break;
    case ToolTexture::Chalk:  brush.setTexture(QBitmap::fromData(QSize(8,8), chalkBits));  break;
    case ToolTexture::Canvas: brush.setTexture(QBitmap::fromData(QSize(8,8), canvasBits)); break;
    default: break;
    }
    return brush;
}

void VectorCanvas::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void VectorCanvas::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) return;
    QString filePath = event->mimeData()->urls().first().toLocalFile();
    QPointF position = event->scenePos();
    QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
        suffix == "svg" || suffix == "bmp" || suffix == "webp")
        emit referenceImageDropped(filePath, position);
    else if (suffix == "mp3" || suffix == "wav" || suffix == "ogg" ||
             suffix == "flac" || suffix == "m4a")
        emit audioDropped(filePath);
    event->acceptProposedAction();
}

void VectorCanvas::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    event->acceptProposedAction();
}

void VectorCanvas::setCurrentTool(Tool *tool)
{
    AKIS_LOG(Tool, QStringLiteral("setCurrentTool %1 → %2 (drawing=%3)")
                        .arg(m_currentTool ? m_currentTool->name() : QStringLiteral("null"))
                        .arg(tool ? tool->name() : QStringLiteral("null"))
                        .arg(m_isDrawing));
    if ((m_isDrawing || m_liveDrawingItem) && tool != m_currentTool)
        cancelLiveDrawing();
    m_currentTool = tool;
}

void VectorCanvas::addPlacedObject(VectorObject *obj)
{
    AKIS_LOG(Canvas, QStringLiteral("addPlacedObject obj=%1")
                        .arg(reinterpret_cast<quintptr>(obj), 0, 16));
    if (!obj || !m_project->currentLayer()) {
        AKIS_LOG(Canvas, QStringLiteral("addPlacedObject ABORT — null obj or layer"));
        return;
    }

    obj->setFlag(QGraphicsItem::ItemIsMovable, false);
    obj->setFlag(QGraphicsItem::ItemIsSelectable, false);
    obj->setAcceptedMouseButtons(Qt::NoButton);
    obj->setCacheMode(QGraphicsItem::NoCache);
    m_undoStack->push(new AddObjectCommand(obj, m_project->currentLayer(),
                                           m_project->currentFrame()));
    refreshFrame();
}

void VectorCanvas::addObject(VectorObject *obj)
{
    AKIS_LOG(Canvas, QStringLiteral("addObject (live) obj=%1 frame=%2")
                        .arg(reinterpret_cast<quintptr>(obj), 0, 16)
                        .arg(m_project ? m_project->currentFrame() : -1));
    if (!obj || !m_project->currentLayer()) {
        AKIS_LOG(Canvas, QStringLiteral("addObject ABORT — null obj or layer"));
        return;
    }

    // Show the live object immediately for real-time drawing feedback.
    // Strip interactive flags — this is a drawing-in-progress item, not an
    // interactable scene object. Without this, Qt tries to drag-move it
    // on mouse-move events, competing with the tool's mouseMoveEvent.
    obj->setFlag(QGraphicsItem::ItemIsMovable,    false);
    obj->setFlag(QGraphicsItem::ItemIsSelectable, false);
    obj->setAcceptedMouseButtons(Qt::NoButton);
    obj->setCacheMode(QGraphicsItem::NoCache);
    // FIX #4: Set high z-value so the stroke being drawn is always visible
    // on top of images and filled objects.
    obj->setZValue(9999);
    m_liveDrawingItem = obj;
    addItem(obj);

    // Commit to layer via undo command. The modified signal fires refreshFrame,
    // which will clone this object — but skips adding it again since it's m_liveDrawingItem.
    m_undoStack->push(new AddObjectCommand(obj, m_project->currentLayer(),
                                           m_project->currentFrame()));
}

void VectorCanvas::removeObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

    // obj may be a display clone — resolve to the layer-owned source.
    VectorObject *src = sourceObject(obj);

    // Remove the display clone from scene (it will be deleted by refreshFrame cleanup).
    if (obj->scene() == this)
        removeItem(obj);

    m_undoStack->push(new RemoveObjectCommand(src, m_project->currentLayer(),
                                              m_project->currentFrame()));
}

void VectorCanvas::clearCurrentFrame()
{
    if (!m_project || !m_project->currentLayer()) return;
    // Clear display clones manually before calling clear() to avoid double-delete.
    for (VectorObject *item : m_displayItems) {
        m_cloneToSource.remove(item);
        if (item->scene() == this) removeItem(item);
        delete item;
    }
    m_displayItems.clear();
    m_liveDrawingItem = nullptr;
    m_project->currentLayer()->clearFrame(m_project->currentFrame());
    update();
}

void VectorCanvas::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    AKIS_LOG(Event, QStringLiteral("mousePress btn=%1 pos=(%2,%3) tool=%4")
                        .arg(static_cast<int>(event->button()))
                        .arg(event->scenePos().x(), 0, 'f', 1)
                        .arg(event->scenePos().y(), 0, 'f', 1)
                        .arg(m_currentTool ? m_currentTool->name() : QStringLiteral("none")));
    if (m_project && m_project->currentLayer() &&
        m_project->currentLayer()->layerType() == LayerType::Audio) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::RightButton) {
        // Right-click is handled by CanvasView::contextMenuEvent — swallow here.
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_currentTool) {
        m_isDrawing = true; // set before tool press so addObject->refreshFrame sees it
        m_currentTool->mousePressEvent(event, this);
        if (!event->isAccepted()) {
            m_isDrawing = false;
            QGraphicsScene::mousePressEvent(event);
        }
    } else {
        QGraphicsScene::mousePressEvent(event);
    }
}

void VectorCanvas::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool) {
        // Always forward to the current tool, not just during m_isDrawing.
        // SelectTool needs this so drag-move works even when m_isDrawing=false.
        m_currentTool->mouseMoveEvent(event, this);
        if (event->isAccepted()) {
            update();
            return;
        }
    }
    if (m_isDrawing && m_currentTool) {
        update();
    } else {
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void VectorCanvas::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    AKIS_LOG(Event, QStringLiteral("mouseRelease btn=%1 canceling=%2 drawing=%3")
                        .arg(static_cast<int>(event->button()))
                        .arg(m_isCancelingDrawing)
                        .arg(m_isDrawing));
    if (event->button() == Qt::RightButton) {
        // Right-click context menu is handled by CanvasView::contextMenuEvent.
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // If we're in the process of canceling a drawing, ignore this event
        if (m_isCancelingDrawing) {
            event->accept();
            return;
        }

        // If we're not drawing or don't have a live drawing item, just forward to the base class
        if (!m_isDrawing || !m_liveDrawingItem) {
            QGraphicsScene::mouseReleaseEvent(event);
            return;
        }

        // Always forward to tool, not just when m_isDrawing.
        // SelectTool needs release to commit drag-move even when m_isDrawing=false.
        if (m_currentTool) {
            m_currentTool->mouseReleaseEvent(event, this);
        }

        // Snapshot before toggling m_isDrawing — refreshFrame() nulls m_liveDrawingItem
        // when it evicts the live stroke from the scene.
        VectorObject *wasLive = m_liveDrawingItem;
        m_isDrawing = false;

        if (wasLive && wasLive->scene() == this) {
            // Once committed, display clones use z=0 (layer order); live preview used 9999.
            wasLive->setZValue(0);
        }
        if (wasLive) {
            // refreshFrame() only removeItem()s the live stroke at the top when
            // !m_isDrawing && m_liveDrawingItem. If we called it while m_isDrawing was
            // still true, the layer-owned item stayed on the scene (never in
            // m_displayItems) and appeared on every frame after scrubbing.
            refreshFrame();
        }

        m_liveDrawingItem = nullptr;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void VectorCanvas::saveCurrentFrameStrokes() {}

QImage VectorCanvas::currentImage()
{
    QRectF bounds = sceneRect();
    if (bounds.isEmpty()) bounds = QRectF(0, 0, m_project->width(), m_project->height());
    QImage image(bounds.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    render(&painter, QRectF(), bounds);
    return image;
}

void VectorCanvas::updateCurrentImage(const QImage &) { update(); }

void VectorCanvas::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);
}

void VectorCanvas::enterInterpolationMode()
{
    if (m_isInterpolating) return;
    m_isInterpolating = true;
    emit interpolationModeChanged(true);
    update();
}

void VectorCanvas::exitInterpolationMode()
{
    if (!m_isInterpolating) return;
    m_isInterpolating = false;
    emit interpolationModeChanged(false);
    update();
}

ObjectGroup* VectorCanvas::groupSelectedObjects(const QString &name)
{
    return groupObjects({}, name);   // fall through to scene selectedItems
}

ObjectGroup* VectorCanvas::groupObjects(const QList<VectorObject*> &sourceObjects,
                                        const QString &name)
{
    // Build the list: prefer caller-supplied sources; fall back to Qt scene selection.
    QList<VectorObject*> objs = sourceObjects;
    if (objs.isEmpty()) {
        for (QGraphicsItem *item : selectedItems()) {
            VectorObject *vo = dynamic_cast<VectorObject*>(item);
            if (vo) objs.append(sourceObject(vo));
        }
    }
    if (objs.isEmpty()) return nullptr;

    QString groupName = name.isEmpty()
                            ? QString("Group %1").arg(QDateTime::currentMSecsSinceEpoch() % 10000) : name;

    ObjectGroup *group = new ObjectGroup(groupName);

    QRectF united;
    for (VectorObject *obj : objs) {
        QRectF r = obj->mapRectToScene(obj->boundingRect());
        united = united.isNull() ? r : united.united(r);
    }
    group->setPos(united.topLeft());

    Layer *layer = m_project ? m_project->currentLayer() : nullptr;
    int frame = m_project ? m_project->currentFrame() : 1;

    beginBatchUpdate();

    for (VectorObject *obj : objs) {
        QPointF scenePos = obj->scenePos();
        if (layer) layer->removeObjectFromFrame(frame, obj);
        obj->setPos(scenePos - united.topLeft());
        group->addChild(obj);
    }

    if (layer) layer->addObjectToFrame(frame, group);

    endBatchUpdate();

    emit objectGroupCreated(group);
    return group;
}

void VectorCanvas::ungroupSelected()
{
    QList<QGraphicsItem*> sel = selectedItems();
    Layer *layer = m_project ? m_project->currentLayer() : nullptr;
    int frame = m_project ? m_project->currentFrame() : 1;

    for (QGraphicsItem *item : sel) {
        // Check if this is a symbol instance
        SymbolInstance *instance = dynamic_cast<SymbolInstance*>(sourceObject(dynamic_cast<VectorObject*>(item)));
        if (instance) {
            // Convert symbol instance back to a regular group
            ObjectGroup *group = new ObjectGroup(instance->groupName());
            group->setPos(instance->pos());

            // Move all children from the instance to the group
            for (VectorObject *child : instance->ObjectGroup::children()) {
                instance->ObjectGroup::removeChild(child);
                child->setPos(child->pos());
                group->addChild(child);
            }

            // Replace the instance with the group in the layer
            if (layer) {
                layer->removeObjectFromFrame(frame, instance);
                layer->addObjectToFrame(frame, group);
            }

            delete instance;
            continue;
        }

        // Handle regular groups
        ObjectGroup *group = dynamic_cast<ObjectGroup*>(sourceObject(dynamic_cast<VectorObject*>(item)));
        if (!group) continue;
        QPointF groupPos = group->scenePos();
        for (VectorObject *child : group->children()) {
            group->removeChild(child);
            child->setPos(groupPos + child->pos());
            if (layer) layer->addObjectToFrame(frame, child);
        }
        if (layer) layer->removeObjectFromFrame(frame, group);
        delete group;
    }
}

// =============================================================================
// SymbolMaster Implementation
// =============================================================================

SymbolMaster::SymbolMaster(const QString &name, QGraphicsItem *parent)
    : ObjectGroup(name, parent)
    , m_uuid(QUuid::createUuid())
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
}

SymbolMaster::~SymbolMaster()
{
    for (SymbolInstance *instance : m_instances) {
        if (instance) instance->setMaster(nullptr);
    }
}

void SymbolMaster::setName(const QString &name)
{
    ObjectGroup::setGroupName(name);
    updateAllInstances();
}

void SymbolMaster::addInstance(SymbolInstance *instance)
{
    if (!instance || m_instances.contains(instance)) return;
    m_instances.append(instance);
    emit instanceAdded(instance);
}

void SymbolMaster::removeInstance(SymbolInstance *instance)
{
    if (!instance) return;
    m_instances.removeAll(instance);
    emit instanceRemoved(instance);
}

void SymbolMaster::updateAllInstances()
{
    for (SymbolInstance *instance : m_instances) {
        if (instance) instance->updateFromMaster();
    }
    emit masterModified();
}

VectorObject* SymbolMaster::clone() const
{
    return new SymbolInstance(const_cast<SymbolMaster*>(this), nullptr);
}

QPixmap SymbolMaster::thumbnail(int size) const
{
    if (children().isEmpty()) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(QColor(100, 180, 255, 128)));
        painter.setPen(QPen(QColor(100, 180, 255), 1));
        painter.drawRect(2, 2, size - 4, size - 4);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, "S");
        return pixmap;
    }
    return ObjectGroup::thumbnail(size);
}

void SymbolMaster::setArtworkChildren(const QList<VectorObject *> &objects)
{
    const QList<VectorObject *> old = children();
    for (VectorObject *c : old) {
        removeChild(c);
        delete c;
    }
    for (VectorObject *o : objects) {
        if (o)
            addChild(o);
    }
    updateAllInstances();
}

// =============================================================================
// Symbol Master Creation
// =============================================================================

SymbolMaster* VectorCanvas::createMasterSymbol(const QList<VectorObject*> &objects, const QString &name)
{
    if (objects.isEmpty() || !m_project || !m_project->currentLayer())
        return nullptr;

    Layer *layer = m_project->currentLayer();
    const int frame = m_project->currentFrame();

    QRectF united;
    for (VectorObject *obj : objects) {
        if (!obj) continue;
        QRectF r = obj->mapRectToScene(obj->boundingRect());
        united = united.isNull() ? r : united.united(r);
    }
    if (united.isNull()) return nullptr;

    QString masterName = name.isEmpty()
        ? QString("Symbol %1").arg(QDateTime::currentMSecsSinceEpoch() % 10000)
        : name;
    SymbolMaster *master = new SymbolMaster(masterName);

    beginBatchUpdate();
    for (VectorObject *obj : objects) {
        if (!obj) continue;
        layer->removeObjectFromFrame(frame, obj);
        QPointF scenePos = obj->scenePos();
        obj->setPos(scenePos - united.topLeft());
        master->addChild(obj);
    }
    SymbolInstance *inst = new SymbolInstance(master);
    inst->setPos(united.topLeft());
    layer->addObjectToFrame(frame, inst);
    endBatchUpdate();

    m_project->addMasterSymbol(master);
    refreshFrame();
    return master;
}

SymbolInstance* VectorCanvas::createSymbolInstance(SymbolMaster *master)
{
    return master ? new SymbolInstance(master) : nullptr;
}

void VectorCanvas::updateSymbolInstances(SymbolMaster *master)
{
    if (master)
        master->updateAllInstances();
}

QList<SymbolMaster*> VectorCanvas::allMasterSymbols() const
{
    return m_project ? m_project->masterSymbols() : QList<SymbolMaster*>();
}

void VectorCanvas::removeMasterSymbol(SymbolMaster *master)
{
    if (m_project)
        m_project->removeMasterSymbol(master);
}

// =============================================================================
// SymbolInstance Implementation
// =============================================================================

SymbolInstance::SymbolInstance(SymbolMaster *master, QGraphicsItem *parent)
    : ObjectGroup(master ? master->groupName() : "Symbol Instance", parent)
    , m_master(master)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    if (m_master) {
        m_master->addInstance(this);
        updateFromMaster();
    }
}
SymbolInstance::~SymbolInstance()
{
    if (m_master) m_master->removeInstance(this);
}

void SymbolInstance::setName(const QString &name)
{
    ObjectGroup::setGroupName(name);
}

void SymbolInstance::setMaster(SymbolMaster *master)
{
    if (m_master) m_master->removeInstance(this);
    m_master = master;
    if (m_master) {
        m_master->addInstance(this);
        updateFromMaster();
    }
    setName(m_master ? m_master->groupName() : "Symbol Instance");
}

VectorObject* SymbolInstance::clone() const
{
    if (m_master) return new SymbolInstance(m_master, nullptr);
    return ObjectGroup::clone();
}

void SymbolInstance::updateFromMaster()
{
    if (!m_master) return;
    // Clear existing children
    for (VectorObject *child : ObjectGroup::children()) {
        ObjectGroup::removeChild(child);
        delete child;
    }
    // Clone children from master
    for (VectorObject *masterChild : m_master->children()) {
        VectorObject *childClone = masterChild->clone();
        childClone->setPos(masterChild->pos());
        ObjectGroup::addChild(childClone);
    }
    ObjectGroup::setGroupName(m_master->groupName());
    update();
}