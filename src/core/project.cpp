#include "project.h"
#include "layer.h"
#include "frame.h"
#include "canvas/objects/vectorobject.h"
#include "canvas/objects/pathobject.h"
#include "canvas/objects/shapeobject.h"
#include "canvas/objects/textobject.h"
#include "canvas/objects/imageobject.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QColor>
#include <QBuffer>

// Forward declarations for serialization helpers
static QJsonObject serializeVectorObject(VectorObject *obj);
static VectorObject* deserializeVectorObject(const QJsonObject &data);

//const Frame& Project::frame(int index) const {
    // Returns a const reference to the frame
   // return *(m_layers[m_currentLayerIndex]->frameAt(index));
//}

//Frame& Project::frame(int index) {
// Returns a writable reference to the frame
    //return *(m_layers[m_currentLayerIndex]->frameAt(index));
//}

Project::Project(QObject *parent)
    : QObject(parent)
    , m_name("Untitled Project")
    , m_width(1920)
    , m_height(1080)
    , m_fps(24)
    , m_currentFrame(1)
    , m_totalFrames(100)
    , m_currentLayerIndex(0)
    , m_smoothPathsEnabled(true)  // Enable smooth paths by default
    , m_onionSkinEnabled(false)   // Onion skinning disabled by default
    , m_onionSkinBefore(1)        // Show 1 frame before
    , m_onionSkinAfter(1)         // Show 1 frame after
    , m_onionSkinOpacity(0.3)     // 30% opacity for onion skins
{
    // Create default layer
    addLayer("Layer 1");
}

Project::~Project()
{
    qDeleteAll(m_layers);
}

void Project::createNew(int width, int height, int fps)
{
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_currentFrame = 1;
    m_totalFrames = 100;

    // Clear existing layers
    qDeleteAll(m_layers);
    m_layers.clear();

    // Create default layer
    addLayer("Layer 1");
    m_currentLayerIndex = 0;

    emit modified();
    emit layersChanged();
}

void Project::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit modified();
    }
}

void Project::setWidth(int width)
{
    if (m_width != width) {
        m_width = width;
        emit modified();
    }
}

void Project::setHeight(int height)
{
    if (m_height != height) {
        m_height = height;
        emit modified();
    }
}

void Project::setFps(int fps)
{
    if (m_fps != fps) {
        m_fps = fps;
        emit modified();
    }
}

void Project::setSmoothPathsEnabled(bool enabled)
{
    if (m_smoothPathsEnabled != enabled) {
        m_smoothPathsEnabled = enabled;
        emit modified();
    }
}

void Project::setOnionSkinEnabled(bool enabled)
{
    if (m_onionSkinEnabled != enabled) {
        m_onionSkinEnabled = enabled;
        emit onionSkinSettingsChanged();
        emit modified();
    }
}

void Project::setOnionSkinBefore(int frames)
{
    if (frames >= 0 && m_onionSkinBefore != frames) {
        m_onionSkinBefore = frames;
        emit onionSkinSettingsChanged();
        emit modified();
    }
}

void Project::setOnionSkinAfter(int frames)
{
    if (frames >= 0 && m_onionSkinAfter != frames) {
        m_onionSkinAfter = frames;
        emit onionSkinSettingsChanged();
        emit modified();
    }
}

void Project::setOnionSkinOpacity(qreal opacity)
{
    if (opacity >= 0.0 && opacity <= 1.0 && m_onionSkinOpacity != opacity) {
        m_onionSkinOpacity = opacity;
        emit onionSkinSettingsChanged();
        emit modified();
    }
}

void Project::setCurrentFrame(int frame)
{
    if (frame >= 1 && frame <= m_totalFrames && frame != m_currentFrame) {
        m_currentFrame = frame;
        emit currentFrameChanged(frame);
    }
}

void Project::setTotalFrames(int frames)
{
    if (frames > 0 && frames != m_totalFrames) {
        m_totalFrames = frames;
        if (m_currentFrame > frames) {
            setCurrentFrame(frames);
        }
        emit modified();
    }
}

Layer* Project::currentLayer() const
{
    if (m_currentLayerIndex >= 0 && m_currentLayerIndex < m_layers.size()) {
        return m_layers[m_currentLayerIndex];
    }
    return nullptr;
}

void Project::setCurrentLayer(int index)
{
    if (index >= 0 && index < m_layers.size() && index != m_currentLayerIndex) {
        m_currentLayerIndex = index;
        emit currentLayerChanged(m_layers[index]);
    }
}

void Project::addLayer(const QString &name)
{
    Layer *layer = new Layer(name, this);
    m_layers.append(layer);
    emit layersChanged();
    emit modified();
}

void Project::removeLayer(int index)
{
    if (index >= 0 && index < m_layers.size() && m_layers.size() > 1) {
        Layer *layer = m_layers.takeAt(index);
        delete layer;

        if (m_currentLayerIndex >= m_layers.size()) {
            m_currentLayerIndex = m_layers.size() - 1;
        }

        emit layersChanged();
        emit modified();
    }
}

void Project::moveLayer(int fromIndex, int toIndex)
{
    if (fromIndex >= 0 && fromIndex < m_layers.size() &&
        toIndex >= 0 && toIndex < m_layers.size() &&
        fromIndex != toIndex) {

        m_layers.move(fromIndex, toIndex);

        if (m_currentLayerIndex == fromIndex) {
            m_currentLayerIndex = toIndex;
        }

        emit layersChanged();
        emit modified();
    }
}

void Project::addLayerSilent(Layer *layer)
{
    if (!layer) return;

    // Safety check: is it already in the list?
    if (m_layers.contains(layer)) return;

    m_layers.append(layer);
    // Note: We emit manually in the Command to avoid recursion
}

void Project::removeLayerSilent(int index)
{
    // SIGABRT prevention: Check bounds BEFORE removal
    if (index >= 0 && index < m_layers.size()) {
        m_layers.removeAt(index);

        // Update current index safely
        if (m_currentLayerIndex >= m_layers.size()) {
            m_currentLayerIndex = qMax(0, m_layers.size() - 1);
        }
    }
}


void Project::insertLayerSilent(int index, Layer *layer)
{
    if (layer && index >= 0 && index <= m_layers.size()) {
        m_layers.insert(index, layer);
        emit layersChanged();
    }
}

Layer* Project::layerAt(int index) const
{
    if (index >= 0 && index < m_layers.size()) {
        return m_layers[index];
    }
    return nullptr;
}

bool Project::saveToFile(const QString &filePath)
{
    QJsonObject projectObj;

    // Project metadata
    projectObj["version"] = "1.0";
    projectObj["name"] = m_name;
    projectObj["width"] = m_width;
    projectObj["height"] = m_height;
    projectObj["fps"] = m_fps;
    projectObj["totalFrames"] = m_totalFrames;
    projectObj["smoothPathsEnabled"] = m_smoothPathsEnabled;

    // Onion skin settings
    QJsonObject onionSkin;
    onionSkin["enabled"] = m_onionSkinEnabled;
    onionSkin["before"] = m_onionSkinBefore;
    onionSkin["after"] = m_onionSkinAfter;
    onionSkin["opacity"] = m_onionSkinOpacity;
    projectObj["onionSkin"] = onionSkin;

    // Layers
    QJsonArray layersArray;
    for (Layer *layer : m_layers) {
        QJsonObject layerObj;
        layerObj["name"] = layer->name();
        layerObj["visible"] = layer->isVisible();
        layerObj["locked"] = layer->isLocked();
        layerObj["opacity"] = layer->opacity();
        layerObj["color"] = layer->color().name();
        layerObj["type"] = layer->layerTypeString();

        // Save audio data if this is an audio layer
        if (layer->layerType() == LayerType::Audio && layer->hasAudio()) {
            AudioData audioData = layer->getAudioData();
            QJsonObject audioObj;
            audioObj["filePath"] = audioData.filePath;
            audioObj["startFrame"] = audioData.startFrame;
            audioObj["durationFrames"] = audioData.durationFrames;
            audioObj["volume"] = static_cast<double>(audioData.volume);
            audioObj["muted"] = audioData.muted;
            layerObj["audioData"] = audioObj;
        }

        // Save interpolation keyframes if this is an interpolation layer
        if (layer->layerType() == LayerType::Interpolation) {
            QJsonArray interpKeyframesArray;
            QList<int> keyframes = layer->getInterpolationKeyframes();
            for (int kf : keyframes) {
                InterpolationKeyframe ikf = layer->getInterpolatedFrame(kf);
                QJsonObject kfObj;
                kfObj["frame"] = ikf.frameNumber;
                kfObj["pos_x"] = ikf.position.x();
                kfObj["pos_y"] = ikf.position.y();
                kfObj["rotation"] = ikf.rotation;
                kfObj["scale"] = ikf.scale;
                kfObj["opacity"] = ikf.opacity;
                kfObj["easing"] = ikf.easingType;
                interpKeyframesArray.append(kfObj);
            }
            layerObj["interpKeyframes"] = interpKeyframesArray;
        }

        // Save all frames with objects
        QJsonArray framesArray;
        for (int frame = 1; frame <= m_totalFrames; ++frame) {
            QList<VectorObject*> objects = layer->objectsAtFrame(frame);
            if (!objects.isEmpty() && layer->isKeyFrame(frame)) {
                QJsonObject frameObj;
                frameObj["number"] = frame;

                QJsonArray objectsArray;
                for (VectorObject *obj : objects) {
                    objectsArray.append(serializeVectorObject(obj));
                }
                frameObj["objects"] = objectsArray;
                framesArray.append(frameObj);
            }
        }
        layerObj["frames"] = framesArray;

        layersArray.append(layerObj);
    }
    projectObj["layers"] = layersArray;

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(projectObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

bool Project::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid project file format";
        return false;
    }

    QJsonObject projectObj = doc.object();

    // Load project metadata
    m_name = projectObj["name"].toString("Untitled");
    m_width = projectObj["width"].toInt(1920);
    m_height = projectObj["height"].toInt(1080);
    m_fps = projectObj["fps"].toInt(24);
    m_totalFrames = projectObj["totalFrames"].toInt(100);
    m_smoothPathsEnabled = projectObj["smoothPathsEnabled"].toBool(true);

    // Load onion skin settings
    if (projectObj.contains("onionSkin")) {
        QJsonObject onionSkin = projectObj["onionSkin"].toObject();
        m_onionSkinEnabled = onionSkin["enabled"].toBool(false);
        m_onionSkinBefore = onionSkin["before"].toInt(1);
        m_onionSkinAfter = onionSkin["after"].toInt(1);
        m_onionSkinOpacity = onionSkin["opacity"].toDouble(0.3);
    }

    // Clear existing layers
    qDeleteAll(m_layers);
    m_layers.clear();

    // Load layers
    QJsonArray layersArray = projectObj["layers"].toArray();
    for (const QJsonValue &layerValue : layersArray) {
        QJsonObject layerObj = layerValue.toObject();

        Layer *layer = new Layer(layerObj["name"].toString("Layer"), this);
        layer->setVisible(layerObj["visible"].toBool(true));
        layer->setLocked(layerObj["locked"].toBool(false));
        layer->setOpacity(layerObj["opacity"].toDouble(1.0));
        layer->setColor(QColor(layerObj["color"].toString("#808080")));

        // Set layer type based on saved type string
        QString typeStr = layerObj["type"].toString("Art");
        if (typeStr == "Audio") {
            layer->setLayerType(LayerType::Audio);
        } else if (typeStr == "Background") {
            layer->setLayerType(LayerType::Background);
        } else if (typeStr == "Reference") {
            layer->setLayerType(LayerType::Reference);
        } else if (typeStr == "Interpolation") {
            layer->setLayerType(LayerType::Interpolation);
        } else {
            layer->setLayerType(LayerType::Art);
        }

        // Load audio data if present
        if (layerObj.contains("audioData")) {
            QJsonObject audioObj = layerObj["audioData"].toObject();
            AudioData audioData;
            audioData.filePath = audioObj["filePath"].toString();
            audioData.startFrame = audioObj["startFrame"].toInt(1);
            audioData.durationFrames = audioObj["durationFrames"].toInt(0);
            audioData.volume = static_cast<float>(audioObj["volume"].toDouble(1.0));
            audioData.muted = audioObj["muted"].toBool(false);
            layer->setAudioData(audioData);
        }

        // Load interpolation keyframes if present
        if (layerObj.contains("interpKeyframes")) {
            QJsonArray interpKeyframesArray = layerObj["interpKeyframes"].toArray();
            for (const QJsonValue &kfVal : interpKeyframesArray) {
                QJsonObject kfObj = kfVal.toObject();
                InterpolationKeyframe ikf;
                ikf.frameNumber = kfObj["frame"].toInt(1);
                ikf.position = QPointF(kfObj["pos_x"].toDouble(), kfObj["pos_y"].toDouble());
                ikf.rotation = kfObj["rotation"].toDouble(0.0);
                ikf.scale = kfObj["scale"].toDouble(1.0);
                ikf.opacity = kfObj["opacity"].toDouble(1.0);
                ikf.easingType = kfObj["easing"].toString("linear");
                layer->addInterpolationKeyframe(ikf.frameNumber, ikf);
            }
        }

        // Load frames with vector objects
        if (layerObj.contains("frames")) {
            QJsonArray framesArray = layerObj["frames"].toArray();
            for (const QJsonValue &frameVal : framesArray) {
                QJsonObject frameObj = frameVal.toObject();
                int frameNum = frameObj["number"].toInt(1);

                // Load objects
                QJsonArray objectsArray = frameObj["objects"].toArray();
                for (const QJsonValue &objVal : objectsArray) {
                    VectorObject *obj = deserializeVectorObject(objVal.toObject());
                    if (obj) {
                        layer->addObjectToFrame(frameNum, obj);
                    }
                }
            }
        }

        m_layers.append(layer);
    }

    if (m_layers.isEmpty()) {
        addLayer("Layer 1");
    }

    m_currentLayerIndex = 0;
    m_currentFrame = 1;

    emit modified();
    emit layersChanged();
    emit onionSkinSettingsChanged();

    return true;
}

// ============= SERIALIZATION HELPERS =============

static QJsonObject serializeVectorObject(VectorObject *obj)
{
    if (!obj) return QJsonObject();

    QJsonObject data;

    // Common properties
    data["type"] = static_cast<int>(obj->objectType());
    data["pos_x"] = obj->pos().x();
    data["pos_y"] = obj->pos().y();
    data["rotation"] = obj->rotation();
    data["scale"] = obj->scale();
    data["strokeColor"] = obj->strokeColor().name(QColor::HexArgb);
    data["fillColor"] = obj->fillColor().name(QColor::HexArgb);
    data["strokeWidth"] = obj->strokeWidth();
    data["opacity"] = obj->objectOpacity();
    data["zValue"] = obj->zValue();

    // Type-specific properties
    switch (obj->objectType()) {
    case VectorObjectType::Path: {
        PathObject *path = static_cast<PathObject*>(obj);
        QPainterPath painterPath = path->path();

        // Serialize path elements
        QJsonArray elementsArray;
        for (int i = 0; i < painterPath.elementCount(); ++i) {
            QPainterPath::Element elem = painterPath.elementAt(i);
            QJsonObject elemObj;
            elemObj["type"] = elem.type;
            elemObj["x"] = elem.x;
            elemObj["y"] = elem.y;
            elementsArray.append(elemObj);
        }
        data["pathElements"] = elementsArray;
        data["smoothPaths"] = path->smoothPaths();
        data["texture"] = static_cast<int>(path->texture());
        break;
    }

    case VectorObjectType::Rectangle:
    case VectorObjectType::Ellipse: {
        ShapeObject *shape = static_cast<ShapeObject*>(obj);
        data["width"] = shape->rect().width();
        data["height"] = shape->rect().height();
        data["rect_x"] = shape->rect().x();
        data["rect_y"] = shape->rect().y();
        break;
    }

    case VectorObjectType::Text: {
        TextObject *text = static_cast<TextObject*>(obj);
        data["text"] = text->text();
        data["fontFamily"] = text->fontFamily();
        data["fontSize"] = text->fontSize();
        // TextObject doesn't store bold/italic currently, just family and size
        break;
    }

    case VectorObjectType::Image: {
        ImageObject *img = static_cast<ImageObject*>(obj);
        QPixmap pixmap = img->image();
        QImage image = pixmap.toImage();

        // Convert image to base64
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        data["imageData"] = QString(ba.toBase64());
        data["imageWidth"] = image.width();
        data["imageHeight"] = image.height();
        break;
    }
    }

    return data;
}

static VectorObject* deserializeVectorObject(const QJsonObject &data)
{
    if (data.isEmpty()) return nullptr;

    VectorObjectType objType = static_cast<VectorObjectType>(data["type"].toInt());
    VectorObject *obj = nullptr;

    switch (objType) {
    case VectorObjectType::Path: {
        PathObject *path = new PathObject();

        // Restore path elements
        QJsonArray elementsArray = data["pathElements"].toArray();
        QPainterPath painterPath;

        for (const QJsonValue &elemVal : elementsArray) {
            QJsonObject elemObj = elemVal.toObject();
            QPainterPath::ElementType type = static_cast<QPainterPath::ElementType>(elemObj["type"].toInt());
            qreal x = elemObj["x"].toDouble();
            qreal y = elemObj["y"].toDouble();

            switch (type) {
            case QPainterPath::MoveToElement:
                painterPath.moveTo(x, y);
                break;
            case QPainterPath::LineToElement:
                painterPath.lineTo(x, y);
                break;
            case QPainterPath::CurveToElement:
            case QPainterPath::CurveToDataElement:
                // Handle curves (simplified)
                painterPath.lineTo(x, y);
                break;
            }
        }

        path->setPath(painterPath);
        path->setSmoothPaths(data["smoothPaths"].toBool(true));
        path->setTexture(static_cast<PathTexture>(data["texture"].toInt()));
        obj = path;
        break;
    }

      case VectorObjectType::Rectangle: {
              // FIXED: Removed the redundant ShapeType:: scope
              ShapeObject *shape = new ShapeObject(ShapeObject::Rectangle);
              qreal w = data["width"].toDouble();
              qreal h = data["height"].toDouble();
              qreal x = data["rect_x"].toDouble();
              qreal y = data["rect_y"].toDouble();
              shape->setRect(QRectF(x, y, w, h));
              obj = shape;
              break;
          }

          case VectorObjectType::Ellipse: {
              // FIXED: Removed the redundant ShapeType:: scope
              ShapeObject *shape = new ShapeObject(ShapeObject::Ellipse);
              qreal w = data["width"].toDouble();
              qreal h = data["height"].toDouble();
              qreal x = data["rect_x"].toDouble();
              qreal y = data["rect_y"].toDouble();
              shape->setRect(QRectF(x, y, w, h));
              obj = shape;
              break;
          }

    case VectorObjectType::Text: {
        TextObject *text = new TextObject();
        text->setText(data["text"].toString());
        text->setFontFamily(data["fontFamily"].toString("Arial"));
        text->setFontSize(data["fontSize"].toInt(12));
        obj = text;
        break;
    }

    case VectorObjectType::Image: {
        ImageObject *img = new ImageObject();

        // Decode base64 image data
        QByteArray ba = QByteArray::fromBase64(data["imageData"].toString().toLatin1());
        QImage image;
        image.loadFromData(ba, "PNG");
        img->setImage(QPixmap::fromImage(image));
        obj = img;
        break;
    }
    }

    if (obj) {
        // Restore common properties
        obj->setPos(data["pos_x"].toDouble(), data["pos_y"].toDouble());
        obj->setRotation(data["rotation"].toDouble());
        obj->setScale(data["scale"].toDouble(1.0));
        obj->setStrokeColor(QColor(data["strokeColor"].toString("#000000")));
        obj->setFillColor(QColor(data["fillColor"].toString("#00000000")));
        obj->setStrokeWidth(data["strokeWidth"].toDouble(1.0));
        obj->setObjectOpacity(data["opacity"].toDouble(1.0));
        obj->setZValue(data["zValue"].toDouble(0.0));
    }

    return obj;
}
