// projectsettings.h
#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

class Project;

class ProjectSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectSettings(Project *project, QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void onFpsChanged(int index);
    void onResolutionChanged();
    void onAudioMuteToggled(bool muted);

private:
    void setupUI();

    Project *m_project;
    QComboBox *m_fpsCombo;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QCheckBox *m_audioMuteCheck;
};

#endif // PROJECTSETTINGS_H

