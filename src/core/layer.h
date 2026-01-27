#ifndef LAYER_H
#define LAYER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QMap>
#include <QList>

class Frame;  // Keep for compatibility
class VectorObject;

enum class LayerType {
    Art,
    Background,
    Audio,
    Reference
};

// Frame extension structure for holding frames
struct FrameExtension {
    int keyFrame;       // The frame with actual content
    int extendToFrame;  // The last frame to hold the content

    FrameExtension() : keyFrame(-1), extendToFrame(-1) {}
    FrameExtension(int key, int extend) : keyFrame(key), extendToFrame(extend) {}
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
};

#endif // LAYER_H
