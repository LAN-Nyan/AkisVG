#include "exportdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFrame>

ExportDialog::ExportDialog(ExportType type, QWidget *parent)
    : QDialog(parent)
    , m_type(type)
    , m_defaultWidth(1920)
    , m_defaultHeight(1080)
    , m_defaultFPS(24)
    , m_maxFrame(100)
{
    setupUI();
    applyModernStyle();
    
    QString title;
    switch (m_type) {
        case ExportType::Video:
            title = "Export Video";
            break;
        case ExportType::GIF:
            title = "Export GIF";
            break;
        case ExportType::Image:
            title = "Export Image";
            break;
    }
    setWindowTitle(title);
    
    resize(500, 600);
}

void ExportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    QLabel *titleLabel = new QLabel();
    titleLabel->setObjectName("titleLabel");
    switch (m_type) {
        case ExportType::Video:
            titleLabel->setText("Video Export Settings");
            break;
        case ExportType::GIF:
            titleLabel->setText("GIF Export Settings");
            break;
        case ExportType::Image:
            titleLabel->setText("Image Export Settings");
            break;
    }
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Resolution Group
    QGroupBox *resolutionGroup = new QGroupBox("Resolution");
    QFormLayout *resLayout = new QFormLayout(resolutionGroup);
    resLayout->setSpacing(12);

    m_widthSpinBox = new QSpinBox();
    m_widthSpinBox->setRange(128, 7680);  // Up to 8K
    m_widthSpinBox->setValue(m_defaultWidth);
    m_widthSpinBox->setSuffix(" px");
    m_widthSpinBox->setMinimumWidth(120);

    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(128, 4320);
    m_heightSpinBox->setValue(m_defaultHeight);
    m_heightSpinBox->setSuffix(" px");
    m_heightSpinBox->setMinimumWidth(120);

    resLayout->addRow("Width:", m_widthSpinBox);
    resLayout->addRow("Height:", m_heightSpinBox);

    // Preset buttons
    QHBoxLayout *presetLayout = new QHBoxLayout();
    QPushButton *hd720Btn = new QPushButton("720p");
    QPushButton *hd1080Btn = new QPushButton("1080p");
    QPushButton *hd1440Btn = new QPushButton("1440p");
    QPushButton *uhd4kBtn = new QPushButton("4K");
    
    connect(hd720Btn, &QPushButton::clicked, [this]() {
        m_widthSpinBox->setValue(1280);
        m_heightSpinBox->setValue(720);
    });
    connect(hd1080Btn, &QPushButton::clicked, [this]() {
        m_widthSpinBox->setValue(1920);
        m_heightSpinBox->setValue(1080);
    });
    connect(hd1440Btn, &QPushButton::clicked, [this]() {
        m_widthSpinBox->setValue(2560);
        m_heightSpinBox->setValue(1440);
    });
    connect(uhd4kBtn, &QPushButton::clicked, [this]() {
        m_widthSpinBox->setValue(3840);
        m_heightSpinBox->setValue(2160);
    });

    presetLayout->addWidget(hd720Btn);
    presetLayout->addWidget(hd1080Btn);
    presetLayout->addWidget(hd1440Btn);
    presetLayout->addWidget(uhd4kBtn);
    presetLayout->addStretch();
    
    resLayout->addRow("Presets:", presetLayout);
    mainLayout->addWidget(resolutionGroup);

    // Type-specific settings
    if (m_type == ExportType::Video) {
        createVideoSettings();
        mainLayout->addWidget(new QWidget());  // Will be replaced by video settings group
    } else if (m_type == ExportType::GIF) {
        createGIFSettings();
        mainLayout->addWidget(new QWidget());  // Will be replaced by GIF settings group
    } else {
        createImageSettings();
    }

    // Quality Group
    QGroupBox *qualityGroup = new QGroupBox("Quality");
    QVBoxLayout *qualityLayout = new QVBoxLayout(qualityGroup);

    QHBoxLayout *qualityHeaderLayout = new QHBoxLayout();
    QLabel *qualityTitleLabel = new QLabel("Quality:");
    m_qualityLabel = new QLabel("85");
    m_qualityLabel->setObjectName("qualityValue");
    qualityHeaderLayout->addWidget(qualityTitleLabel);
    qualityHeaderLayout->addStretch();
    qualityHeaderLayout->addWidget(m_qualityLabel);
    qualityLayout->addLayout(qualityHeaderLayout);

    m_qualitySlider = new QSlider(Qt::Horizontal);
    m_qualitySlider->setRange(1, 100);
    m_qualitySlider->setValue(85);
    m_qualitySlider->setTickPosition(QSlider::TicksBelow);
    m_qualitySlider->setTickInterval(10);
    connect(m_qualitySlider, &QSlider::valueChanged, [this](int value) {
        m_qualityLabel->setText(QString::number(value));
        updatePreview();
    });
    qualityLayout->addWidget(m_qualitySlider);

    // Quality description
    QLabel *qualityDesc = new QLabel("Lower = smaller file, Higher = better quality");
    qualityDesc->setObjectName("descriptionLabel");
    qualityLayout->addWidget(qualityDesc);

    mainLayout->addWidget(qualityGroup);

    // Background Options
    QGroupBox *bgGroup = new QGroupBox("Background");
    QVBoxLayout *bgLayout = new QVBoxLayout(bgGroup);

    m_transparentBgCheckBox = new QCheckBox("Transparent Background");
    m_transparentBgCheckBox->setChecked(false);
    connect(m_transparentBgCheckBox, &QCheckBox::toggled, this, &ExportDialog::updatePreview);
    bgLayout->addWidget(m_transparentBgCheckBox);

    QLabel *bgDesc = new QLabel("Note: Not all formats support transparency");
    bgDesc->setObjectName("descriptionLabel");
    bgLayout->addWidget(bgDesc);

    mainLayout->addWidget(bgGroup);

    // Preview Info
    m_previewLabel = new QLabel();
    m_previewLabel->setObjectName("previewLabel");
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    updatePreview();
    mainLayout->addWidget(m_previewLabel);

    mainLayout->addStretch();

    // Dialog Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    
    m_resetButton = new QPushButton("Reset to Defaults");
    buttonBox->addButton(m_resetButton, QDialogButtonBox::ResetRole);
    
    connect(m_resetButton, &QPushButton::clicked, [this]() {
        m_widthSpinBox->setValue(m_defaultWidth);
        m_heightSpinBox->setValue(m_defaultHeight);
        if (m_fpsSpinBox) m_fpsSpinBox->setValue(m_defaultFPS);
        m_qualitySlider->setValue(85);
        m_transparentBgCheckBox->setChecked(false);
        updatePreview();
    });

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Connect updates
    connect(m_widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExportDialog::updatePreview);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExportDialog::updatePreview);
}

void ExportDialog::createVideoSettings()
{
    QGroupBox *videoGroup = new QGroupBox("Video Settings");
    QFormLayout *videoLayout = new QFormLayout(videoGroup);

    // FPS
    m_fpsSpinBox = new QSpinBox();
    m_fpsSpinBox->setRange(1, 120);
    m_fpsSpinBox->setValue(m_defaultFPS);
    m_fpsSpinBox->setSuffix(" fps");
    connect(m_fpsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExportDialog::updatePreview);
    videoLayout->addRow("Frame Rate:", m_fpsSpinBox);

    // Format
    m_formatComboBox = new QComboBox();
    m_formatComboBox->addItems({"MP4 (H.264)", "MKV (H.264)", "WebM (VP9)"});
    connect(m_formatComboBox, &QComboBox::currentTextChanged,
            this, &ExportDialog::onFormatChanged);
    videoLayout->addRow("Format:", m_formatComboBox);

    // Frame Range
    QGroupBox *rangeGroup = new QGroupBox("Frame Range");
    QVBoxLayout *rangeLayout = new QVBoxLayout(rangeGroup);

    m_allFramesCheckBox = new QCheckBox("Export All Frames");
    m_allFramesCheckBox->setChecked(true);
    rangeLayout->addWidget(m_allFramesCheckBox);

    QWidget *customRangeWidget = new QWidget();
    QHBoxLayout *customRangeLayout = new QHBoxLayout(customRangeWidget);
    customRangeLayout->setContentsMargins(0, 0, 0, 0);

    m_startFrameSpinBox = new QSpinBox();
    m_startFrameSpinBox->setRange(1, m_maxFrame);
    m_startFrameSpinBox->setValue(1);
    m_startFrameSpinBox->setEnabled(false);

    m_endFrameSpinBox = new QSpinBox();
    m_endFrameSpinBox->setRange(1, m_maxFrame);
    m_endFrameSpinBox->setValue(m_maxFrame);
    m_endFrameSpinBox->setEnabled(false);

    customRangeLayout->addWidget(new QLabel("From:"));
    customRangeLayout->addWidget(m_startFrameSpinBox);
    customRangeLayout->addWidget(new QLabel("To:"));
    customRangeLayout->addWidget(m_endFrameSpinBox);
    customRangeLayout->addStretch();

    rangeLayout->addWidget(customRangeWidget);

    connect(m_allFramesCheckBox, &QCheckBox::toggled, [this](bool checked) {
        m_startFrameSpinBox->setEnabled(!checked);
        m_endFrameSpinBox->setEnabled(!checked);
        updatePreview();
    });

    videoLayout->addRow(rangeGroup);

    // Add to main layout (replace placeholder)
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->insertWidget(2, videoGroup);
    }
}

void ExportDialog::createGIFSettings()
{
    QGroupBox *gifGroup = new QGroupBox("GIF Settings");
    QFormLayout *gifLayout = new QFormLayout(gifGroup);

    // FPS
    m_fpsSpinBox = new QSpinBox();
    m_fpsSpinBox->setRange(1, 60);
    m_fpsSpinBox->setValue(qMin(m_defaultFPS, 30));  // GIFs typically 30fps max
    m_fpsSpinBox->setSuffix(" fps");
    connect(m_fpsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExportDialog::updatePreview);
    gifLayout->addRow("Frame Rate:", m_fpsSpinBox);

    QLabel *fpsNote = new QLabel("Note: GIFs work best at 12-30 fps");
    fpsNote->setObjectName("descriptionLabel");
    gifLayout->addRow("", fpsNote);

    // Export Mode
    m_formatComboBox = new QComboBox();
    m_formatComboBox->addItems({"All Frames", "Keyframes Only"});
    gifLayout->addRow("Export Mode:", m_formatComboBox);

    QLabel *modeNote = new QLabel("Keyframes Only exports only frames with content");
    modeNote->setObjectName("descriptionLabel");
    gifLayout->addRow("", modeNote);

    // Add to main layout
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->insertWidget(2, gifGroup);
    }
}

void ExportDialog::createImageSettings()
{
    QGroupBox *imageGroup = new QGroupBox("Image Settings");
    QFormLayout *imageLayout = new QFormLayout(imageGroup);

    // Format
    m_formatComboBox = new QComboBox();
    m_formatComboBox->addItems({"PNG", "JPEG", "WebP", "TIFF", "BMP"});
    connect(m_formatComboBox, &QComboBox::currentTextChanged,
            this, &ExportDialog::onFormatChanged);
    imageLayout->addRow("Format:", m_formatComboBox);

    // Add to main layout
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->insertWidget(2, imageGroup);
    }
}

void ExportDialog::updatePreview()
{
    int w = m_widthSpinBox->value();
    int h = m_heightSpinBox->value();
    qreal aspectRatio = (qreal)w / h;
    QString aspectStr = QString::number(aspectRatio, 'f', 2);

    QString preview = QString("<b>Output:</b> %1×%2 px (%3:1 aspect)")
                          .arg(w).arg(h).arg(aspectStr);

    if (m_fpsSpinBox) {
        int fps = m_fpsSpinBox->value();
        preview += QString("<br><b>Frame Rate:</b> %1 fps").arg(fps);

        if (m_type == ExportType::Video && !exportAllFrames()) {
            int frames = m_endFrameSpinBox->value() - m_startFrameSpinBox->value() + 1;
            qreal duration = (qreal)frames / fps;
            preview += QString("<br><b>Duration:</b> %.2f seconds (%3 frames)")
                           .arg(duration).arg(frames);
        }
    }

    if (m_transparentBgCheckBox->isChecked()) {
        preview += "<br><b>Background:</b> Transparent";
    } else {
        preview += "<br><b>Background:</b> White";
    }

    m_previewLabel->setText(preview);
}

void ExportDialog::onFormatChanged(const QString &format)
{
    // Update transparency checkbox based on format support
    if (format.contains("JPEG") || format.contains("BMP")) {
        m_transparentBgCheckBox->setEnabled(false);
        m_transparentBgCheckBox->setChecked(false);
    } else {
        m_transparentBgCheckBox->setEnabled(true);
    }
    updatePreview();
}

void ExportDialog::applyModernStyle()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        
        #titleLabel {
            font-size: 18px;
            font-weight: bold;
            color: #2a82da;
            padding: 10px;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 12px;
            background-color: #2d2d2d;
            color: #e0e0e0;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 4px 8px;
            background-color: #2a82da;
            color: white;
            border-radius: 4px;
            left: 10px;
        }
        
        QLabel {
            color: #e0e0e0;
        }
        
        #descriptionLabel {
            color: #a0a0a0;
            font-style: italic;
            font-size: 11px;
        }
        
        #qualityValue {
            color: #2a82da;
            font-weight: bold;
            font-size: 14px;
        }
        
        #previewLabel {
            background-color: #2d2d2d;
            border: 1px solid #2a82da;
            border-radius: 6px;
            padding: 12px;
            color: #e0e0e0;
        }
        
        QSpinBox {
            background-color: #3a3a3a;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px;
            color: #e0e0e0;
            selection-background-color: #2a82da;
        }
        
        QSpinBox:focus {
            border: 2px solid #2a82da;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #4a4a4a;
            border: none;
            width: 20px;
        }
        
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #2a82da;
        }
        
        QComboBox {
            background-color: #3a3a3a;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px;
            color: #e0e0e0;
            min-width: 150px;
        }
        
        QComboBox:focus {
            border: 2px solid #2a82da;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 25px;
        }
        
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #e0e0e0;
            margin-right: 8px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            border: 1px solid #2a82da;
            selection-background-color: #2a82da;
            color: #e0e0e0;
        }
        
        QCheckBox {
            color: #e0e0e0;
            spacing: 8px;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #4a4a4a;
            border-radius: 4px;
            background-color: #3a3a3a;
        }
        
        QCheckBox::indicator:checked {
            background-color: #2a82da;
            border-color: #2a82da;
        }
        
        QCheckBox::indicator:checked:after {
            content: "✓";
            color: white;
        }
        
        QSlider::groove:horizontal {
            height: 8px;
            background-color: #3a3a3a;
            border-radius: 4px;
        }
        
        QSlider::handle:horizontal {
            background-color: #2a82da;
            border: 2px solid #1a5fa5;
            width: 18px;
            height: 18px;
            margin: -6px 0;
            border-radius: 9px;
        }
        
        QSlider::handle:horizontal:hover {
            background-color: #3a92ea;
        }
        
        QSlider::sub-page:horizontal {
            background-color: #2a82da;
            border-radius: 4px;
        }
        
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 16px;
            color: #e0e0e0;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #4a4a4a;
            border-color: #2a82da;
        }
        
        QPushButton:pressed {
            background-color: #2a82da;
        }
        
        QDialogButtonBox QPushButton {
            min-width: 80px;
        }
    )");
}

// Getters
int ExportDialog::width() const { return m_widthSpinBox->value(); }
int ExportDialog::height() const { return m_heightSpinBox->value(); }
int ExportDialog::fps() const { return m_fpsSpinBox ? m_fpsSpinBox->value() : 24; }
int ExportDialog::quality() const { return m_qualitySlider->value(); }
bool ExportDialog::transparentBackground() const { return m_transparentBgCheckBox->isChecked(); }
int ExportDialog::startFrame() const { return m_startFrameSpinBox ? m_startFrameSpinBox->value() : 1; }
int ExportDialog::endFrame() const { return m_endFrameSpinBox ? m_endFrameSpinBox->value() : m_maxFrame; }
bool ExportDialog::exportAllFrames() const { return m_allFramesCheckBox ? m_allFramesCheckBox->isChecked() : true; }

QString ExportDialog::format() const
{
    if (!m_formatComboBox) return "MP4";
    
    QString text = m_formatComboBox->currentText();
    if (text.contains("MP4")) return "mp4";
    if (text.contains("MKV")) return "mkv";
    if (text.contains("WebM")) return "webm";
    if (text.contains("PNG")) return "png";
    if (text.contains("JPEG")) return "jpg";
    if (text.contains("WebP")) return "webp";
    if (text.contains("TIFF")) return "tiff";
    if (text.contains("BMP")) return "bmp";
    if (text.contains("Keyframes")) return "keyframes";
    
    return "mp4";
}

// Setters
void ExportDialog::setDefaultResolution(int width, int height)
{
    m_defaultWidth = width;
    m_defaultHeight = height;
    if (m_widthSpinBox) m_widthSpinBox->setValue(width);
    if (m_heightSpinBox) m_heightSpinBox->setValue(height);
}

void ExportDialog::setDefaultFPS(int fps)
{
    m_defaultFPS = fps;
    if (m_fpsSpinBox) m_fpsSpinBox->setValue(fps);
}

void ExportDialog::setFrameRange(int start, int end)
{
    m_maxFrame = end;
    if (m_startFrameSpinBox) {
        m_startFrameSpinBox->setRange(1, end);
        m_startFrameSpinBox->setValue(start);
    }
    if (m_endFrameSpinBox) {
        m_endFrameSpinBox->setRange(1, end);
        m_endFrameSpinBox->setValue(end);
    }
}
