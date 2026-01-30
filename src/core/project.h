#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QString>
#include <QList>
#include <memory>

class Layer;
class Frame;

class Project : public QObject
{
    Q_OBJECT

public:
    explicit Project(QObject *parent = nullptr);
    ~Project();

    // Project properties
    void createNew(int width, int height, int fps);
    QString name() const { return m_name; }
    void setName(const QString &name);

    int width() const { return m_width; }
    int height() const { return m_height; }
    int fps() const { return m_fps; }

    void setWidth(int width);
    void setHeight(int height);
    void setFps(int fps);

    // Drawing settings
    bool smoothPathsEnabled() const { return m_smoothPathsEnabled; }
    void setSmoothPathsEnabled(bool enabled);

    // Onion skinning settings
    bool onionSkinEnabled() const { return m_onionSkinEnabled; }
    void setOnionSkinEnabled(bool enabled);
    int onionSkinBefore() const { return m_onionSkinBefore; }
    void setOnionSkinBefore(int frames);
    int onionSkinAfter() const { return m_onionSkinAfter; }
    void setOnionSkinAfter(int frames);
    qreal onionSkinOpacity() const { return m_onionSkinOpacity; }
    void setOnionSkinOpacity(qreal opacity);

    void addLayerSilent(Layer *layer);
    void removeLayerSilent(int index);
    void insertLayerSilent(int index, Layer *layer);

    // Frame management
    int currentFrame() const { return m_currentFrame; }
    void setCurrentFrame(int frame);
    int totalFrames() const { return m_totalFrames; }
    void setTotalFrames(int frames);

   // const Frame& frame(int index) const;
   // Frame& frame(int index);

    // Layer management
    QList<Layer*> layers() const { return m_layers; }
    Layer* currentLayer() const;
    void setCurrentLayer(int index);
    void addLayer(const QString &name);
    void removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);
    Layer* layerAt(int index) const;
    int layerCount() const { return m_layers.size(); }
    
    // Save/Load
    bool saveToFile(const QString &filePath);
    bool loadFromFile(const QString &filePath);


signals:
    void modified();
    void currentFrameChanged(int frame);
    void currentLayerChanged(Layer *layer);
    void layersChanged();
    void onionSkinSettingsChanged();

private:
    QString m_name;
    int m_width;
    int m_height;
    int m_fps;
    int m_currentFrame;
    int m_totalFrames;
    int m_currentLayerIndex;
    bool m_smoothPathsEnabled;
    
    // Onion skinning settings
    bool m_onionSkinEnabled;
    int m_onionSkinBefore;  // Number of frames to show before current
    int m_onionSkinAfter;   // Number of frames to show after current
    qreal m_onionSkinOpacity; // Base opacity for onion skins

    QList<Layer*> m_layers;
};

#endif // PROJECT_H
