#include "layer.h"
#include "canvas/objects/vectorobject.h"

Layer::Layer(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_visible(true)
    , m_locked(false)
    , m_color(QColor(59, 130, 246)) // Blue
    , m_opacity(1.0)
    , m_layerType(LayerType::Art)
{
}

Layer::~Layer()
{
    // Clean up all objects in all frames
    for (auto &frameObjects : m_frames) {
        qDeleteAll(frameObjects);
    }
}

void Layer::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit modified();
    }
}

void Layer::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        emit visibilityChanged(visible);
        emit modified();
    }
}

void Layer::setLocked(bool locked)
{
    if (m_locked != locked) {
        m_locked = locked;
        emit lockedChanged(locked);
        emit modified();
    }
}

void Layer::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit modified();
    }
}

void Layer::setOpacity(qreal opacity)
{
    if (m_opacity != opacity) {
        m_opacity = qBound(0.0, opacity, 1.0);
        emit modified();
    }
}

void Layer::setLayerType(LayerType type)
{
    if (m_layerType != type) {
        m_layerType = type;

        // Auto-lock background layers
        if (type == LayerType::Background) {
            setLocked(true);
        }

        // Set reference layers to 50% opacity
        if (type == LayerType::Reference) {
            setOpacity(0.5);
        }

        emit typeChanged(type);
        emit modified();
    }
}

QString Layer::layerTypeString() const
{
    switch (m_layerType) {
    case LayerType::Art: return "Art";
    case LayerType::Background: return "Background";
    case LayerType::Audio: return "Audio";
    case LayerType::Reference: return "Reference";
    }
    return "Unknown";
}

QList<VectorObject*> Layer::objectsAtFrame(int frameNumber) const
{
    return m_frames.value(frameNumber, QList<VectorObject*>());
}

void Layer::addObjectToFrame(int frameNumber, VectorObject *obj)
{
    if (obj) {
        m_frames[frameNumber].append(obj);
        emit modified();
    }
}

void Layer::removeObjectFromFrame(int frameNumber, VectorObject *obj)
{
    if (m_frames.contains(frameNumber)) {
        m_frames[frameNumber].removeOne(obj);
        if (m_frames[frameNumber].isEmpty()) {
            m_frames.remove(frameNumber);
        }
        emit modified();
    }
}

void Layer::clearFrame(int frameNumber)
{
    if (m_frames.contains(frameNumber)) {
        qDeleteAll(m_frames[frameNumber]);
        m_frames.remove(frameNumber);
        emit modified();
    }
}

bool Layer::hasContentAtFrame(int frameNumber) const
{
    return m_frames.contains(frameNumber) && !m_frames[frameNumber].isEmpty();
}

