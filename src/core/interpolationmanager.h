#ifndef INTERPOLATIONMANAGER_H
#define INTERPOLATIONMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QString>

class VectorObject;
class Project;

// Represents a group of vectors being interpolated together
class InterpolationGroup {
public:
    QString groupId;
    QList<VectorObject*> objects;
    QMap<int, QList<QPointF>> keyframes;  // frameNumber -> list of positions for each object
    QString easingType;

    InterpolationGroup(const QString &id = QString())
        : groupId(id), easingType("linear") {}
};

// Manages all interpolation groups in the project
class InterpolationManager : public QObject
{
    Q_OBJECT

public:
    explicit InterpolationManager(Project *project, QObject *parent = nullptr);
    ~InterpolationManager();
    const InterpolationGroup* getGroup(const QString &groupId) const;

    // Group management
    QString createGroup(const QList<VectorObject*> &objects);
    void deleteGroup(const QString &groupId);
    InterpolationGroup* getGroup(const QString &groupId);
    QList<QString> getAllGroupIds() const;

    // Keyframe management
    bool addKeyframe(const QString &groupId, int frameNumber);
    void removeKeyframe(const QString &groupId, int frameNumber);
    QList<int> getKeyframes(const QString &groupId) const;
    bool hasKeyframe(const QString &groupId, int frameNumber) const;

    // Interpolation calculation
    void updateObjectPositions(const QString &groupId, int frameNumber);
    void setEasingType(const QString &groupId, const QString &easingType);

    // State management
    bool isInInterpolationMode() const { return !m_activeGroupId.isEmpty(); }
    QString activeGroupId() const { return m_activeGroupId; }
    void setActiveGroup(const QString &groupId);
    void finishInterpolation();

signals:
    void interpolationModeChanged(bool active);
    void keyframeAdded(const QString &groupId, int frameNumber);
    void keyframeRemoved(const QString &groupId, int frameNumber);
    void groupCreated(const QString &groupId);
    void groupDeleted(const QString &groupId);

private:
    Project *m_project;
    QMap<QString, InterpolationGroup*> m_groups;
    QString m_activeGroupId;
    int m_groupCounter;

    // Helper methods
    QPointF interpolatePosition(const QPointF &start, const QPointF &end, qreal t, const QString &easingType);
    qreal applyEasing(qreal t, const QString &easingType);
};

#endif // INTERPOLATIONMANAGER_H
