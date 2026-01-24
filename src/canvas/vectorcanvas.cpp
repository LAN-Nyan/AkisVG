#include "vectorcanvas.h"
#include "core/project.h"
#include "core/layer.h"
#include "core/commands.h"
#include "objects/vectorobject.h"
#include "objects/imageobject.h"
#include "tools/tool.h"

#include <QPen>
#include <QBrush>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

VectorCanvas::VectorCanvas(Project *project, QUndoStack *undoStack, QObject *parent)
    : QGraphicsScene(parent)
    , m_project(project)
    , m_undoStack(undoStack)
    , m_currentTool(nullptr)
    , m_canvasBounds(nullptr)
{
    if (!m_project) return; // Critical safety check

    setSceneRect(0, 0, project->width(), project->height());
    setBackgroundBrush(QBrush(Qt::white));

    m_canvasBounds = addRect(sceneRect(), QPen(Qt::lightGray, 2));
    m_canvasBounds->setZValue(-1);

    setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    // 1. Frame change connection
    connect(project, &Project::currentFrameChanged, this, &VectorCanvas::onFrameChanged);

    // 2. Initial layer connection setup
    setupLayerConnections();

    // 3. Dynamic layer connection setup (Safe handling for AddLayerCommand)
    // Use Qt::QueuedConnection to ensure the Command finishes before we try to loop
    connect(project, &Project::layersChanged, this, [this]() {
        setupLayerConnections();
        refreshFrame();
    }, Qt::QueuedConnection);

    refreshFrame();
}

// Add this helper method to your vectorcanvas.cpp (and declare it in vectorcanvas.h)
void VectorCanvas::setupLayerConnections() {
    if (!m_project) return;

    for (Layer *layer : m_project->layers()) {
        if (!layer) continue; // Prevent Level 7 SIGABRT

        // Qt::UniqueConnection prevents multiple connections to the same signal
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

void VectorCanvas::setCurrentTool(Tool *tool)
{
    m_currentTool = tool;
}

void VectorCanvas::addObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

    // Add to scene
    addItem(obj);

    // Push undo command
    m_undoStack->push(new AddObjectCommand(obj, m_project->currentLayer(),
                                           m_project->currentFrame()));
}

void VectorCanvas::removeObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

    removeItem(obj);

    // Push undo command
    m_undoStack->push(new RemoveObjectCommand(obj, m_project->currentLayer(),
                                              m_project->currentFrame()));
}

void VectorCanvas::clearCurrentFrame()
{
    if (m_project->currentLayer()) {
        m_project->currentLayer()->clearFrame(m_project->currentFrame());
        refreshFrame();
    }
}

void VectorCanvas::refreshFrame()
{
    // Clear all objects from scene (except canvas bounds)
    for (QGraphicsItem *item : items()) {
        if (item != m_canvasBounds && dynamic_cast<VectorObject*>(item)) {
            removeItem(item);
        }
    }

    // Add objects from all visible layers at current frame
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

void VectorCanvas::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool) {
        m_currentTool->mousePressEvent(event, this);
    } else {
        QGraphicsScene::mousePressEvent(event);
    }
}

void VectorCanvas::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool) {
        m_currentTool->mouseMoveEvent(event, this);
    } else {
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void VectorCanvas::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool) {
        m_currentTool->mouseReleaseEvent(event, this);
    } else {
        QGraphicsScene::mouseReleaseEvent(event);
    }
}

void VectorCanvas::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-lumina-asset")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void VectorCanvas::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-lumina-asset")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void VectorCanvas::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()->hasFormat("application/x-lumina-asset")) {
        event->ignore();
        return;
    }

    // Get asset info from MIME data
    QString assetType = event->mimeData()->data("application/x-lumina-asset-type");
    QString assetPath = QString::fromUtf8(event->mimeData()->data("application/x-lumina-asset-path"));

    int typeInt = assetType.toInt();

    if (typeInt == 0) {  // Image asset
        ImageObject *imageObj = new ImageObject();
        imageObj->setImagePath(assetPath);
        imageObj->setPos(event->scenePos() - QPointF(imageObj->boundingRect().width() / 2,
                                                     imageObj->boundingRect().height() / 2));
        addObject(imageObj);

        event->acceptProposedAction();
    } else {
        // Audio assets go to timeline, not canvas
        event->ignore();
    }
}
