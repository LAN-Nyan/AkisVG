#ifndef GIFEXPORTER_H
#define GIFEXPORTER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QList>

class Project;

/**
 * @brief The GifExporter class handles exporting animations to GIF format
 *
 * Supports two export modes:
 * 1. Keyframes Only - Exports only frames with actual drawn content
 * 2. Every Frame - Exports all frames in the timeline range
 */
class GifExporter : public QObject
{
    Q_OBJECT

public:
    enum class ExportMode {
        KeyframesOnly,  // Export only frames with keyframe content
        EveryFrame      // Export all frames in range
    };

    explicit GifExporter(Project *project, QObject *parent = nullptr);
    ~GifExporter();

    /**
     * @brief Export animation to GIF file
     * @param outputPath Output file path
     * @param mode Export mode (keyframes only or every frame)
     * @param startFrame Starting frame (1-based, default 1)
     * @param endFrame Ending frame (1-based, -1 = project total)
     * @param quality Quality setting (1-100, default 85)
     * @return true if export succeeded
     */
    bool exportToGif(
        const QString &outputPath,
        ExportMode mode = ExportMode::EveryFrame,
        int startFrame = 1,
        int endFrame = -1,
        int quality = 85
    );

    /**
     * @brief Set frame delay for GIF animation
     * @param delayMs Delay in milliseconds between frames
     */
    void setFrameDelay(int delayMs);

    /**
     * @brief Set whether to loop the GIF infinitely
     * @param loop True for infinite loop, false for play once
     */
    void setLoop(bool loop);

    /**
     * @brief Get last error message
     */
    QString lastError() const { return m_lastError; }

signals:
    void exportStarted(int totalFrames);
    void frameExported(int frameNumber, int totalFrames);
    void exportFinished(bool success);
    void errorOccurred(const QString &error);

private:
    Project *m_project;
    QString m_lastError;
    int m_frameDelayMs;
    bool m_loopForever;

    // Render a single frame to QImage
    QImage renderFrame(int frameNumber);

    // Get list of frames to export based on mode
    QList<int> getFramesToExport(ExportMode mode, int startFrame, int endFrame);

    // Convert QImage to indexed color palette (required for GIF)
    QImage convertToIndexedColor(const QImage &image, int colorCount = 256);

    // Write GIF using external library or Qt's QImageWriter
    bool writeGifFile(const QString &path, const QList<QImage> &frames);

    // Helper to check if a frame has keyframe content
    bool isKeyframeOrExtended(int frameNumber);
};

#endif // GIFEXPORTER_H
