#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

class Project;
class Layer;
class QMediaPlayer;
class QAudioOutput;
struct AudioData;

class FrameGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FrameGridWidget(Project *project, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void setOnionSkin(bool enabled, int frames);

signals:
    void audioLoaded(Layer *layer, const QString &audioPath);
    void referenceImageImported(Layer *layer, const QString &imagePath, int frame);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    AudioData loadAudioFile(const QString &filePath, int startFrame);

    Project *m_project;
    int m_onionFrames;
    bool m_isDragging;
    bool m_onionSkinEnabled;
};

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineWidget(Project *project, QWidget *parent = nullptr);

signals:
    void referenceImageRequested(Layer *layer, const QString &imagePath, int frame);

protected:
    void timerEvent(QTimerEvent *event) override;

public slots:
    void setOnionSkinEnabled(bool enabled);

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onFrameChanged(int frame);
    void updateFrameDisplay();
    void loadAudioTrack(Layer *layer, const QString &audioPath);
    void syncAudioToFrame();
    void handleReferenceImport(Layer *layer, const QString &imagePath, int frame);

private:
    void setupUI();
    void startPlayback();
    void stopPlayback();

    Project *m_project;
    QPushButton *m_playPauseBtn;
    QPushButton *m_stopBtn;
    QLabel *m_frameLabel;
    QLabel *m_fpsLabel;
    bool m_isPlaying;
    int m_playbackTimerId;

    // Audio playback
    QMediaPlayer *m_audioPlayer;
    QAudioOutput *m_audioOutput;
    Layer *m_currentAudioLayer = nullptr;
};

#endif // TIMELINEWIDGET_H
