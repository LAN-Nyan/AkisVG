#include "interpolation.h"
#include "canvas/objects/vectorobject.h"
#include <QtMath>

// ==================== VectorGroup ====================

VectorGroup::VectorGroup(const QString &name)
    : m_name(name)
    , m_rotation(0)
    , m_scale(1.0)
{
}

void VectorGroup::addObject(VectorObject *obj)
{
    if (obj && !m_objects.contains(obj)) {
        m_objects.append(obj);
    }
}

void VectorGroup::removeObject(VectorObject *obj)
{
    m_objects.removeOne(obj);
}

bool VectorGroup::contains(VectorObject *obj) const
{
    return m_objects.contains(obj);
}

void VectorGroup::setPosition(const QPointF &pos)
{
    QPointF delta = pos - m_position;
    m_position = pos;

    // Move all objects by the delta
    for (VectorObject *obj : m_objects) {
        obj->moveBy(delta.x(), delta.y());
    }
}

void VectorGroup::setRotation(qreal angle)
{
    qreal delta = angle - m_rotation;
    m_rotation = angle;

    // Rotate all objects around group center
    QTransform transform;
    transform.translate(m_position.x(), m_position.y());
    transform.rotate(delta);
    transform.translate(-m_position.x(), -m_position.y());

    for (VectorObject *obj : m_objects) {
        QPointF objPos = obj->pos();
        QPointF newPos = transform.map(objPos);
        obj->setPos(newPos);
        obj->setRotation(obj->rotation() + delta);
    }
}

void VectorGroup::setScale(qreal scale)
{
    qreal ratio = scale / m_scale;
    m_scale = scale;

    // Scale all objects relative to group center
    for (VectorObject *obj : m_objects) {
        QPointF objPos = obj->pos();
        QPointF delta = objPos - m_position;
        delta *= ratio;
        obj->setPos(m_position + delta);
        obj->setScale(obj->scale() * ratio);
    }
}

// ==================== InterpolationEngine ====================

InterpolationEngine::InterpolationEngine(QObject *parent)
    : QObject(parent)
{
}

void InterpolationEngine::addKeyframe(VectorGroup *group, int frameNumber)
{
    if (!group) return;

    Keyframe kf(frameNumber);
    kf.position = group->position();
    kf.rotation = group->rotation();
    kf.scale = group->scale();

    m_keyframes[group][frameNumber] = kf;

    emit keyframeAdded(group, frameNumber);
}

void InterpolationEngine::removeKeyframe(VectorGroup *group, int frameNumber)
{
    if (!group || !m_keyframes.contains(group)) return;

    m_keyframes[group].remove(frameNumber);

    if (m_keyframes[group].isEmpty()) {
        m_keyframes.remove(group);
    }

    emit keyframeRemoved(group, frameNumber);
}

Keyframe* InterpolationEngine::getInterpolatedFrame(VectorGroup *group, int frameNumber)
{
    if (!group || !m_keyframes.contains(group)) {
        return nullptr;
    }

    auto &keyframes = m_keyframes[group];

    // If this exact frame is a keyframe, return it
    if (keyframes.contains(frameNumber)) {
        return &keyframes[frameNumber];
    }

    // Find surrounding keyframes
    auto surrounding = findSurroundingKeyframes(group, frameNumber);
    Keyframe *start = surrounding.first;
    Keyframe *end = surrounding.second;

    if (!start || !end) {
        return nullptr;  // No interpolation possible
    }

    // Calculate interpolation factor (0 to 1)
    int totalFrames = end->frameNumber - start->frameNumber;
    qreal t = qreal(frameNumber - start->frameNumber) / totalFrames;

    // Apply easing
    EasingType easing = EasingType::Linear;
    if (m_easings.contains(group) && m_easings[group].contains(start->frameNumber)) {
        easing = m_easings[group][start->frameNumber];
    }
    qreal easedT = ease(t, easing);

    // Interpolate properties
    static Keyframe interpolated;
    interpolated.frameNumber = frameNumber;
    interpolated.position = start->position + (end->position - start->position) * easedT;
    interpolated.rotation = start->rotation + (end->rotation - start->rotation) * easedT;
    interpolated.scale = start->scale + (end->scale - start->scale) * easedT;
    interpolated.opacity = start->opacity + (end->opacity - start->opacity) * easedT;

    return &interpolated;
}

void InterpolationEngine::setEasingType(VectorGroup *group, int startFrame, int endFrame, EasingType easing)
{
    if (!group) return;

    Q_UNUSED(endFrame); // Added

    m_easings[group][startFrame] = easing;
}

bool InterpolationEngine::hasKeyframe(VectorGroup *group, int frameNumber) const
{
    if (!group || !m_keyframes.contains(group)) {
        return false;
    }

    return m_keyframes[group].contains(frameNumber);
}

QList<int> InterpolationEngine::getKeyframes(VectorGroup *group) const
{
    if (!group || !m_keyframes.contains(group)) {
        return QList<int>();
    }

    return m_keyframes[group].keys();
}

qreal InterpolationEngine::ease(qreal t, EasingType type)
{
    t = qBound(0.0, t, 1.0);

    switch (type) {
    case EasingType::Linear:
        return t;

    case EasingType::EaseIn:
    case EasingType::EaseInQuad:
        return t * t;

    case EasingType::EaseOut:
    case EasingType::EaseOutQuad:
        return t * (2.0 - t);

    case EasingType::EaseInOut:
    case EasingType::EaseInOutQuad:
        if (t < 0.5) {
            return 2.0 * t * t;
        } else {
            return -1.0 + (4.0 - 2.0 * t) * t;
        }

    case EasingType::EaseInCubic:
        return t * t * t;

    case EasingType::EaseOutCubic: {
        qreal tMinus = t - 1.0;
        return 1.0 + tMinus * tMinus * tMinus;
    }

    case EasingType::EaseInOutCubic: {
        if (t < 0.5) {
            return 4.0 * t * t * t;
        } else {
            qreal tMinus = t - 1.0;
            qreal tMinus2 = tMinus - 1.0;
            return 1.0 + tMinus * (2.0 * tMinus2) * (2.0 * tMinus);
        }
    }

    default:
        return t;
    }
}

QPair<Keyframe*, Keyframe*> InterpolationEngine::findSurroundingKeyframes(VectorGroup *group, int frameNumber)
{
    if (!group || !m_keyframes.contains(group)) {
        return QPair<Keyframe*, Keyframe*>(nullptr, nullptr);
    }

    auto &keyframes = m_keyframes[group];
    QList<int> frames = keyframes.keys();
    std::sort(frames.begin(), frames.end());

    Keyframe *start = nullptr;
    Keyframe *end = nullptr;

    for (int frame : frames) {
        if (frame < frameNumber) {
            start = &keyframes[frame];
        } else if (frame > frameNumber) {
            end = &keyframes[frame];
            break;
        }
    }

    return QPair<Keyframe*, Keyframe*>(start, end);
}
