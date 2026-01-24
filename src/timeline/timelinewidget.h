#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

class Project;

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineWidget(Project *project, QWidget *parent = nullptr);

protected:
    void timerEvent(QTimerEvent *event) override;

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
};

#endif // TIMELINEWIDGET_H
