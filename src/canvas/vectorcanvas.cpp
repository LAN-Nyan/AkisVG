#include "vectorcanvas.h"
#include "core/project.h"
#include "core/frame.h"
#include "core/layer.h"
#include "tools/tool.h"
#include "objects/vectorobject.h"
#include "objects/pathobject.h"
#include "core/commands.h"

#include <QPainter>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QBitmap>
#include <QPen>
#include <QDebug>


VectorCanvas::VectorCanvas(Project *project, QUndoStack *undoStack, QObject *parent)
    : QGraphicsScene(parent)
    , m_project(project)
    , m_undoStack(undoStack)
    , m_currentTool(nullptr)
    , m_onionSkinEnabled(true)
{
    if (!m_project) return;

    setSceneRect(0, 0, project->width(), project->height());
    setBackgroundBrush(QBrush(Qt::white));
    setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    connect(project, &Project::currentFrameChanged, this, &VectorCanvas::onFrameChanged);
    setupLayerConnections();

    connect(project, &Project::layersChanged, this, [this]() {
        setupLayerConnections();
        refreshFrame();
    }, Qt::QueuedConnection);

    refreshFrame();
}

void VectorCanvas::setupLayerConnections()
{
    if (!m_project) return;

    for (Layer *layer : m_project->layers()) {
        if (!layer) continue;

        connect(layer, &Layer::visibilityChanged, this, [this](bool) {
            refreshFrame();
        }, Qt::UniqueConnection);

        connect(layer, &Layer::modified, this, [this]() {
            refreshFrame();
        }, Qt::UniqueConnection);
    }
}

VectorCanvas::~VectorCanvas()
{
}

void VectorCanvas::refreshFrame()
{
    // Clear all objects from scene
    QList<QGraphicsItem*> toRemove;
    for (QGraphicsItem *item : items()) {
        if (dynamic_cast<VectorObject*>(item)) {
            toRemove.append(item);
        }
    }
    for (auto *item : toRemove) {
        removeItem(item);
    }

    if (!m_project) return;

    // Add objects from all visible layers - WORKING VERSION LOGIC
    int currentFrame = m_project->currentFrame();
    for (Layer *layer : m_project->layers()) {
        if (!layer->isVisible()) continue;

        for (VectorObject *obj : layer->objectsAtFrame(currentFrame)) {
            addItem(obj);
            obj->setOpacity(layer->opacity());
        }
    }
}

void VectorCanvas::onFrameChanged(int frame)
{
    Q_UNUSED(frame)
    refreshFrame();
}

void VectorCanvas::connectLayerSignals(Layer *layer)
{
    if (!layer) return;

    // Check if already connected
    if (m_connectedLayers.contains(layer)) {
        return;  // Already connected, skip
    }

    // Mark as connected
    m_connectedLayers.insert(layer);

    // Connect layer signals to force canvas update
    connect(layer, &Layer::modified, this, [this]() {
        qDebug() << "VectorCanvas: Layer modified signal received";
        invalidate();  // Force complete scene redraw
    });

    connect(layer, &Layer::visibilityChanged, this, [this](bool visible) {
        qDebug() << "VectorCanvas: Layer visibility changed to:" << visible;
        invalidate();  // Force complete scene redraw
    });

    // Also connect to layer destruction to remove from set
    connect(layer, &QObject::destroyed, this, [this, layer]() {
        m_connectedLayers.remove(layer);
    });

    qDebug() << "VectorCanvas: Connected signals for layer:" << layer->name();
}

void VectorCanvas::setOnionSkinEnabled(bool enabled)
{
    m_onionSkinEnabled = enabled;
    update();
}

QBrush VectorCanvas::getTextureBrush(ToolTexture texture, const QColor &color)
{
    QBrush brush(color);
    static const uchar grainBits[] = { 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44 };
    static const uchar chalkBits[] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };
    static const uchar canvasBits[] = { 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

    switch (texture) {
    case ToolTexture::Grainy:
        brush.setTexture(QBitmap::fromData(QSize(8, 8), grainBits));
        break;
    case ToolTexture::Chalk:
        brush.setTexture(QBitmap::fromData(QSize(8, 8), chalkBits));
        break;
    case ToolTexture::Canvas:
        brush.setTexture(QBitmap::fromData(QSize(8, 8), canvasBits));
        break;
    default:
        break;
    }
    return brush;
}

void VectorCanvas::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VectorCanvas::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) return;

    QString filePath = event->mimeData()->urls().first().toLocalFile();
    QPointF position = event->scenePos();
    QString suffix = QFileInfo(filePath).suffix().toLower();

    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
        suffix == "svg" || suffix == "bmp" || suffix == "webp") {
        emit referenceImageDropped(filePath, position);
    }
    else if (suffix == "mp3" || suffix == "wav" || suffix == "ogg" ||
             suffix == "flac" || suffix == "m4a") {
        emit audioDropped(filePath);
    }

    event->acceptProposedAction();
}

void VectorCanvas::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    event->acceptProposedAction();
}

void VectorCanvas::setCurrentTool(Tool *tool)
{
    m_currentTool = tool;
}

//void VectorCanvas::refreshFrame()
//{
    // Clear all graphic items from scene
   // clear();

    // DON'T reload strokes as scene items anymore - drawBackground handles all rendering
    // Just force a complete redraw
    //invalidate();
//}

//void VectorCanvas::onFrameChanged(int frame)
//{
   // Q_UNUSED(frame);
   // refreshFrame();
//}

//void VectorCanvas::setupLayerConnections()
//{
  //  if (!m_project) return;
  //  connect(m_project, &Project::currentFrameChanged,
     //       this, &VectorCanvas::onFrameChanged);
//}

void VectorCanvas::addObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

    // Add to scene for display
    addItem(obj);

    // CRITICAL FIX: Save to layer's frame data via undo command
    m_undoStack->push(new AddObjectCommand(obj, m_project->currentLayer(),
                                           m_project->currentFrame()));
}

void VectorCanvas::removeObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

    removeItem(obj);

    // Push undo command to remove from layer data
    m_undoStack->push(new RemoveObjectCommand(obj, m_project->currentLayer(),
                                              m_project->currentFrame()));
}

//void VectorCanvas::clearCurrentFrame()
//{
    //if (!m_project) return;
    //clear();
   // Frame &currentFrame = m_project->frame(m_project->currentFrame());
   // currentFrame.strokes.clear();
   // currentFrame.objects.clear();
   // update();
//}

void VectorCanvas::clearCurrentFrame()
{
    if (!m_project || !m_project->currentLayer()) return;

    clear();
    m_project->currentLayer()->clearFrame(m_project->currentFrame());
    update();
}

void VectorCanvas::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_currentTool) {
        // Let the tool handle the event first
        m_currentTool->mousePressEvent(event, this);

        // Only set drawing mode if the tool accepted the event
        // SelectTool will NOT accept, so it won't draw
        if (event->isAccepted()) {
            m_isDrawing = true;
        } else {
            // Let Qt's default selection handling work
            QGraphicsScene::mousePressEvent(event);
        }
    } else {
        QGraphicsScene::mousePressEvent(event);
    }
}

void VectorCanvas::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isDrawing && m_currentTool) {
        m_currentTool->mouseMoveEvent(event, this);
        update();
    } else {
        // Pass to scene for selection rectangle, etc.
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void VectorCanvas::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isDrawing && m_currentTool) {
            m_currentTool->mouseReleaseEvent(event, this);

            // Only save if we were actually drawing
            saveCurrentFrameStrokes();
        }
        m_isDrawing = false;
        update();
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void VectorCanvas::saveCurrentFrameStrokes()
{
    // No longer needed - objects saved via AddObjectCommand
}

//void VectorCanvas::saveCurrentFrameStrokes()
//{
  //  if (!m_project) return;

  //  Frame &currentFrame = m_project->frame(m_project->currentFrame());
   // currentFrame.strokes.clear();

   // for (QGraphicsItem *item : items()) {
     //   PathObject *pathObj = dynamic_cast<PathObject*>(item);
      //  if (pathObj) {
         //   VectorStroke stroke;
         //   stroke.path = pathObj->path();
         //   stroke.color = pathObj->strokeColor();
          //  stroke.width = pathObj->strokeWidth();
          //  stroke.texture = ToolTexture::Smooth;
//currentFrame.strokes.append(stroke);
      //  }
    //}
//}

QImage VectorCanvas::currentImage()
{
    QRectF bounds = sceneRect();
    if (bounds.isEmpty()) {
        bounds = QRectF(0, 0, m_project->width(), m_project->height());
    }

    QImage image(bounds.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    render(&painter, QRectF(), bounds);

    return image;
}

void VectorCanvas::updateCurrentImage(const QImage &image)
{
    Q_UNUSED(image);
    update();
}
