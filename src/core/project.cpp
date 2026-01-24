#include "project.h"
#include "layer.h"

Project::Project(QObject *parent)
    : QObject(parent)
    , m_name("Untitled Project")
    , m_width(1920)
    , m_height(1080)
    , m_fps(24)
    , m_currentFrame(1)
    , m_totalFrames(100)
    , m_currentLayerIndex(0)
{
    // Create default layer
    addLayer("Layer 1");
}

Project::~Project()
{
    qDeleteAll(m_layers);
}

void Project::createNew(int width, int height, int fps)
{
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_currentFrame = 1;
    m_totalFrames = 100;

    // Clear existing layers
    qDeleteAll(m_layers);
    m_layers.clear();

    // Create default layer
    addLayer("Layer 1");
    m_currentLayerIndex = 0;

    emit modified();
    emit layersChanged();
}

void Project::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit modified();
    }
}

void Project::setWidth(int width)
{
    if (m_width != width) {
        m_width = width;
        emit modified();
    }
}

void Project::setHeight(int height)
{
    if (m_height != height) {
        m_height = height;
        emit modified();
    }
}

void Project::setFps(int fps)
{
    if (m_fps != fps) {
        m_fps = fps;
        emit modified();
    }
}

void Project::setCurrentFrame(int frame)
{
    if (frame >= 1 && frame <= m_totalFrames && frame != m_currentFrame) {
        m_currentFrame = frame;
        emit currentFrameChanged(frame);
    }
}

void Project::setTotalFrames(int frames)
{
    if (frames > 0 && frames != m_totalFrames) {
        m_totalFrames = frames;
        if (m_currentFrame > frames) {
            setCurrentFrame(frames);
        }
        emit modified();
    }
}

Layer* Project::currentLayer() const
{
    if (m_currentLayerIndex >= 0 && m_currentLayerIndex < m_layers.size()) {
        return m_layers[m_currentLayerIndex];
    }
    return nullptr;
}

void Project::setCurrentLayer(int index)
{
    if (index >= 0 && index < m_layers.size() && index != m_currentLayerIndex) {
        m_currentLayerIndex = index;
        emit currentLayerChanged(m_layers[index]);
    }
}

void Project::addLayer(const QString &name)
{
    Layer *layer = new Layer(name, this);
    m_layers.append(layer);
    emit layersChanged();
    emit modified();
}

void Project::removeLayer(int index)
{
    if (index >= 0 && index < m_layers.size() && m_layers.size() > 1) {
        Layer *layer = m_layers.takeAt(index);
        delete layer;

        if (m_currentLayerIndex >= m_layers.size()) {
            m_currentLayerIndex = m_layers.size() - 1;
        }

        emit layersChanged();
        emit modified();
    }
}

void Project::moveLayer(int fromIndex, int toIndex)
{
    if (fromIndex >= 0 && fromIndex < m_layers.size() &&
        toIndex >= 0 && toIndex < m_layers.size() &&
        fromIndex != toIndex) {

        m_layers.move(fromIndex, toIndex);

        if (m_currentLayerIndex == fromIndex) {
            m_currentLayerIndex = toIndex;
        }

        emit layersChanged();
        emit modified();
    }
}

Layer* Project::layerAt(int index) const
{
    if (index >= 0 && index < m_layers.size()) {
        return m_layers[index];
    }
    return nullptr;
}
