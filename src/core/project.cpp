#include "project.h"
#include "layer.h"
#include "frame.h"
#include "canvas/objects/vectorobject.h"
#include "canvas/objects/pathobject.h"
#include "canvas/objects/shapeobject.h"
#include "canvas/objects/textobject.h"
#include "canvas/objects/imageobject.h"
#include "canvas/objects/transformableimageobject.h"
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
    , m_totalFrames(10)   // Start with 10 blank frames; grows dynamically
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
    m_totalFrames = 10;   // Start with 10 blank frames; grows dynamically

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
    if (frame >= 1 && frame != m_currentFrame) {
        m_currentFrame = frame;
        emit currentFrameChanged(frame);
    }
}

void Project::setTotalFrames(int frames)
{
    if (frames > 0 && m_totalFrames != frames) {
        m_totalFrames = frames;
        emit modified();
    }
}

int Project::highestUsedFrame() const
{
    int highest = 1;
    for (const Layer *layer : m_layers) {
        // Art frames
        const auto &frames = layer->allFrameNumbers();
        for (int f : frames)
            if (f > highest) highest = f;
        // Frame extensions
        const auto &exts = layer->allExtensionEnds();
        for (int f : exts)
            if (f > highest) highest = f;
        // Interpolation keyframes
        for (int f : layer->getInterpolationKeyframes())
            if (f > highest) highest = f;
        // Audio clips
        for (const AudioData &clip : layer->audioClips()) {
            int end = clip.startFrame;
            if (clip.durationFrames > 0)
                end = clip.startFrame + clip.durationFrames - 1;
            if (end > highest) highest = end;
        }
    }
    return highest;
}

int Project::totalFrames() const
{
    // Always keep exactly 10 blank frames ahead of the last used frame.
    // On a fresh project (nothing drawn yet) that means frames 1-10 are
    // visible.  After drawing on frame 1 the highest used frame is 1, so
    // totalFrames becomes 1 + 10 = 11.  Drawing on frame 2 makes it 12,
    // etc.  Interpolation keyframes also count as "used" so there will
    // always be 10 empty frames to draw new poses into.
    int highest  = highestUsedFrame();         // ≥ 1
    int computed = highest + 10;               // always 10 blank ahead
    // During load we restore m_totalFrames from the save file so we
    // never shrink a project below what was saved.
    return qMax(computed, m_totalFrames);
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

        // Save all audio clips for audio layers (supports multiple clips per layer)
        if (layer->layerType() == LayerType::Audio && layer->hasAudio()) {
            QJsonArray audioClipsArray;
            for (const AudioData &clipData : layer->audioClips()) {
                QJsonObject audioObj;
                audioObj["filePath"]       = clipData.filePath;
                audioObj["startFrame"]     = clipData.startFrame;
                audioObj["durationFrames"] = clipData.durationFrames;
                audioObj["volume"]         = static_cast<double>(clipData.volume);
                audioObj["muted"]          = clipData.muted;
                audioClipsArray.append(audioObj);
            }
            layerObj["audioClips"] = audioClipsArray;
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

        // Save interpolation ranges (tween ranges) — FIX #26: was not previously saved
        QJsonArray interpRangesArray;
        for (const FrameInterpolation &ir : layer->allInterpolationRanges()) {
            QJsonObject irObj;
            irObj["startFrame"] = ir.startFrame;
            irObj["endFrame"]   = ir.endFrame;
            irObj["easing"]     = ir.easingType;
            interpRangesArray.append(irObj);
        }
        if (!interpRangesArray.isEmpty())
            layerObj["interpRanges"] = interpRangesArray;

        // Save motion path frames — FIX #26: frame colors (purple) now persist
        QJsonArray motionPathArray;
        for (int f : layer->motionPathFrames()) {
            motionPathArray.append(f);
        }
        if (!motionPathArray.isEmpty())
            layerObj["motionPathFrames"] = motionPathArray;

        // Save frame extensions (hold/repeat frames)
        QJsonArray extensionsArray;
        for (int keyFr : layer->allFrameNumbers()) {
            int extEnd = layer->getExtensionEnd(keyFr);
            if (extEnd > keyFr) {
                QJsonObject extObj;
                extObj["keyFrame"]      = keyFr;
                extObj["extendToFrame"] = extEnd;
                extensionsArray.append(extObj);
            }
        }
        if (!extensionsArray.isEmpty())
            layerObj["frameExtensions"] = extensionsArray;

        layersArray.append(layerObj);
    }
    projectObj["layers"] = layersArray;

    // Write to file — compressed binary format
    // Magic header "AVG2" identifies compressed saves; legacy plain-JSON files lack it
    // and are still loaded correctly in loadFromFile().
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(projectObj);
    // Compact (no whitespace) JSON cuts ~30% vs Indented before compression.
    // qCompress level 7 gives near-maximum compression with reasonable CPU cost.
    QByteArray compressed = qCompress(doc.toJson(QJsonDocument::Compact), 7);
    file.write("AVG2");        // 4-byte magic — marks this as a compressed save
    file.write(compressed);
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

    QByteArray raw = file.readAll();
    file.close();

    // Detect compressed format (magic "AVG2") vs legacy plain JSON
    QByteArray data;
    if (raw.startsWith("AVG2")) {
        data = qUncompress(raw.mid(4));
        if (data.isEmpty()) {
            qWarning() << "Failed to decompress project file:" << filePath;
            return false;
        }
    } else {
        data = raw;   // legacy plain-JSON — load as-is
    }

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

        // Load audio clips (new multi-clip format, with legacy single-clip fallback)
        if (layerObj.contains("audioClips")) {
            QJsonArray audioClipsArray = layerObj["audioClips"].toArray();
            for (const QJsonValue &clipVal : audioClipsArray) {
                QJsonObject audioObj = clipVal.toObject();
                AudioData audioData;
                audioData.filePath       = audioObj["filePath"].toString();
                audioData.startFrame     = audioObj["startFrame"].toInt(1);
                audioData.durationFrames = audioObj["durationFrames"].toInt(-1);
                audioData.volume         = static_cast<float>(audioObj["volume"].toDouble(1.0));
                audioData.muted          = audioObj["muted"].toBool(false);
                audioData.isMidi         = audioData.filePath.endsWith(".mid",  Qt::CaseInsensitive)
                                        || audioData.filePath.endsWith(".midi", Qt::CaseInsensitive);
                layer->addAudioClip(audioData);
            }
        } else if (layerObj.contains("audioData")) {
            // Legacy: single-clip format from older saves
            QJsonObject audioObj = layerObj["audioData"].toObject();
            AudioData audioData;
            audioData.filePath       = audioObj["filePath"].toString();
            audioData.startFrame     = audioObj["startFrame"].toInt(1);
            audioData.durationFrames = audioObj["durationFrames"].toInt(-1);
            audioData.volume         = static_cast<float>(audioObj["volume"].toDouble(1.0));
            audioData.muted          = audioObj["muted"].toBool(false);
            audioData.isMidi         = audioData.filePath.endsWith(".mid",  Qt::CaseInsensitive)
                                    || audioData.filePath.endsWith(".midi", Qt::CaseInsensitive);
            layer->addAudioClip(audioData);
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

        // Load frame extensions (hold frames)
        if (layerObj.contains("frameExtensions")) {
            QJsonArray extsArray = layerObj["frameExtensions"].toArray();
            for (const QJsonValue &extVal : extsArray) {
                QJsonObject extObj = extVal.toObject();
                int keyFr  = extObj["keyFrame"].toInt(-1);
                int extEnd = extObj["extendToFrame"].toInt(-1);
                if (keyFr > 0 && extEnd > keyFr)
                    layer->extendFrameTo(keyFr, extEnd);
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

        // Load interpolation ranges — FIX #26
        if (layerObj.contains("interpRanges")) {
            for (const QJsonValue &irVal : layerObj["interpRanges"].toArray()) {
                QJsonObject irObj = irVal.toObject();
                int startF  = irObj["startFrame"].toInt();
                int endF    = irObj["endFrame"].toInt();
                QString easing = irObj["easing"].toString("linear");
                if (startF > 0 && endF > startF)
                    layer->setInterpolation(startF, endF, easing);
            }
        }

        // Load motion path frames — FIX #26 (purple frame color persistence)
        if (layerObj.contains("motionPathFrames")) {
            for (const QJsonValue &fVal : layerObj["motionPathFrames"].toArray()) {
                layer->addMotionPathFrame(fVal.toInt());
            }
        }
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
            // Round to 2 decimal places — sub-pixel precision is invisible
            // but full double (15+ digits) balloons file size enormously
            elemObj["x"] = qRound(elem.x * 100.0) / 100.0;
            elemObj["y"] = qRound(elem.y * 100.0) / 100.0;
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
        // Handle both ImageObject and TransformableImageObject (which also reports Image type)
        QImage image;
        qreal imgW = 0, imgH = 0;
        QPointF imgPos;
        qreal imgAngle = 0;
        bool isTransformable = false;

        if (auto *timg = dynamic_cast<TransformableImageObject*>(obj)) {
            // TransformableImageObject stores its own position, size, angle
            image = timg->getImage();
            imgW = timg->imgWidth();
            imgH = timg->imgHeight();
            imgPos = timg->position();
            imgAngle = timg->imgAngle();
            isTransformable = true;
        } else if (auto *img = dynamic_cast<ImageObject*>(obj)) {
            QPixmap pixmap = img->image();
            image = pixmap.toImage();
        }

        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        // WebP at quality 85 is ~3-5x smaller than PNG for photos/complex images.
        // Fall back to PNG if WebP is unavailable (older Qt builds).
        bool savedOk = image.save(&buffer, "WEBP", 85);
        if (!savedOk) {
            ba.clear();
            buffer.seek(0);
            image.save(&buffer, "PNG");
        }
        data["imageData"] = QString(ba.toBase64());
        data["imageFmt"]  = savedOk ? QString("WEBP") : QString("PNG");
        data["imageWidth"] = image.width();
        data["imageHeight"] = image.height();
        if (isTransformable) {
            data["isTransformable"] = true;
            data["img_w"] = imgW;
            data["img_h"] = imgH;
            data["img_pos_x"] = imgPos.x();
            data["img_pos_y"] = imgPos.y();
            data["img_angle"] = imgAngle;
        }
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
        // Decode base64 image data — use saved format tag if present, PNG for legacy files
        QByteArray ba = QByteArray::fromBase64(data["imageData"].toString().toLatin1());
        QString fmt = data["imageFmt"].toString("PNG");
        QImage image;
        image.loadFromData(ba, fmt.toLatin1().constData());

        if (data["isTransformable"].toBool(false)) {
            // Reconstruct as TransformableImageObject
            auto *timg = new TransformableImageObject(image);
            timg->setImgSize(data["img_w"].toDouble(image.width()),
                             data["img_h"].toDouble(image.height()));
            timg->setPosition(QPointF(data["img_pos_x"].toDouble(0),
                                     data["img_pos_y"].toDouble(0)));
            timg->setImgAngle(data["img_angle"].toDouble(0));
            obj = timg;
        } else {
            ImageObject *img = new ImageObject();
            img->setImage(QPixmap::fromImage(image));
            obj = img;
        }
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
