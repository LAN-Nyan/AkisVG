
#include "settingspanel.h"
#include "core/project.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QPainter>
#include <QPen>
#include <QBitmap>
#include "settingspanel.h"

ProjectSettings::ProjectSettings(Project *project, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
{
    setupUI();
}

void ProjectSettings::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    QWidget *header = new QWidget();
    header->setStyleSheet("background-color: #3a3a3a; border-bottom: 1px solid #000;");
    header->setFixedHeight(40);

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel *title = new QLabel("⚙️ PROJECT SETTINGS");
    title->setStyleSheet("color: white; font-weight: bold; font-size: 11px; letter-spacing: 1px;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    // Settings Content
    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: #2d2d2d;");

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(12, 16, 12, 16);
    contentLayout->setSpacing(16);

    // === PLAYBACK SETTINGS ===
    QGroupBox *playbackGroup = new QGroupBox("Playback");
    playbackGroup->setStyleSheet(
        "QGroupBox {"
        "   color: white;"
        "   font-weight: bold;"
        "   border: 1px solid #444;"
        "   border-radius: 4px;"
        "   margin-top: 8px;"
        "   padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 8px;"
        "}"
        );

    QFormLayout *playbackLayout = new QFormLayout(playbackGroup);
    playbackLayout->setSpacing(8);

    // FPS Selector
    QLabel *fpsLabel = new QLabel("Frame Rate:");
    fpsLabel->setStyleSheet("color: #ccc; font-weight: normal;");

    m_fpsCombo = new QComboBox();
    m_fpsCombo->addItem("12 FPS", 12);
    m_fpsCombo->addItem("24 FPS", 24);
    m_fpsCombo->addItem("30 FPS", 30);
    m_fpsCombo->addItem("60 FPS", 60);
    m_fpsCombo->setCurrentIndex(1); // Default to 24 FPS
    m_fpsCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: #1e1e1e;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "}"
        "QComboBox:hover {"
        "   border-color: #2a82da;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        );

    connect(m_fpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProjectSettings::onFpsChanged);

    playbackLayout->addRow(fpsLabel, m_fpsCombo);

    contentLayout->addWidget(playbackGroup);

    // === CANVAS SETTINGS ===
    QGroupBox *canvasGroup = new QGroupBox("Canvas");
    canvasGroup->setStyleSheet(playbackGroup->styleSheet());

    QFormLayout *canvasLayout = new QFormLayout(canvasGroup);
    canvasLayout->setSpacing(8);

    // Resolution
    QLabel *resLabel = new QLabel("Resolution:");
    resLabel->setStyleSheet("color: #ccc; font-weight: normal;");

    QWidget *resWidget = new QWidget();
    QHBoxLayout *resLayout = new QHBoxLayout(resWidget);
    resLayout->setContentsMargins(0, 0, 0, 0);
    resLayout->setSpacing(8);

    m_widthSpinBox = new QSpinBox();
    m_widthSpinBox->setRange(320, 7680);
    m_widthSpinBox->setSingleStep(8);
    m_widthSpinBox->setValue(m_project->width());
    m_widthSpinBox->setSuffix(" px");
    m_widthSpinBox->setStyleSheet(
        "QSpinBox {"
        "   background-color: #1e1e1e;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "}"
        );

    QLabel *xLabel = new QLabel("×");
    xLabel->setStyleSheet("color: #888;");

    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(240, 4320);
    m_heightSpinBox->setSingleStep(8);
    m_heightSpinBox->setValue(m_project->height());
    m_heightSpinBox->setSuffix(" px");
    m_heightSpinBox->setStyleSheet(m_widthSpinBox->styleSheet());

    resLayout->addWidget(m_widthSpinBox);
    resLayout->addWidget(xLabel);
    resLayout->addWidget(m_heightSpinBox);
    resLayout->addStretch();

    connect(m_widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ProjectSettings::onResolutionChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ProjectSettings::onResolutionChanged);

    canvasLayout->addRow(resLabel, resWidget);

    // Quick presets
    QHBoxLayout *presetsLayout = new QHBoxLayout();

    auto createPresetButton = [this](const QString &text, int w, int h) {
        QPushButton *btn = new QPushButton(text);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: #1e1e1e;"
            "   color: #aaa;"
            "   border: 1px solid #555;"
            "   border-radius: 3px;"
            "   padding: 4px 8px;"
            "   font-size: 10px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #2a82da;"
            "   color: white;"
            "}"
            );
        connect(btn, &QPushButton::clicked, [this, w, h]() {
            m_widthSpinBox->setValue(w);
            m_heightSpinBox->setValue(h);
        });
        return btn;
    };

    presetsLayout->addWidget(createPresetButton("HD", 1280, 720));
    presetsLayout->addWidget(createPresetButton("Full HD", 1920, 1080));
    presetsLayout->addWidget(createPresetButton("4K", 3840, 2160));
    presetsLayout->addStretch();

    canvasLayout->addRow("", presetsLayout);

    contentLayout->addWidget(canvasGroup);

    // === DRAWING SETTINGS ===
    QGroupBox *drawingGroup = new QGroupBox("Drawing");
    drawingGroup->setStyleSheet(playbackGroup->styleSheet());

    QVBoxLayout *drawingLayout = new QVBoxLayout(drawingGroup);

    m_smoothPathsCheck = new QCheckBox("✨ Enable Smooth Paths (Fix Dotted Lines)");
    m_smoothPathsCheck->setChecked(true);  // Enabled by default
    m_smoothPathsCheck->setStyleSheet(
        "QCheckBox {"
        "   color: white;"
        "   font-weight: normal;"
        "   spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "   width: 18px;"
        "   height: 18px;"
        "   border: 2px solid #555;"
        "   border-radius: 3px;"
        "   background-color: #1e1e1e;"
        "}"
        "QCheckBox::indicator:checked {"
        "   background-color: #2a82da;"
        "   border-color: #2a82da;"
        "}"
        );

    connect(m_smoothPathsCheck, &QCheckBox::toggled,
            this, &ProjectSettings::onSmoothPathsToggled);

    drawingLayout->addWidget(m_smoothPathsCheck);
    
    // Onion Skinning Toggle
    m_onionSkinCheck = new QCheckBox("👁️ Enable Onion Skinning");
    m_onionSkinCheck->setChecked(false);  // Disabled by default
    m_onionSkinCheck->setStyleSheet(m_smoothPathsCheck->styleSheet());
    connect(m_onionSkinCheck, &QCheckBox::toggled,
            this, &ProjectSettings::onOnionSkinToggled);
    drawingLayout->addWidget(m_onionSkinCheck);
    
    // Onion Skin Settings (indent these)
    QWidget *onionSkinSettings = new QWidget();
    QVBoxLayout *onionSettingsLayout = new QVBoxLayout(onionSkinSettings);
    onionSettingsLayout->setContentsMargins(24, 4, 0, 0);
    onionSettingsLayout->setSpacing(6);
    
    // Frames Before
    QHBoxLayout *beforeLayout = new QHBoxLayout();
    QLabel *beforeLabel = new QLabel("Previous Frames:");
    beforeLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    m_onionBeforeSpin = new QSpinBox();
    m_onionBeforeSpin->setRange(0, 5);
    m_onionBeforeSpin->setValue(1);
    m_onionBeforeSpin->setStyleSheet(
        "QSpinBox { background-color: #1e1e1e; color: white; border: 1px solid #555; "
        "border-radius: 3px; padding: 4px; min-width: 60px; }"
        "QSpinBox:hover { border-color: #2a82da; }");
    connect(m_onionBeforeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ProjectSettings::onOnionBeforeChanged);
    beforeLayout->addWidget(beforeLabel);
    beforeLayout->addWidget(m_onionBeforeSpin);
    beforeLayout->addStretch();
    onionSettingsLayout->addLayout(beforeLayout);
    
    // Frames After
    QHBoxLayout *afterLayout = new QHBoxLayout();
    QLabel *afterLabel = new QLabel("Next Frames:");
    afterLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    m_onionAfterSpin = new QSpinBox();
    m_onionAfterSpin->setRange(0, 5);
    m_onionAfterSpin->setValue(1);
    m_onionAfterSpin->setStyleSheet(m_onionBeforeSpin->styleSheet());
    connect(m_onionAfterSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ProjectSettings::onOnionAfterChanged);
    afterLayout->addWidget(afterLabel);
    afterLayout->addWidget(m_onionAfterSpin);
    afterLayout->addStretch();
    onionSettingsLayout->addLayout(afterLayout);
    
    // Opacity Slider
    QHBoxLayout *opacityLayout = new QHBoxLayout();
    QLabel *opacityLabel = new QLabel("Opacity:");
    opacityLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    m_onionOpacitySlider = new QSlider(Qt::Horizontal);
    m_onionOpacitySlider->setRange(10, 80);
    m_onionOpacitySlider->setValue(30);
    m_onionOpacitySlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #1e1e1e; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #2a82da; width: 14px; margin: -4px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #3a92ea; }");
    m_onionOpacityLabel = new QLabel("30%");
    m_onionOpacityLabel->setStyleSheet("color: white; font-size: 11px; min-width: 35px;");
    connect(m_onionOpacitySlider, &QSlider::valueChanged,
            this, &ProjectSettings::onOnionOpacityChanged);
    opacityLayout->addWidget(opacityLabel);
    opacityLayout->addWidget(m_onionOpacitySlider);
    opacityLayout->addWidget(m_onionOpacityLabel);
    onionSettingsLayout->addLayout(opacityLayout);
    
    drawingLayout->addWidget(onionSkinSettings);

    contentLayout->addWidget(drawingGroup);

    // === AUDIO SETTINGS ===
    QGroupBox *audioGroup = new QGroupBox("Audio");
    audioGroup->setStyleSheet(playbackGroup->styleSheet());

    QVBoxLayout *audioLayout = new QVBoxLayout(audioGroup);

    m_audioMuteCheck = new QCheckBox("🔇 Mute All Audio Layers");
    m_audioMuteCheck->setStyleSheet(
        "QCheckBox {"
        "   color: white;"
        "   font-weight: normal;"
        "   spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "   width: 18px;"
        "   height: 18px;"
        "   border: 2px solid #555;"
        "   border-radius: 3px;"
        "   background-color: #1e1e1e;"
        "}"
        "QCheckBox::indicator:checked {"
        "   background-color: #2a82da;"
        "   border-color: #2a82da;"
        "}"
        );

    connect(m_audioMuteCheck, &QCheckBox::toggled,
            this, &ProjectSettings::onAudioMuteToggled);

    audioLayout->addWidget(m_audioMuteCheck);

    contentLayout->addWidget(audioGroup);

    contentLayout->addStretch();

    mainLayout->addWidget(contentWidget);
}

void ProjectSettings::onFpsChanged(int index)
{
    int fps = m_fpsCombo->itemData(index).toInt();
    m_project->setFps(fps);
    emit settingsChanged();
}

void ProjectSettings::onResolutionChanged()
{
    int width = m_widthSpinBox->value();
    int height = m_heightSpinBox->value();

    m_project->setWidth(width);
    m_project->setHeight(height);

    emit settingsChanged();
}

void ProjectSettings::onAudioMuteToggled(bool muted)
{
    // TODO: Implement audio muting when audio playback is added
    Q_UNUSED(muted);
    emit settingsChanged();
}

void ProjectSettings::onSmoothPathsToggled(bool enabled)
{
    m_project->setSmoothPathsEnabled(enabled);
    emit settingsChanged();
}

void ProjectSettings::onOnionSkinToggled(bool enabled)
{
    m_project->setOnionSkinEnabled(enabled);
    emit settingsChanged();
}

void ProjectSettings::onOnionBeforeChanged(int frames)
{
    m_project->setOnionSkinBefore(frames);
    emit settingsChanged();
}

void ProjectSettings::onOnionAfterChanged(int frames)
{
    m_project->setOnionSkinAfter(frames);
    emit settingsChanged();
}

void ProjectSettings::onOnionOpacityChanged(int value)
{
    m_project->setOnionSkinOpacity(value / 100.0);
    m_onionOpacityLabel->setText(QString("%1%").arg(value));
    emit settingsChanged();
}


