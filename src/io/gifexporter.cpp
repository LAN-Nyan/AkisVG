#include "gifexporter.h"
#include "core/project.h"
#include "core/layer.h"
#include "canvas/objects/vectorobject.h"
#include <QPainter>
#include <QImageWriter>
#include <QBuffer>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QDir>

// For advanced GIF encoding, you might want to use a library like giflib or libgif
// For this implementation, we'll use Qt's basic GIF support with some workarounds

GifExporter::GifExporter(Project *project, QObject *parent)
    : QObject(parent)
    , m_project(project)
    , m_frameDelayMs(1000 / 24) // Default to 24 FPS
    , m_loopForever(true)
{
}

GifExporter::~GifExporter()
{
}

void GifExporter::setFrameDelay(int delayMs)
{
    m_frameDelayMs = qMax(1, delayMs);
}

void GifExporter::setLoop(bool loop)
{
    m_loopForever = loop;
}

bool GifExporter::exportToGif(const QString &outputPath, ExportMode mode,
                               int startFrame, int endFrame, int quality)
{
    if (!m_project) {
        m_lastError = "No project loaded";
        emit errorOccurred(m_lastError);
        return false;
    }

    // Determine frame range
    if (endFrame == -1) {
        endFrame = m_project->totalFrames();
    }

    startFrame = qMax(1, startFrame);
    endFrame = qMin(endFrame, m_project->totalFrames());

    if (startFrame > endFrame) {
        m_lastError = "Invalid frame range";
        emit errorOccurred(m_lastError);
        return false;
    }

    // Get frames to export based on mode
    QList<int> framesToExport = getFramesToExport(mode, startFrame, endFrame);

    if (framesToExport.isEmpty()) {
        m_lastError = "No frames to export";
        emit errorOccurred(m_lastError);
        return false;
    }

    emit exportStarted(framesToExport.size());

    // Render all frames
    QList<QImage> frames;
    for (int i = 0; i < framesToExport.size(); ++i) {
        int frameNum = framesToExport[i];

        QImage frame = renderFrame(frameNum);
        if (frame.isNull()) {
            m_lastError = QString("Failed to render frame %1").arg(frameNum);
            emit errorOccurred(m_lastError);
            return false;
        }

        // Convert to indexed color for GIF
        QImage indexedFrame = convertToIndexedColor(frame);
        frames.append(indexedFrame);

        emit frameExported(i + 1, framesToExport.size());
    }

    // Write GIF file
    bool success = writeGifFile(outputPath, frames);

    emit exportFinished(success);

    if (!success) {
        m_lastError = "Failed to write GIF file";
        emit errorOccurred(m_lastError);
    }

    return success;
}

QList<int> GifExporter::getFramesToExport(ExportMode mode, int startFrame, int endFrame)
{
    QList<int> frames;

    if (mode == ExportMode::EveryFrame) {
        // Export all frames in range
        for (int f = startFrame; f <= endFrame; ++f) {
            frames.append(f);
        }
    } else {
        // Export only keyframes (and extended frames)
        for (int f = startFrame; f <= endFrame; ++f) {
            if (isKeyframeOrExtended(f)) {
                frames.append(f);
            }
        }
    }

    return frames;
}

bool GifExporter::isKeyframeOrExtended(int frameNumber)
{
    QList<Layer*> layers = m_project->layers();

    for (Layer* layer : layers) {
        if (layer->layerType() == LayerType::Audio) {
            continue; // Skip audio layers
        }

        // Check if this frame has content (keyframe or extended)
        if (layer->hasContentAtFrame(frameNumber)) {
            return true;
        }
    }

    return false;
}

QImage GifExporter::renderFrame(int frameNumber)
{
    // Create image with project dimensions
    QImage image(m_project->width(), m_project->height(), QImage::Format_ARGB32);
    image.fill(Qt::white); // Fill with white background

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Render all visible layers
    QList<Layer*> layers = m_project->layers();

    // Render from bottom to top (reverse order)
    for (int i = layers.size() - 1; i >= 0; --i) {
        Layer* layer = layers[i];

        // Skip invisible or audio layers
        if (!layer->isVisible() || layer->layerType() == LayerType::Audio) {
            continue;
        }

        // Set layer opacity
        painter.setOpacity(layer->opacity());

        // Get objects at this frame
        QList<VectorObject*> objects = layer->objectsAtFrame(frameNumber);

        // Render each object
        for (VectorObject* obj : objects) {
            if (obj) {
                obj->paint(&painter, nullptr, nullptr);
            }
        }
    }

    painter.end();
    return image;
}

QImage GifExporter::convertToIndexedColor(const QImage &image, int colorCount)
{
    // Convert to 8-bit indexed color with optimized palette
    // This is necessary for GIF format

    QImage indexed = image.convertToFormat(
        QImage::Format_Indexed8,
        Qt::AutoColor | Qt::ThresholdDither | Qt::AvoidDither
    );

    // If conversion failed, try without dithering
    if (indexed.isNull()) {
        indexed = image.convertToFormat(QImage::Format_Indexed8);
    }

    return indexed;
}

bool GifExporter::writeGifFile(const QString &path, const QList<QImage> &frames)
{
    if (frames.isEmpty()) {
        return false;
    }

    // For single frames, use simple writer
    if (frames.size() == 1) {
        QImageWriter writer(path, "GIF");
        writer.setQuality(85);
        return writer.write(frames.first());
    }

    // For animated GIFs, use FFmpeg
    // Calculate FPS based on delay
    int fps = qMax(1, 1000 / m_frameDelayMs);
    
    // Create temporary directory for frames
    QString tempDir = QFileInfo(path).absolutePath() + "/temp_gif_frames";
    QDir().mkpath(tempDir);
    
    // Save all frames as PNG
    for (int i = 0; i < frames.size(); ++i) {
        QString framePath = QString("%1/frame_%2.png")
            .arg(tempDir)
            .arg(i, 4, 10, QChar('0'));
        
        if (!frames[i].save(framePath, "PNG")) {
            QDir(tempDir).removeRecursively();
            m_lastError = QString("Failed to save frame %1").arg(i);
            return false;
        }
    }
    
    // Use FFmpeg to create animated GIF
    QProcess ffmpeg;
    QStringList args;
    args << "-y"  // Overwrite output
         << "-framerate" << QString::number(fps)
         << "-i" << QString("%1/frame_%04d.png").arg(tempDir)
         << "-vf" << "split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse"  // High quality palette
         << "-loop" << (m_loopForever ? "0" : "-1")
         << path;
    
    ffmpeg.start("ffmpeg", args);
    
    if (!ffmpeg.waitForStarted(3000)) {
        QDir(tempDir).removeRecursively();
        m_lastError = "FFmpeg not found. Please install ffmpeg to export animated GIFs.";
        return false;
    }
    
    if (!ffmpeg.waitForFinished(60000)) {  // 60 second timeout
        ffmpeg.kill();
        QDir(tempDir).removeRecursively();
        m_lastError = "FFmpeg process timed out";
        return false;
    }
    
    // Clean up temporary frames
    QDir(tempDir).removeRecursively();
    
    if (ffmpeg.exitCode() != 0) {
        QString errorOutput = QString::fromUtf8(ffmpeg.readAllStandardError());
        m_lastError = QString("FFmpeg error: %1").arg(errorOutput);
        return false;
    }
    
    return true;
}

// ===== Integration with giflib (if available) =====
// Uncomment and use this if you have giflib installed

/*
#ifdef HAS_GIFLIB
#include <gif_lib.h>

bool GifExporter::writeGifFile(const QString &path, const QList<QImage> &frames)
{
    if (frames.isEmpty()) return false;

    int errorCode;
    GifFileType *gifFile = EGifOpenFileName(path.toUtf8().constData(), false, &errorCode);

    if (!gifFile) {
        m_lastError = QString("Failed to create GIF file: %1").arg(GifErrorString(errorCode));
        return false;
    }

    // Set GIF properties
    int width = frames.first().width();
    int height = frames.first().height();
    int colorRes = 8; // 8-bit color

    if (EGifPutScreenDesc(gifFile, width, height, colorRes, 0, nullptr) == GIF_ERROR) {
        EGifCloseFile(gifFile, &errorCode);
        return false;
    }

    // Set up looping
    if (m_loopForever) {
        unsigned char loop[11] = "NETSCAPE2.0";
        unsigned char params[3] = {1, 0, 0}; // Loop forever

        EGifPutExtensionLeader(gifFile, APPLICATION_EXT_FUNC_CODE);
        EGifPutExtensionBlock(gifFile, 11, loop);
        EGifPutExtensionBlock(gifFile, 3, params);
        EGifPutExtensionTrailer(gifFile);
    }

    // Write each frame
    for (const QImage &frame : frames) {
        // Convert to indexed color
        QImage indexed = convertToIndexedColor(frame);

        // Set up graphics control extension for frame delay
        unsigned char gcExtension[4];
        gcExtension[0] = 0; // Disposal method
        gcExtension[1] = (m_frameDelayMs / 10) & 0xFF; // Delay in 1/100ths of second
        gcExtension[2] = ((m_frameDelayMs / 10) >> 8) & 0xFF;
        gcExtension[3] = 0; // Transparent color index (none)

        EGifPutExtension(gifFile, GRAPHICS_EXT_FUNC_CODE, 4, gcExtension);

        // Create color map
        ColorMapObject *colorMap = GifMakeMapObject(256, nullptr);
        for (int i = 0; i < indexed.colorCount(); ++i) {
            QRgb color = indexed.color(i);
            colorMap->Colors[i].Red = qRed(color);
            colorMap->Colors[i].Green = qGreen(color);
            colorMap->Colors[i].Blue = qBlue(color);
        }

        // Write image descriptor
        if (EGifPutImageDesc(gifFile, 0, 0, width, height, false, colorMap) == GIF_ERROR) {
            GifFreeMapObject(colorMap);
            EGifCloseFile(gifFile, &errorCode);
            return false;
        }

        // Write pixel data
        for (int y = 0; y < height; ++y) {
            const uchar *line = indexed.constScanLine(y);
            if (EGifPutLine(gifFile, const_cast<uchar*>(line), width) == GIF_ERROR) {
                GifFreeMapObject(colorMap);
                EGifCloseFile(gifFile, &errorCode);
                return false;
            }
        }

        GifFreeMapObject(colorMap);
    }

    // Close the GIF file
    if (EGifCloseFile(gifFile, &errorCode) == GIF_ERROR) {
        m_lastError = QString("Failed to close GIF file: %1").arg(GifErrorString(errorCode));
        return false;
    }

    return true;
}
#endif // HAS_GIFLIB
*/
