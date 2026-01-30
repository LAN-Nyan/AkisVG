#ifndef LAYER_H
#define LAYER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QMap>
#include <QList>
#include <QPointF>

class Frame;  // Keep for compatibility
class VectorObject;

enum class LayerType {
    Art,
    Background,
    Audio,
    Reference,
    Interpolation  // NEW: Keyframe-based interpolation layer
};

// Frame extension structure for holding frames
struct FrameExtension {
    int keyFrame;       // The frame with actual content
    int extendToFrame;  // The last frame to hold the content

    FrameExtension() : keyFrame(-1), extendToFrame(-1) {}
    FrameExtension(int key, int extend) : keyFrame(key), extendToFrame(extend) {}
};

// Interpolation/Tween data structure
struct FrameInterpolation {
    int startFrame;     // Starting keyframe
    int endFrame;       // Ending keyframe
    QString easingType; // "linear", "easeIn", "easeOut", "easeInOut"

    FrameInterpolation() : startFrame(-1), endFrame(-1), easingType("linear") {}
    FrameInterpolation(int start, int end, const QString &easing = "linear")
        : startFrame(start), endFrame(end), easingType(easing) {}
};

// Audio layer data structure
struct AudioData {
    QString filePath;
    int startFrame;
    int durationFrames;
    float volume;
    bool muted;
    QVector<float> waveformData; // Normalized audio samples for visualization

    AudioData() : startFrame(1), durationFrames(0), volume(1.0f), muted(false) {}
    AudioData(const QString &path, int start, int duration)
        : filePath(path), startFrame(start), durationFrames(duration),
          volume(1.0f), muted(false) {}
};

// Interpolation keyframe data
struct InterpolationKeyframe {
    int frameNumber;
    QPointF position;
    qreal rotation;
    qreal scale;
    qreal opacity;
    QString easingType;  // "linear", "easeIn", "easeOut", "easeInOut"
    
    InterpolationKeyframe() 
        : frameNumber(1), rotation(0), scale(1.0), opacity(1.0), easingType("linear") {}
    InterpolationKeyframe(int frame, QPointF pos = QPointF(0, 0))
        : frameNumber(frame), position(pos), rotation(0), scale(1.0), 
          opacity(1.0), easingType("linear") {}
};

class Layer : public QObject
{
    Q_OBJECT

public:
    explicit Layer(const QString &name, QObject *parent = nullptr);
    ~Layer();

    // Properties
    QString name() const { return m_name; }
    void setName(const QString &name);

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

    bool isLocked() const { return m_locked; }
    void setLocked(bool locked);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

    LayerType layerType() const { return m_layerType; }
    void setLayerType(LayerType type);
    QString layerTypeString() const;

    // Frame data management
    QList<VectorObject*> objectsAtFrame(int frameNumber) const;
    void addObjectToFrame(int frameNumber, VectorObject *obj);
    void removeObjectFromFrame(int frameNumber, VectorObject *obj);
    void clearFrame(int frameNumber);
    bool hasContentAtFrame(int frameNumber) const;

    // Frame extension/hold functionality
    void extendFrameTo(int fromFrame, int toFrame);
    void clearFrameExtension(int frame);
    bool isFrameExtended(int frameNumber) const;
    int getKeyFrameFor(int frameNumber) const;
    int getExtensionEnd(int frameNumber) const;
    bool isKeyFrame(int frameNumber) const;

    // Interpolation/Tween functionality
    void setInterpolation(int startFrame, int endFrame, const QString &easingType = "linear");
    void clearInterpolation(int startFrame);
    bool isInterpolated(int frameNumber) const;
    bool isInterpolationKeyFrame(int frameNumber) const;
    FrameInterpolation getInterpolationFor(int frameNumber) const;

    // Audio layer functionality
    void setAudioData(const AudioData &audio);
    AudioData getAudioData() const { return m_audioData; }
    bool hasAudio() const { return !m_audioData.filePath.isEmpty(); }
    void clearAudio();
    
    // Interpolation layer functionality (for LayerType::Interpolation)
    void addInterpolationKeyframe(int frameNumber, const InterpolationKeyframe &keyframe);
    void removeInterpolationKeyframe(int frameNumber);
    InterpolationKeyframe getInterpolatedFrame(int frameNumber) const;
    bool hasInterpolationKeyframe(int frameNumber) const;
    QList<int> getInterpolationKeyframes() const;
    QList<VectorObject*> getInterpolatedObjects(int frameNumber) const;

    // Make keyframe (converts extended/interpolated frame to actual keyframe)
    void makeKeyFrame(int frameNumber);

    // For tools that still need Frame* interface
    Frame* frameAt(int index);
    Frame* frameIfExists(int index) const;
    const Frame* getActiveFrameAt(int index) const;

    void emitModified() { emit modified(); }

signals:
    void modified();
    void visibilityChanged(bool visible);
    void lockedChanged(bool locked);
    void typeChanged(LayerType type);

private:
    QString m_name;
    bool m_visible;
    bool m_locked;
    QColor m_color;
    qreal m_opacity;
    LayerType m_layerType;

    // Frame data: frameNumber -> list of objects
    QMap<int, QList<VectorObject*>> m_frames;

    // COMPATIBILITY: Cache Frame objects for tools that need them
    mutable QMap<int, Frame*> m_framCache;

    // Frame extensions mapping
    QMap<int, FrameExtension> m_frameExtensions;

    // Interpolation data
    QMap<int, FrameInterpolation> m_interpolations;

    // Audio data (for audio layers)
    AudioData m_audioData;
    
    // Interpolation keyframes (for interpolation layers)
    QMap<int, InterpolationKeyframe> m_interpKeyframes;

    // Helper method for interpolation
    qreal calculateEasing(qreal t, const QString &easingType) const;
};

#endif // LAYER_H
