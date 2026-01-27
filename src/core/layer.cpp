#include "layer.h"
#include "frame.h"
#include "canvas/objects/vectorobject.h"

Layer::Layer(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_visible(true)
    , m_locked(false)
    , m_color(QColor(59, 130, 246))
    , m_opacity(1.0)
    , m_layerType(LayerType::Art)
{
}

Layer::~Layer()
{
    // Clean up cached Frame objects
    qDeleteAll(m_framCache);
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
        if (type == LayerType::Background) {
            setLocked(true);
        }
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

// Frame data management with extension support
QList<VectorObject*> Layer::objectsAtFrame(int frameNumber) const
{
    // Check if this frame is extended from a key frame
    int keyFrame = getKeyFrameFor(frameNumber);
    if (keyFrame != -1 && keyFrame != frameNumber) {
        // This is an extended frame, return objects from the key frame
        return m_frames.value(keyFrame, QList<VectorObject*>());
    }

    // Return objects from this frame directly
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

        // Also clear cached Frame
        if (m_framCache.contains(frameNumber)) {
            delete m_framCache.take(frameNumber);
        }

        emit modified();
    }
}

bool Layer::hasContentAtFrame(int frameNumber) const
{
    // Check if frame has actual content
    if (m_frames.contains(frameNumber) && !m_frames[frameNumber].isEmpty()) {
        return true;
    }

    // Check if this frame is extended from a key frame
    int keyFrame = getKeyFrameFor(frameNumber);
    if (keyFrame != -1 && keyFrame != frameNumber) {
        return m_frames.contains(keyFrame) && !m_frames[keyFrame].isEmpty();
    }

    return false;
}

// ============= Frame Extension Methods =============

void Layer::extendFrameTo(int fromFrame, int toFrame)
{
    if (fromFrame < 1 || toFrame < fromFrame) {
        return; // Invalid range
    }

    // Only allow extending frames that have actual content
    if (!m_frames.contains(fromFrame) || m_frames[fromFrame].isEmpty()) {
        return;
    }

    // Store the extension info
    m_frameExtensions[fromFrame] = FrameExtension(fromFrame, toFrame);
    emit modified();
}

void Layer::clearFrameExtension(int frame)
{
    if (m_frameExtensions.contains(frame)) {
        m_frameExtensions.remove(frame);
        emit modified();
    }
}

bool Layer::isFrameExtended(int frameNumber) const
{
    // Check if this frame is within any extension range
    for (auto it = m_frameExtensions.constBegin(); it != m_frameExtensions.constEnd(); ++it) {
        const FrameExtension &ext = it.value();
        if (frameNumber > ext.keyFrame && frameNumber <= ext.extendToFrame) {
            return true;
        }
    }
    return false;
}

int Layer::getKeyFrameFor(int frameNumber) const
{
    // If this frame has actual content, it IS the key frame
    if (m_frames.contains(frameNumber) && !m_frames[frameNumber].isEmpty()) {
        return frameNumber;
    }

    // Check if this frame is extended from a key frame
    for (auto it = m_frameExtensions.constBegin(); it != m_frameExtensions.constEnd(); ++it) {
        const FrameExtension &ext = it.value();
        if (frameNumber >= ext.keyFrame && frameNumber <= ext.extendToFrame) {
            return ext.keyFrame;
        }
    }

    return -1; // No key frame found
}

int Layer::getExtensionEnd(int frameNumber) const
{
    if (m_frameExtensions.contains(frameNumber)) {
        return m_frameExtensions[frameNumber].extendToFrame;
    }

    // Check if this frame is within an extension
    for (auto it = m_frameExtensions.constBegin(); it != m_frameExtensions.constEnd(); ++it) {
        const FrameExtension &ext = it.value();
        if (frameNumber >= ext.keyFrame && frameNumber <= ext.extendToFrame) {
            return ext.extendToFrame;
        }
    }

    return -1;
}

bool Layer::isKeyFrame(int frameNumber) const
{
    return m_frames.contains(frameNumber) && !m_frames[frameNumber].isEmpty();
}

// ============= COMPATIBILITY: Frame* interface =============

Frame* Layer::frameAt(int index)
{
    if (!m_framCache.contains(index)) {
        m_framCache[index] = new Frame(index, this);
    }

    Frame* frame = m_framCache[index];

    // Sync objects from m_frames to Frame
    frame->objects = m_frames.value(index, QList<VectorObject*>());

    return frame;
}

Frame* Layer::frameIfExists(int index) const
{
    if (m_framCache.contains(index)) {
        return m_framCache[index];
    }
    if (m_frames.contains(index)) {
        m_framCache[index] = new Frame(index, const_cast<Layer*>(this));
        m_framCache[index]->objects = m_frames[index];
        return m_framCache[index];
    }
    return nullptr;
}

const Frame* Layer::getActiveFrameAt(int index) const
{
    // Frame hold logic
    if (hasContentAtFrame(index)) {
        return frameIfExists(index);
    }

    // Find previous frame with content
    QList<int> frameNums = m_frames.keys();
    std::sort(frameNums.begin(), frameNums.end(), std::greater<int>());

    for (int frameNum : frameNums) {
        if (frameNum < index) {
            return frameIfExists(frameNum);
        }
    }

    return nullptr;
}
