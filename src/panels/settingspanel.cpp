// FIX APPLIED: Settings panel startup rectangle bug is resolved in mainwindow.cpp.
// MainWindow now calls m_projectSettings->hide() immediately after construction.

#include "settingspanel.h"
#include <QWidget>
#include "core/project.h"
#include "utils/thememanager.h"

#include <QApplication>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QPainter>
#include <QPen>
#include <QBitmap>
#include <QScrollArea>
#include <QLineEdit>
#include <QFileDialog>
#include <QSettings>
#include "settingspanel.h"
#include <QWidget>

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

    QLabel *title = new QLabel("PROJECT SETTINGS");
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
    // PLAYBACK SETTINGS
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
        "   border-color: #c0392b;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        );

    connect(m_fpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProjectSettings::onFpsChanged);

    playbackLayout->addRow(fpsLabel, m_fpsCombo);

    contentLayout->addWidget(playbackGroup);

    // CANVAS SETTINGS
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
            "   background-color: #c0392b;"
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

    // DRAWING SETTINGS
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
        "   background-color: #c0392b;"
        "   border-color: #c0392b;"
        "}"
        );

    connect(m_smoothPathsCheck, &QCheckBox::toggled,
            this, &ProjectSettings::onSmoothPathsToggled);

    drawingLayout->addWidget(m_smoothPathsCheck);

    // Onion Skinning Toggle
    m_onionSkinCheck = new QCheckBox("Enable Onion Skinning");
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
        "QSpinBox:hover { border-color: #c0392b; }");
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
        "QSlider::handle:horizontal { background: #c0392b; width: 14px; margin: -4px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #e05241; }");
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

    m_audioMuteCheck = new QCheckBox("Mute All Audio Layers");
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
        "   background-color: #c0392b;"
        "   border-color: #c0392b;"
        "}"
        );

    connect(m_audioMuteCheck, &QCheckBox::toggled,
            this, &ProjectSettings::onAudioMuteToggled);

    audioLayout->addWidget(m_audioMuteCheck);

    // ── MIDI Soundfont picker ─────────────────────────────────────────────
    QLabel *sf2Label = new QLabel("MIDI Soundfont (.sf2):");
    sf2Label->setStyleSheet("color: #ccc; font-weight: normal; margin-top: 8px;");
    audioLayout->addWidget(sf2Label);

    QWidget *sf2Row = new QWidget();
    QHBoxLayout *sf2Layout = new QHBoxLayout(sf2Row);
    sf2Layout->setContentsMargins(0, 0, 0, 0);
    sf2Layout->setSpacing(6);

    m_sf2Edit = new QLineEdit();
    m_sf2Edit->setPlaceholderText("Auto-detect (system soundfont)");
    m_sf2Edit->setReadOnly(true);
    {
        QSettings s("AkisVG", "AkisVG");
        m_sf2Edit->setText(s.value("midi/soundfont").toString());
    }
    m_sf2Edit->setStyleSheet(
        "QLineEdit {"
        "   background-color: #1e1e1e; color: white;"
        "   border: 1px solid #555; border-radius: 4px; padding: 6px;"
        "}"
        "QLineEdit:hover { border-color: #c0392b; }");

    QPushButton *sf2Browse = new QPushButton("Browse…");
    sf2Browse->setStyleSheet(
        "QPushButton {"
        "   background-color: #1e1e1e; color: #aaa;"
        "   border: 1px solid #555; border-radius: 4px; padding: 6px 10px;"
        "}"
        "QPushButton:hover { background-color: #c0392b; color: white; }");
    connect(sf2Browse, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Select MIDI Soundfont", QString(),
            "SoundFont Files (*.sf2 *.SF2);;All Files (*)");
        if (!path.isEmpty()) {
            m_sf2Edit->setText(path);
            QSettings s("AkisVG", "AkisVG");
            s.setValue("midi/soundfont", path);
            emit settingsChanged();
        }
    });

    QPushButton *sf2Clear = new QPushButton("✕");
    sf2Clear->setToolTip("Reset to auto-detect");
    sf2Clear->setFixedWidth(28);
    sf2Clear->setStyleSheet(sf2Browse->styleSheet());
    connect(sf2Clear, &QPushButton::clicked, this, [this]() {
        m_sf2Edit->clear();
        QSettings s("AkisVG", "AkisVG");
        s.remove("midi/soundfont");
        emit settingsChanged();
    });

    sf2Layout->addWidget(m_sf2Edit, 1);
    sf2Layout->addWidget(sf2Browse);
    sf2Layout->addWidget(sf2Clear);
    audioLayout->addWidget(sf2Row);

    QLabel *sf2Hint = new QLabel("Arch: sudo pacman -S soundfont-fluid");
    sf2Hint->setStyleSheet("color: #555; font-size: 10px;");
    audioLayout->addWidget(sf2Hint);

    contentLayout->addWidget(audioGroup);


    // === APPEARANCE GROUP ===
    QGroupBox *appearanceGroup = new QGroupBox("Appearance");
    appearanceGroup->setStyleSheet(
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

    QFormLayout *appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->setSpacing(8);

    QLabel *themeLabel = new QLabel("Theme:");
    themeLabel->setStyleSheet("color: #ccc; font-weight: normal;");

    m_themeCombo = new QComboBox();
    m_themeCombo->addItem("Studio Grey (default)", 0);
    m_themeCombo->addItem("Red & Black",           1);
    m_themeCombo->addItem("Blue & Black",          2);
    m_themeCombo->addItem("Grey & Blue",           3);
    m_themeCombo->addItem("Grey & Red",            4);
    m_themeCombo->setCurrentIndex(0);
    m_themeCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: #1e1e1e;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "}"
        "QComboBox:hover {"
        "   border-color: #c0392b;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #1e1e1e;"
        "   color: white;"
        "   selection-background-color: #c0392b;"
        "}"
    );

    QLabel *themeHint = new QLabel("Choose the accent + background colour scheme.");
    themeHint->setStyleSheet("color: #666; font-size: 10px; font-weight: normal;");

    appearanceLayout->addRow(themeLabel, m_themeCombo);
    appearanceLayout->addRow(QString(), themeHint);

    // UI Scale
    QLabel *scaleLabel2 = new QLabel("UI Scale:");
    scaleLabel2->setStyleSheet("color: #ccc; font-weight: normal;");

    QWidget *scaleWidget = new QWidget();
    QHBoxLayout *scaleLayout = new QHBoxLayout(scaleWidget);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(8);

    QSlider *uiScaleSlider = new QSlider(Qt::Horizontal);
    uiScaleSlider->setRange(50, 200);
    uiScaleSlider->setTickInterval(25);
    {
        QSettings s("AkisVG", "AkisVG");
        uiScaleSlider->setValue(s.value("ui/scale", 100).toInt());
    }
    uiScaleSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #1e1e1e; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #c0392b; width: 14px; margin: -4px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #e05241; }"
        "QSlider::sub-page:horizontal { background: #c0392b; border-radius: 3px; }");

    QLabel *scaleValueLbl = new QLabel(QString("%1%").arg(uiScaleSlider->value()));
    scaleValueLbl->setStyleSheet("color: white; font-size: 11px; min-width: 36px;");
    scaleValueLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    connect(uiScaleSlider, &QSlider::valueChanged, this, [scaleValueLbl](int v) {
        scaleValueLbl->setText(QString("%1%").arg(v));
        QSettings s("AkisVG", "AkisVG");
        s.setValue("ui/scale", v);
        invalidateUiScaleCache(); // force re-read on next sc() call
        double factor = v / 100.0;
        // Scale font
        QFont f = QApplication::font();
        f.setPointSizeF(9.0 * factor);
        QApplication::setFont(f);
        // Scale all panels, inputs, buttons, scrollbars
        int btnH    = qRound(28  * factor);
        int inputH  = qRound(24  * factor);
        int scrollW = qRound(8   * factor);
        int menuPad = qRound(4   * factor);
        int pad     = qRound(4   * factor);
        int radius  = qRound(4   * factor);
        if (v != 100) {
            qApp->setStyleSheet(QString(
                "QPushButton  { min-height: %1px; padding: 0 %2px; border-radius: %8px; }"
                "QToolButton  { min-height: %1px; min-width:  %1px; }"
                "QLineEdit    { min-height: %3px; padding: %2px; border-radius: %8px; }"
                "QSpinBox     { min-height: %3px; padding: %2px; }"
                "QDoubleSpinBox { min-height: %3px; padding: %2px; }"
                "QComboBox    { min-height: %3px; padding: 0 %2px; }"
                "QTextEdit    { padding: %2px; }"
                "QScrollBar:vertical   { width:  %4px; }"
                "QScrollBar:horizontal { height: %4px; }"
                "QMenu::item  { padding: %5px %6px; }"
                "QMenuBar::item{ padding: %5px %6px; }"
                "QGroupBox    { padding-top: %7px; margin-top: %7px; font-size: %9pt; }"
                "QLabel       { padding: 0; }"
                "QDockWidget::title { padding: %5px; }"
                "QTabBar::tab { min-height: %3px; padding: %5px %6px; }"
            )
            .arg(btnH).arg(pad).arg(inputH).arg(scrollW)
            .arg(menuPad).arg(menuPad*4).arg(menuPad*2)
            .arg(radius).arg(qRound(9 * factor)));
        } else {
            qApp->setStyleSheet(QString()); // reset to Qt defaults at 100%
        }
    });

    scaleLayout->addWidget(uiScaleSlider, 1);
    scaleLayout->addWidget(scaleValueLbl);

    QLabel *scaleHint = new QLabel("Changes apply immediately. Restart recommended for full effect.");
    scaleHint->setStyleSheet("color: #666; font-size: 10px; font-weight: normal;");
    scaleHint->setWordWrap(true);

    appearanceLayout->addRow(scaleLabel2, scaleWidget);
    appearanceLayout->addRow(QString(), scaleHint);

    // Keep unused member initialised to avoid warnings
    m_blueThemeCheck = nullptr;

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProjectSettings::onThemeChanged);

    contentLayout->addWidget(appearanceGroup);

    contentLayout->addStretch();

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: #1a1a1a; width: 8px; border-radius: 4px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #444; border-radius: 4px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: #c0392b; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);
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

void ProjectSettings::onThemeChanged(int index)
{
    ThemeManager::instance().setTheme(index);

    // Update the theme combo's own hover/selection highlight immediately
    if (m_themeCombo) {
        const ThemeColors &t = theme();
        m_themeCombo->setStyleSheet(
            QString("QComboBox { background-color: %1; color: white; border: 1px solid %2;"
                    " border-radius: 4px; padding: 6px; }"
                    "QComboBox:hover { border-color: %3; }"
                    "QComboBox::drop-down { border: none; }"
                    "QComboBox QAbstractItemView { background-color: %1; color: white;"
                    " selection-background-color: %3; }")
            .arg(t.bg4, t.bg3, t.accent));
    }

    emit themeChanged(index);
}

void ProjectSettings::applyTheme()
{
    const ThemeColors &t = theme();

    // Common group box style
    QString groupStyle = QString(
        "QGroupBox {"
        "   color: white;"
        "   font-weight: bold;"
        "   border: 1px solid %1;"
        "   border-radius: 4px;"
        "   margin-top: 8px;"
        "   padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 8px;"
        "}").arg(t.bg3);

    // Common input style
    QString inputStyle = QString(
        "QComboBox, QSpinBox {"
        "   background-color: %1;"
        "   color: white;"
        "   border: 1px solid %2;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "}"
        "QComboBox:hover, QSpinBox:hover {"
        "   border-color: %3;"
        "}"
        "QComboBox::drop-down { border: none; }").arg(t.bg4, t.bg3, t.accent);

    QString checkStyle = QString(
        "QCheckBox {"
        "   color: white;"
        "   font-weight: normal;"
        "   spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "   width: 18px; height: 18px;"
        "   border: 2px solid %1;"
        "   border-radius: 3px;"
        "   background-color: %2;"
        "}"
        "QCheckBox::indicator:checked {"
        "   background-color: %3;"
        "   border-color: %3;"
        "}").arg(t.bg3, t.bg4, t.accent);

    // Apply to all group boxes
    for (QGroupBox *gb : findChildren<QGroupBox*>())
        gb->setStyleSheet(groupStyle);

    // Apply to all combos and spinboxes
    for (QComboBox *cb : findChildren<QComboBox*>())
        cb->setStyleSheet(inputStyle);
    for (QSpinBox *sb : findChildren<QSpinBox*>())
        sb->setStyleSheet(inputStyle);

    // Apply to all checkboxes
    for (QCheckBox *chk : findChildren<QCheckBox*>())
        chk->setStyleSheet(checkStyle);

    // Scroll bars and background
    for (QScrollArea *sa : findChildren<QScrollArea*>())
        sa->setStyleSheet(
            QString("QScrollArea { border: none; background: transparent; }"
                    "QScrollBar:vertical { background: %1; width: 8px; border-radius: 4px; }"
                    "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 20px; }"
                    "QScrollBar::handle:vertical:hover { background: %3; }"
                    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
            .arg(t.bg0, t.bg3, t.accent));

    // Update onion opacity slider
    if (m_onionOpacitySlider)
        m_onionOpacitySlider->setStyleSheet(
            QString("QSlider::groove:horizontal { background: %1; height: 6px; border-radius: 3px; }"
                    "QSlider::handle:horizontal { background: %2; width: 14px; margin: -4px 0; border-radius: 7px; }"
                    "QSlider::handle:horizontal:hover { background: %3; }")
            .arg(t.bg4, t.accent, t.accentHover));

    // Update theme combo specifically
    if (m_themeCombo)
        m_themeCombo->setStyleSheet(
            QString("QComboBox { background-color: %1; color: white; border: 1px solid %2;"
                    " border-radius: 4px; padding: 6px; }"
                    "QComboBox:hover { border-color: %3; }"
                    "QComboBox::drop-down { border: none; }"
                    "QComboBox QAbstractItemView { background-color: %1; color: white;"
                    " selection-background-color: %3; }")
            .arg(t.bg4, t.bg3, t.accent));

    // Update header widget if we can find it
    QList<QWidget*> headers = findChildren<QWidget*>();
    for (QWidget *w : headers) {
        if (w->height() == 40 && w->layout()) {
            w->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
                .arg(t.bg2, t.bg0));
        }
    }
}
