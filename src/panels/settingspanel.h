// projectsettings.h
#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>

class Project;

class ProjectSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectSettings(Project *project, QWidget *parent = nullptr);
    void applyTheme();

signals:
    void settingsChanged();
    void themeChanged(int themeIndex);

private slots:
    void onFpsChanged(int index);
    void onResolutionChanged();
    void onAudioMuteToggled(bool muted);
    void onSmoothPathsToggled(bool enabled);
    void onOnionSkinToggled(bool enabled);
    void onOnionBeforeChanged(int frames);
    void onOnionAfterChanged(int frames);
    void onOnionOpacityChanged(int value);
    void onThemeChanged(int index);

private:
    void setupUI();

    Project *m_project;
    QComboBox *m_fpsCombo;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QCheckBox *m_audioMuteCheck;
    QCheckBox *m_smoothPathsCheck;

    // Onion skinning controls
    QCheckBox *m_onionSkinCheck;
    QSpinBox *m_onionBeforeSpin;
    QSpinBox *m_onionAfterSpin;
    QSlider *m_onionOpacitySlider;
    QLabel *m_onionOpacityLabel;
    QCheckBox *m_blueThemeCheck; // kept for ABI compat, unused
    QComboBox *m_themeCombo;
    QLineEdit *m_sf2Edit = nullptr;  // MIDI soundfont path
};

#endif // PROJECTSETTINGS_H
