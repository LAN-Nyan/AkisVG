#include "vectorcanvas.h"
#include "core/project.h"
#include "core/frame.h"
#include "core/layer.h"
#include "tools/tool.h"
#include "objects/vectorobject.h"
#include "objects/pathobject.h"
#include "objects/objectgroup.h"
#include "core/commands.h"

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
#include <QGraphicsRectItem>
#include <QTimer>

VectorCanvas::VectorCanvas(Project *project, QUndoStack *undoStack, QObject *parent)
    : QGraphicsScene(parent)
    , m_project(project)
    , m_undoStack(undoStack)
    , m_currentTool(nullptr)
    , m_onionSkinEnabled(true)
    , m_isDrawing(false)
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

void VectorCanvas::setupLayerConnections()
{
    if (!m_project) return;
    for (Layer *layer : m_project->layers()) {
        if (!layer || m_connectedLayers.contains(layer)) continue;
        m_connectedLayers.insert(layer);
        connect(layer, &Layer::visibilityChanged, this, [this](bool) { refreshFrame(); });
        connect(layer, &Layer::modified,          this, [this]()      { refreshFrame(); });
        connect(layer, &QObject::destroyed, this, [this, layer]() { m_connectedLayers.remove(layer); });
    }
}

void VectorCanvas::clearDisplay()
{
    if (m_liveDrawingItem) {
        if (m_liveDrawingItem->scene() == this)
            removeItem(m_liveDrawingItem);
        m_liveDrawingItem = nullptr;
    }
    for (VectorObject *item : m_displayItems) {
        m_cloneToSource.remove(item);
        if (item->scene() == this)
            removeItem(item);
        delete item;
    }
    m_displayItems.clear();
    m_cloneToSource.clear();
    m_connectedLayers.clear();
}

void VectorCanvas::refreshFrame()
{
    if (m_batchUpdating) return;

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

    // Tear down all previous display items.
    for (VectorObject *item : m_displayItems) {
        m_cloneToSource.remove(item);
        if (item->scene() == this)
            removeItem(item);
        delete item;
    }
    m_displayItems.clear();

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

    for (Layer *layer : m_project->layers()) {
        if (!layer->isVisible()) continue;
        addForFrame(layer, currentFrame, layer->opacity(), 1.0);
    }

    // SAFETY: After rebuilding all display clones, the live stroke must sit above
    // everything else. Clones no longer copy z-values, so they all land at z=0.
    // Re-asserting z=9999 here guarantees the active stroke is always on top,
    // regardless of how many refreshFrame() calls happen during a single stroke.
    if (m_isDrawing && m_liveDrawingItem) {
        m_liveDrawingItem->setZValue(9999);
    }
}

VectorObject* VectorCanvas::sourceObject(VectorObject *item) const
{
    return m_cloneToSource.value(item, item);
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

void VectorCanvas::setCurrentTool(Tool *tool) { m_currentTool = tool; }

void VectorCanvas::addObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

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
    if (event->button() == Qt::RightButton) {
        // Right-click context menu is handled by CanvasView::contextMenuEvent.
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Always forward to tool, not just when m_isDrawing.
        // SelectTool needs release to commit drag-move even when m_isDrawing=false.
        if (m_currentTool)
            m_currentTool->mouseReleaseEvent(event, this);
        m_isDrawing = false;
        // Reset the live item's z-value before refreshFrame so that once committed,
        // it stacks normally with all other objects (z=0 = layer insertion order).
        if (m_liveDrawingItem)
            m_liveDrawingItem->setZValue(0);
        // Call refreshFrame() BEFORE clearing m_liveDrawingItem.
        // refreshFrame evicts the live item via removeItem() when m_isDrawing is
        // false and m_liveDrawingItem is non-null. Without this, the item added
        // directly via addItem() stays in the scene forever (it was never in
        // m_displayItems so it was never cleaned up), causing every frame to
        // show the stroke that was drawn on frame 1.
        refreshFrame();
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
