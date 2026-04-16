#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QPointF>
#include <QTransform>
#include <QString>
#include <QMap>

class VectorObject;
class Layer;

/**
 * @brief Easing functions for interpolation
 */
enum class EasingType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic
};

/**
 * @brief Group of vector objects that can be animated together
 */
class VectorGroup {
public:
    VectorGroup(const QString &name = "Group");
    
    void addObject(VectorObject *obj);
    void removeObject(VectorObject *obj);
    bool contains(VectorObject *obj) const;
    QList<VectorObject*> objects() const { return m_objects; }
    
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    
    // Transform operations on the group
    void setPosition(const QPointF &pos);
    void setRotation(qreal angle);
    void setScale(qreal scale);
    
    QPointF position() const { return m_position; }
    qreal rotation() const { return m_rotation; }
    qreal scale() const { return m_scale; }
    
private:
    QString m_name;
    QList<VectorObject*> m_objects;
    QPointF m_position;
    qreal m_rotation;
    qreal m_scale;
};

/**
 * @brief Keyframe data for interpolation
 */
struct Keyframe {
    int frameNumber;
    QPointF position;
    qreal rotation;
    qreal scale;
    qreal opacity;
    
    Keyframe() : frameNumber(0), rotation(0), scale(1.0), opacity(1.0) {}
    Keyframe(int frame) : frameNumber(frame), rotation(0), scale(1.0), opacity(1.0) {}
};

/**
 * @brief Interpolation engine for smooth animations
 */
class InterpolationEngine : public QObject
{
    Q_OBJECT
    
public:
    explicit InterpolationEngine(QObject *parent = nullptr);
    
    /**
     * @brief Add a keyframe for a group at a specific frame
     */
    void addKeyframe(VectorGroup *group, int frameNumber);
    
    /**
     * @brief Remove a keyframe
     */
    void removeKeyframe(VectorGroup *group, int frameNumber);
    
    /**
     * @brief Get interpolated properties at a specific frame
     * @return Interpolated keyframe data, or null if no interpolation needed
     */
    Keyframe* getInterpolatedFrame(VectorGroup *group, int frameNumber);
    
    /**
     * @brief Set easing type between two keyframes
     */
    void setEasingType(VectorGroup *group, int startFrame, int endFrame, EasingType easing);
    
    /**
     * @brief Check if a frame has a keyframe
     */
    bool hasKeyframe(VectorGroup *group, int frameNumber) const;
    
    /**
     * @brief Get all keyframes for a group
     */
    QList<int> getKeyframes(VectorGroup *group) const;
    
    /**
     * @brief Calculate eased value
     */
    static qreal ease(qreal t, EasingType type);
    
signals:
    void keyframeAdded(VectorGroup *group, int frameNumber);
    void keyframeRemoved(VectorGroup *group, int frameNumber);
    
private:
    // Group -> Frame Number -> Keyframe data
    QMap<VectorGroup*, QMap<int, Keyframe>> m_keyframes;
    
    // Group -> Start Frame -> Easing Type
    QMap<VectorGroup*, QMap<int, EasingType>> m_easings;
    
    /**
     * @brief Find the two keyframes surrounding a given frame
     */
    QPair<Keyframe*, Keyframe*> findSurroundingKeyframes(VectorGroup *group, int frameNumber);
};

#endif // INTERPOLATION_H
