#include "interpolationmanager.h"
#include "project.h"
#include "canvas/objects/vectorobject.h"
#include <QtMath>
#include <algorithm>

InterpolationManager::InterpolationManager(Project *project, QObject *parent)
    : QObject(parent)
    , m_project(project)
    , m_groupCounter(0)
{
}

InterpolationManager::~InterpolationManager()
{
    qDeleteAll(m_groups);
}

QString InterpolationManager::createGroup(const QList<VectorObject*> &objects)
{
    if (objects.isEmpty()) {
        return QString();
    }

    // Generate unique group ID
    QString groupId = QString("interp_group_%1").arg(++m_groupCounter);

    // Create new group
    InterpolationGroup *group = new InterpolationGroup(groupId);
    group->objects = objects;

    // Store group
    m_groups[groupId] = group;
    m_activeGroupId = groupId;

    emit groupCreated(groupId);
    emit interpolationModeChanged(true);

    return groupId;
}

void InterpolationManager::deleteGroup(const QString &groupId)
{
    if (m_groups.contains(groupId)) {
        delete m_groups.take(groupId);

        if (m_activeGroupId == groupId) {
            m_activeGroupId.clear();
            emit interpolationModeChanged(false);
        }

        emit groupDeleted(groupId);
    }
}

InterpolationGroup* InterpolationManager::getGroup(const QString &groupId)
{
    return m_groups.value(groupId, nullptr);
}

QList<QString> InterpolationManager::getAllGroupIds() const
{
    return m_groups.keys();
}

bool InterpolationManager::addKeyframe(const QString &groupId, int frameNumber)
{
    InterpolationGroup *group = getGroup(groupId);
    if (!group) {
        return false;
    }

    // Capture current positions of all objects in the group
    QList<QPointF> positions;
    for (VectorObject *obj : group->objects) {
        positions.append(obj->pos());
    }

    // Store keyframe
    group->keyframes[frameNumber] = positions;

    emit keyframeAdded(groupId, frameNumber);

    return true;
}

void InterpolationManager::removeKeyframe(const QString &groupId, int frameNumber)
{
    InterpolationGroup *group = getGroup(groupId);
    if (!group) {
        return;
    }

    if (group->keyframes.remove(frameNumber)) {
        emit keyframeRemoved(groupId, frameNumber);
    }
}

const InterpolationGroup* InterpolationManager::getGroup(const QString &groupId) const
{
    return m_groups.value(groupId, nullptr);
}

QList<int> InterpolationManager::getKeyframes(const QString &groupId) const
{
    const InterpolationGroup *group = getGroup(groupId);
    if (!group) {
        return QList<int>();
    }

    return group->keyframes.keys();
}

bool InterpolationManager::hasKeyframe(const QString &groupId, int frameNumber) const
{
    const InterpolationGroup *group = getGroup(groupId);
    if (!group) {
        return false;
    }

    return group->keyframes.contains(frameNumber);
}

void InterpolationManager::updateObjectPositions(const QString &groupId, int frameNumber)
{
    InterpolationGroup *group = getGroup(groupId);
    if (!group || group->keyframes.isEmpty()) {
        return;
    }

    // If this frame is exactly a keyframe, use those positions
    if (group->keyframes.contains(frameNumber)) {
        const QList<QPointF> &positions = group->keyframes[frameNumber];
        for (int i = 0; i < qMin(group->objects.size(), positions.size()); ++i) {
            group->objects[i]->setPos(positions[i]);
        }
        return;
    }

    // Find surrounding keyframes for interpolation
    QList<int> keyframeNumbers = group->keyframes.keys();
    std::sort(keyframeNumbers.begin(), keyframeNumbers.end());

    int startFrame = -1;
    int endFrame = -1;

    // Find the keyframes before and after current frame
    for (int kf : keyframeNumbers) {
        if (kf < frameNumber) {
            startFrame = kf;
        } else if (kf > frameNumber && endFrame == -1) {
            endFrame = kf;
            break;
        }
    }

    // Can't interpolate if we don't have surrounding keyframes
    if (startFrame == -1 || endFrame == -1) {
        return;
    }

    // Calculate interpolation factor (0.0 to 1.0)
    qreal t = qreal(frameNumber - startFrame) / qreal(endFrame - startFrame);

    // Apply easing function
    t = applyEasing(t, group->easingType);

    // Get start and end positions
    const QList<QPointF> &startPositions = group->keyframes[startFrame];
    const QList<QPointF> &endPositions = group->keyframes[endFrame];

    // Interpolate each object's position
    int objectCount = qMin(group->objects.size(), qMin(startPositions.size(), endPositions.size()));
    for (int i = 0; i < objectCount; ++i) {
        QPointF interpolated = interpolatePosition(startPositions[i], endPositions[i], t, group->easingType);
        group->objects[i]->setPos(interpolated);
    }
}

void InterpolationManager::setEasingType(const QString &groupId, const QString &easingType)
{
    InterpolationGroup *group = getGroup(groupId);
    if (group) {
        group->easingType = easingType;
    }
}

void InterpolationManager::setActiveGroup(const QString &groupId)
{
    if (m_groups.contains(groupId)) {
        m_activeGroupId = groupId;
        emit interpolationModeChanged(!groupId.isEmpty());
    }
}

void InterpolationManager::finishInterpolation()
{
    m_activeGroupId.clear();
    emit interpolationModeChanged(false);
}

QPointF InterpolationManager::interpolatePosition(const QPointF &start, const QPointF &end, qreal t, const QString &easingType)
{
    Q_UNUSED(easingType);  // Already applied to t

    // Linear interpolation between start and end
    return start + (end - start) * t;
}

qreal InterpolationManager::applyEasing(qreal t, const QString &easingType)
{
    // Clamp t to [0, 1]
    t = qBound(0.0, t, 1.0);

    // Apply easing function based on type
    if (easingType == "easeIn" || easingType == "easeInQuad") {
        // Accelerating from zero velocity
        return t * t;
    }
    else if (easingType == "easeOut" || easingType == "easeOutQuad") {
        // Decelerating to zero velocity
        return t * (2.0 - t);
    }
    else if (easingType == "easeInOut" || easingType == "easeInOutQuad") {
        // Acceleration until halfway, then deceleration
        if (t < 0.5) {
            return 2.0 * t * t;
        } else {
            return -1.0 + (4.0 - 2.0 * t) * t;
        }
    }
    else if (easingType == "easeInCubic") {
        // Cubic ease in
        return t * t * t;
    }
    else if (easingType == "easeOutCubic") {
        // Cubic ease out
        qreal tMinus = t - 1.0;
        return 1.0 + tMinus * tMinus * tMinus;
    }
    else if (easingType == "easeInOutCubic") {
        // Cubic ease in-out
        if (t < 0.5) {
            return 4.0 * t * t * t;
        } else {
            qreal tMinus = t - 1.0;
            return 1.0 + 4.0 * tMinus * tMinus * tMinus;
        }
    }

    // Default: linear (no easing)
    return t;
}
