#include "vectorcanvas.h"
#include "core/project.h"
#include "core/layer.h"
#include "objects/vectorobject.h"
#include "tools/tool.h"

#include <QPen>
#include <QBrush>

VectorCanvas::VectorCanvas(Project *project, QObject *parent)
    : QGraphicsScene(parent)
    , m_project(project)
    , m_currentTool(nullptr)
    , m_canvasBounds(nullptr)
{
    // Set scene rect to match project dimensions
    setSceneRect(0, 0, project->width(), project->height());
    setBackgroundBrush(QBrush(Qt::white));

    // Draw canvas bounds
    m_canvasBounds = addRect(sceneRect(), QPen(Qt::lightGray, 2));
    m_canvasBounds->setZValue(-1);

    // Enable BSP index for performance
    setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    // Connect to project signals
    connect(project, &Project::currentFrameChanged, this, &VectorCanvas::onFrameChanged);

    // Connect to layer visibility changes
    for (Layer *layer : project->layers()) {
        connect(layer, &Layer::visibilityChanged, this, [this]() { refreshFrame(); });
        connect(layer, &Layer::modified, this, [this]() { refreshFrame(); });
    }

    // Connect to layers changed to hook up new layers
    connect(project, &Project::layersChanged, this, [this]() {
        for (Layer *layer : m_project->layers()) {
            connect(layer, &Layer::visibilityChanged, this, [this]() { refreshFrame(); }, Qt::UniqueConnection);
            connect(layer, &Layer::modified, this, [this]() { refreshFrame(); }, Qt::UniqueConnection);
        }
        refreshFrame();
    });

    refreshFrame();
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

    // Add to current layer's current frame
    m_project->currentLayer()->addObjectToFrame(m_project->currentFrame(), obj);
}

void VectorCanvas::removeObject(VectorObject *obj)
{
    if (!obj || !m_project->currentLayer()) return;

    removeItem(obj);
    m_project->currentLayer()->removeObjectFromFrame(m_project->currentFrame(), obj);
    delete obj;
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
