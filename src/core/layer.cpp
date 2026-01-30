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
    case LayerType::Interpolation: return "Interpolation";
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
        // CRITICAL FIX: Check if this frame is extended from a key frame
        int keyFrame = getKeyFrameFor(frameNumber);

        // If we're adding to an extended frame (not the key frame itself)
        if (keyFrame != -1 && keyFrame != frameNumber) {
            // Break the extension - this frame becomes its own key frame
            // We need to:
            // 1. Copy all objects from the key frame to this frame
            // 2. Clear any extension that includes this frame
            // 3. If the extension goes beyond this frame, create a new extension from this frame

            // Get the extension info before we modify it
            int extensionEnd = getExtensionEnd(frameNumber);

            // Copy objects from key frame to make this an independent frame
            QList<VectorObject*> keyFrameObjects = m_frames.value(keyFrame, QList<VectorObject*>());
            for (VectorObject *keyObj : keyFrameObjects) {
                // Clone the object so this frame has its own copy
                VectorObject *clonedObj = keyObj->clone();
                m_frames[frameNumber].append(clonedObj);
            }

            // Clear the old extension from the key frame
            clearFrameExtension(keyFrame);

            // If the key frame was extended beyond this frame, re-extend it to just before this frame
            if (keyFrame < frameNumber - 1) {
                extendFrameTo(keyFrame, frameNumber - 1);
            }

            // If there were frames after this one in the old extension, extend from this frame
            if (extensionEnd > frameNumber) {
                extendFrameTo(frameNumber, extensionEnd);
            }
        }

        // Now add the new object to this frame
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

// ============= INTERPOLATION / TWEEN Methods =============

void Layer::setInterpolation(int startFrame, int endFrame, const QString &easingType)
{
    if (startFrame < 1 || endFrame <= startFrame) {
        return; // Invalid range
    }

    // Both start and end frames must be keyframes with content
    if (!isKeyFrame(startFrame) || !isKeyFrame(endFrame)) {
        return;
    }

    // Store the interpolation info
    m_interpolations[startFrame] = FrameInterpolation(startFrame, endFrame, easingType);
    emit modified();
}

void Layer::clearInterpolation(int startFrame)
{
    if (m_interpolations.contains(startFrame)) {
        m_interpolations.remove(startFrame);
        emit modified();
    }
}

bool Layer::isInterpolated(int frameNumber) const
{
    // Check if this frame is within any interpolation range
    for (auto it = m_interpolations.constBegin(); it != m_interpolations.constEnd(); ++it) {
        const FrameInterpolation &interp = it.value();
        if (frameNumber > interp.startFrame && frameNumber < interp.endFrame) {
            return true;
        }
    }
    return false;
}

bool Layer::isInterpolationKeyFrame(int frameNumber) const
{
    // Check if this frame is a start or end of an interpolation
    for (auto it = m_interpolations.constBegin(); it != m_interpolations.constEnd(); ++it) {
        const FrameInterpolation &interp = it.value();
        if (frameNumber == interp.startFrame || frameNumber == interp.endFrame) {
            return true;
        }
    }
    return false;
}

FrameInterpolation Layer::getInterpolationFor(int frameNumber) const
{
    // Find the interpolation that contains this frame
    for (auto it = m_interpolations.constBegin(); it != m_interpolations.constEnd(); ++it) {
        const FrameInterpolation &interp = it.value();
        if (frameNumber >= interp.startFrame && frameNumber <= interp.endFrame) {
            return interp;
        }
    }
    return FrameInterpolation(); // Return empty if not found
}

qreal Layer::calculateEasing(qreal t, const QString &easingType) const
{
    // t should be between 0 and 1
    if (easingType == "easeIn") {
        return t * t; // Quadratic ease in
    } else if (easingType == "easeOut") {
        return 1.0 - (1.0 - t) * (1.0 - t); // Quadratic ease out
    } else if (easingType == "easeInOut") {
        if (t < 0.5) {
            return 2.0 * t * t;
        } else {
            return 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
        }
    }
    // Default: linear
    return t;
}

// ============= AUDIO Layer Methods =============

void Layer::setAudioData(const AudioData &audio)
{
    m_audioData = audio;

    // When audio is set, change layer type to Audio
    if (!audio.filePath.isEmpty()) {
        setLayerType(LayerType::Audio);
    }

    emit modified();
}

void Layer::clearAudio()
{
    m_audioData = AudioData();
    emit modified();
}

// ============= MAKE KEYFRAME Method =============

void Layer::makeKeyFrame(int frameNumber)
{
    // If already a keyframe, nothing to do
    if (isKeyFrame(frameNumber)) {
        return;
    }

    // Get the content that should be at this frame
    QList<VectorObject*> currentObjects = objectsAtFrame(frameNumber);

    // Clone all objects to create an independent keyframe
    QList<VectorObject*> newObjects;
    for (VectorObject *obj : currentObjects) {
        VectorObject *cloned = obj->clone();
        newObjects.append(cloned);
    }

    // Clear any extension or interpolation that includes this frame
    int keyFrame = getKeyFrameFor(frameNumber);
    if (keyFrame != -1 && keyFrame != frameNumber) {
        // This was an extended frame
        int extensionEnd = getExtensionEnd(frameNumber);
        clearFrameExtension(keyFrame);

        // Re-extend key frame to just before this frame
        if (keyFrame < frameNumber - 1) {
            extendFrameTo(keyFrame, frameNumber - 1);
        }

        // Extend from this new keyframe if needed
        if (extensionEnd > frameNumber) {
            extendFrameTo(frameNumber, extensionEnd);
        }
    }

    // Check for interpolation
    for (auto it = m_interpolations.begin(); it != m_interpolations.end(); ) {
        FrameInterpolation &interp = it.value();
        if (frameNumber > interp.startFrame && frameNumber < interp.endFrame) {
            // Split the interpolation
            int oldEnd = interp.endFrame;
            QString easing = interp.easingType;

            // Clear old interpolation
            it = m_interpolations.erase(it);

            // Create two new interpolations
            setInterpolation(interp.startFrame, frameNumber, easing);
            setInterpolation(frameNumber, oldEnd, easing);
        } else {
            ++it;
        }
    }

    // Add the cloned objects to this frame
    m_frames[frameNumber] = newObjects;

    emit modified();
}

// ============= INTERPOLATION Layer Methods =============

void Layer::addInterpolationKeyframe(int frameNumber, const InterpolationKeyframe &keyframe)
{
    if (m_layerType != LayerType::Interpolation) {
        setLayerType(LayerType::Interpolation);
    }

    m_interpKeyframes[frameNumber] = keyframe;
    emit modified();
}

void Layer::removeInterpolationKeyframe(int frameNumber)
{
    m_interpKeyframes.remove(frameNumber);
    emit modified();
}

bool Layer::hasInterpolationKeyframe(int frameNumber) const
{
    return m_interpKeyframes.contains(frameNumber);
}

QList<int> Layer::getInterpolationKeyframes() const
{
    return m_interpKeyframes.keys();
}

InterpolationKeyframe Layer::getInterpolatedFrame(int frameNumber) const
{
    // If exact keyframe exists, return it
    if (m_interpKeyframes.contains(frameNumber)) {
        return m_interpKeyframes[frameNumber];
    }

    QList<int> keyframes = m_interpKeyframes.keys();
    if (keyframes.isEmpty()) {
        return InterpolationKeyframe();
    }

    std::sort(keyframes.begin(), keyframes.end());

    const InterpolationKeyframe *startKf = nullptr;
    const InterpolationKeyframe *endKf = nullptr;
    int startFrame = -1;
    int endFrame = -1;

    for (int kf : keyframes) {
        if (kf < frameNumber) {
            startFrame = kf;
            // FIX: Use find() to get a pointer to the actual data in the map
            startKf = &m_interpKeyframes.find(kf).value();
        } else if (kf > frameNumber && endFrame == -1) {
            endFrame = kf;
            // FIX: Use find() here as well
            endKf = &m_interpKeyframes.find(kf).value();
            break;
        }
    }

    if (!startKf || !endKf) {
        if (startKf) return *startKf;
        if (endKf) return *endKf;
        return InterpolationKeyframe();
    }

    int totalFrames = endFrame - startFrame;
    qreal t = static_cast<qreal>(frameNumber - startFrame) / totalFrames;
    t = calculateEasing(t, startKf->easingType);

    InterpolationKeyframe result;
    result.frameNumber = frameNumber;
    result.position = startKf->position + (endKf->position - startKf->position) * t;
    result.rotation = startKf->rotation + (endKf->rotation - startKf->rotation) * t;
    result.scale = startKf->scale + (endKf->scale - startKf->scale) * t;
    result.opacity = startKf->opacity + (endKf->opacity - startKf->opacity) * t;
    result.easingType = startKf->easingType;

    return result;
}

QList<VectorObject*> Layer::getInterpolatedObjects(int frameNumber) const
{
    if (m_layerType != LayerType::Interpolation) {
        return objectsAtFrame(frameNumber);
    }

    InterpolationKeyframe kf = getInterpolatedFrame(frameNumber);

    // Get the base objects from the nearest keyframe
    QList<int> keyframes = m_interpKeyframes.keys();
    if (keyframes.isEmpty()) {
        return QList<VectorObject*>();
    }

    std::sort(keyframes.begin(), keyframes.end());
    int nearestKf = keyframes.first();
    for (int k : keyframes) {
        if (k <= frameNumber) {
            nearestKf = k;
        } else {
            break;
        }
    }

    // Get objects at nearest keyframe and apply interpolated transforms
    QList<VectorObject*> baseObjects = objectsAtFrame(nearestKf);
    QList<VectorObject*> result;

    for (VectorObject *obj : baseObjects) {
        VectorObject *clone = obj->clone();
        clone->setPos(kf.position);
        clone->setRotation(kf.rotation);
        clone->setScale(kf.scale);
        clone->setOpacity(kf.opacity);
        result.append(clone);
    }

    return result;
}
