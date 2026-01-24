#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

class Project;

class FrameGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FrameGridWidget(Project *project, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void setOnionSkin(bool enabled, int frames);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
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

protected:
    void timerEvent(QTimerEvent *event) override;

public slots:
    void setOnionSkinEnabled(bool enabled);

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onFrameChanged(int frame);
    void updateFrameDisplay();

private:
    void setupUI();
    void startPlayback();
    void stopPlayback();

    Project *m_project;
    QPushButton *m_playPauseBtn;
    QPushButton *m_stopBtn;
    QSlider *m_frameSlider;
    QLabel *m_frameLabel;
    bool m_isPlaying;
    int m_playbackTimerId;
    bool m_onionSkinEnabled;
};

#endif // TIMELINEWIDGET_H
