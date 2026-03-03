// TODO:
// 1. Fix the update failure when changing themes, when the themeis changed is stays Red & Grey till a new tool is selected. So it needs a force update (i think)
// 2. Advanced Mode - When in interpolation settings you cant set the Points (refered to as K-X) to be here by specifc frames, the points dont show up.
// 3. Audio settings - For MiDi files you should be able to pick a synthasizer.


#include "toolsettingspanel.h"
#include "utils/thememanager.h"
#include "tools/texttool.h"
#include "tools/gradienttool.h"
#include "tools/shapetool.h"
#include "tools/linetool.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QScrollArea>
#include <QFileInfo>
#include <QListWidget>
#include <QPushButton>
#include <QColorDialog>
#include <QButtonGroup>

// helpers

static QLabel* makeLabel(const QString &text) {
    QLabel *l = new QLabel(text);
    const auto &t = theme();
    l->setStyleSheet(QString("color:%1; font-size:10px; font-weight: bold;").arg(t.accent));
    return l;
}

static QString sectionStyle() {
    const auto &t = theme();
    return QString(
        "QGroupBox {"
        "  color: %1; font-weight: bold; font-size: 10px; letter-spacing: 1.5px; text-transform: uppercase;"
        "  border: 1px solid %1; border-radius: 4px;"
        "  margin-top: 12px; padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left; padding: 0 8px;"
        "}").arg(t.accent);
}

static QString inputStyle() {
    const auto &t = theme();
    return QString(
        "QSpinBox, QDoubleSpinBox, QComboBox, QFontComboBox {"
        "  background-color: %1; color: white;"
        "  border: 1px solid %2; border-radius: 4px; padding: 4px 6px;"
        "}"
        "QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover { border-color: %3; }"
        "QComboBox::drop-down { border: none; width: 16px; }"
        "QComboBox QAbstractItemView { background:%1; color:white; selection-background-color:%3; }")
        .arg(t.bg4, t.bg2, t.accent);
}

static QString sliderStyle() {
    const auto &t = theme();
    return QString(
        "QSlider::groove:horizontal { background:%1; height:4px; border-radius:2px; }"
        "QSlider::handle:horizontal { background:%2; width:12px; height:12px; margin:-4px 0; border-radius:6px; }"
        "QSlider::handle:horizontal:hover { background:%3; }"
        "QSlider::sub-page:horizontal { background:%2; border-radius:2px; }")
        .arg(t.bg4, t.accent, t.accentHover);
}

static QString checkStyle() {
    const auto &t = theme();
    return QString(
        "QCheckBox { color:#ddd; font-size:11px; spacing:6px; }"
        "QCheckBox::indicator { width:14px; height:14px; border:1px solid %1; border-radius:3px; background:%2; }"
        "QCheckBox::indicator:checked { background:%3; border-color:%3; }")
        .arg(t.bg3, t.bg4, t.accent);
}

static QString toggleStyle() {
    const auto &t = theme();
    return QString(
        "QToolButton { background:%1; color:#aaa; border:1px solid %2; border-radius:4px; font-weight:bold; }"
        "QToolButton:hover { background:%3; }"
        "QToolButton:checked { background:%4; color:white; border-color:%4; }")
        .arg(t.bg2, t.bg3, t.bg3, t.accent);
}

// constructor
ToolSettingsPanel::ToolSettingsPanel(QWidget *parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Header bar
    m_headerWidget = new QWidget();
    m_headerWidget->setFixedHeight(40);
    QHBoxLayout *hl = new QHBoxLayout(m_headerWidget);
    hl->setContentsMargins(12, 0, 12, 0);
    m_titleLabel = new QLabel("TOOL OPTIONS");
    hl->addWidget(m_titleLabel);
    hl->addStretch();
    outer->addWidget(m_headerWidget);

    // Scroll area
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    outer->addWidget(m_scrollArea, 1);

    applyTheme();
    buildEmptyMessage("Select a tool to see its options");
}

void ToolSettingsPanel::clearContent()
{
    if (m_contentWidget) {
        m_scrollArea->takeWidget();
        delete m_contentWidget;
        m_contentWidget = nullptr;
        m_contentLayout = nullptr;
    }
}

void ToolSettingsPanel::buildEmptyMessage(const QString &msg)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    QVBoxLayout *l = new QVBoxLayout(m_contentWidget);
    l->setAlignment(Qt::AlignCenter);
    QLabel *lbl = new QLabel(msg);
    lbl->setStyleSheet(QString("color:%1; font-size:11px;").arg(theme().bg3));
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setWordWrap(true);
    l->addWidget(lbl);
    m_scrollArea->setWidget(m_contentWidget);
}

// updateForTool
void ToolSettingsPanel::updateForTool(ToolType type, Tool *tool)
{
    // Remember what is currently displayed so applyTheme() can force-rebuild it
    // without needing the caller to pass the tool reference again.
    m_lastToolType = type;
    m_lastTool     = tool;

    switch (type) {
    case ToolType::Pencil:
    case ToolType::Brush:
        buildPencilBrushControls(tool);
        break;
    case ToolType::Eraser:
        buildEraserControls(tool);
        break;
    case ToolType::Text:
        buildTextControls(tool);
        break;
    case ToolType::Gradient:
        buildGradientControls(tool);
        break;
    case ToolType::Select:
        buildSelectControls();
        break;
    case ToolType::Rectangle:
    case ToolType::Ellipse:
        buildShapeControls(tool);
        break;
    case ToolType::Line:
        buildLineControls(tool);
        break;
    default:
        buildEmptyMessage("No options for this tool");
        break;
    }
}

// Pencil / Brush
void ToolSettingsPanel::buildPencilBrushControls(Tool *tool)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    // ── Size ─────────────────────────────────────────────
    QGroupBox *sizeGroup = new QGroupBox("Brush Size");
    sizeGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *sg = new QVBoxLayout(sizeGroup);
    sg->setSpacing(6);

    QHBoxLayout *sizeRow = new QHBoxLayout();
    QSlider *sizeSlider = new QSlider(Qt::Horizontal);
    sizeSlider->setRange(1, 200);
    int currentSize = tool ? static_cast<int>(tool->strokeWidth()) : 2;
    sizeSlider->setValue(currentSize);
    sizeSlider->setStyleSheet(sliderStyle());

    QSpinBox *sizeSpin = new QSpinBox();
    sizeSpin->setRange(1, 200);
    sizeSpin->setValue(currentSize);
    sizeSpin->setSuffix(" px");
    sizeSpin->setFixedWidth(64);
    sizeSpin->setStyleSheet(inputStyle());

    connect(sizeSlider, &QSlider::valueChanged, sizeSpin, &QSpinBox::setValue);
    connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), sizeSlider, &QSlider::setValue);
    connect(sizeSlider, &QSlider::valueChanged, this, [tool](int v) {
        if (tool) tool->setStrokeWidth(v);
    });

    sizeRow->addWidget(sizeSlider, 1);
    sizeRow->addWidget(sizeSpin);
    sg->addLayout(sizeRow);
    m_contentLayout->addWidget(sizeGroup);

    // ── Opacity ───────────────────────────────────────────
    QGroupBox *opGroup = new QGroupBox("Opacity");
    opGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *og = new QVBoxLayout(opGroup);
    og->setSpacing(6);

    QHBoxLayout *opRow = new QHBoxLayout();
    QSlider *opSlider = new QSlider(Qt::Horizontal);
    opSlider->setRange(1, 100);
    int currentOpacity = tool ? qRound(tool->strokeOpacity() * 100) : 100;
    opSlider->setValue(currentOpacity);
    opSlider->setStyleSheet(sliderStyle());

    QSpinBox *opSpin = new QSpinBox();
    opSpin->setRange(1, 100);
    opSpin->setValue(currentOpacity);
    opSpin->setSuffix(" %");
    opSpin->setFixedWidth(64);
    opSpin->setStyleSheet(inputStyle());

    connect(opSlider, &QSlider::valueChanged, opSpin, &QSpinBox::setValue);
    connect(opSpin, QOverload<int>::of(&QSpinBox::valueChanged), opSlider, &QSlider::setValue);
    connect(opSlider, &QSlider::valueChanged, this, [tool](int v) {
        if (tool) tool->setStrokeOpacity(v / 100.0);
    });

    opRow->addWidget(opSlider, 1);
    opRow->addWidget(opSpin);
    og->addLayout(opRow);
    m_contentLayout->addWidget(opGroup);

    // ── Texture ───────────────────────────────────────────
    QGroupBox *texGroup = new QGroupBox("Texture");
    texGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *tg = new QVBoxLayout(texGroup);

    QComboBox *texCombo = new QComboBox();
    texCombo->setStyleSheet(inputStyle());
    texCombo->addItem("Solid",    static_cast<int>(ToolTexture::Smooth));
    texCombo->addItem("Grainy",   static_cast<int>(ToolTexture::Grainy));
    texCombo->addItem("Chalk",    static_cast<int>(ToolTexture::Chalk));
    texCombo->addItem("Canvas",   static_cast<int>(ToolTexture::Canvas));
    if (tool) {
        for (int i = 0; i < texCombo->count(); ++i) {
            if (texCombo->itemData(i).toInt() == static_cast<int>(tool->texture())) {
                texCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    connect(texCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [tool, texCombo](int idx) {
        if (tool) tool->setTexture(static_cast<ToolTexture>(texCombo->itemData(idx).toInt()));
    });
    tg->addWidget(texCombo);
    m_contentLayout->addWidget(texGroup);

    // ── Smoothing ─────────────────────────────────────────
    QGroupBox *smGroup = new QGroupBox("Smoothing");
    smGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *smg = new QVBoxLayout(smGroup);
    smg->setSpacing(6);

    smg->addWidget(makeLabel("Amount"));
    QSlider *smSlider = new QSlider(Qt::Horizontal);
    smSlider->setRange(0, 100);
    smSlider->setValue(tool ? tool->smoothingAmount() : 50);
    smSlider->setStyleSheet(sliderStyle());
    connect(smSlider, &QSlider::valueChanged, this, [tool](int v) {
        if (tool) tool->setSmoothingAmount(v);
    });
    smg->addWidget(smSlider);

    QCheckBox *pressureCheck = new QCheckBox("Pressure Sensitivity");
    pressureCheck->setChecked(tool ? tool->pressureSensitive() : true);
    pressureCheck->setStyleSheet(checkStyle());
    connect(pressureCheck, &QCheckBox::toggled, this, [tool](bool on) {
        if (tool) tool->setPressureSensitivity(on);
    });
    smg->addWidget(pressureCheck);
    m_contentLayout->addWidget(smGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// Eraser
void ToolSettingsPanel::buildEraserControls(Tool *tool)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    QGroupBox *sizeGroup = new QGroupBox("Eraser Size");
    sizeGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *sg = new QVBoxLayout(sizeGroup);

    QHBoxLayout *row = new QHBoxLayout();
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 200);
    slider->setValue(tool ? static_cast<int>(tool->strokeWidth()) : 20);
    slider->setStyleSheet(sliderStyle());

    QSpinBox *spin = new QSpinBox();
    spin->setRange(1, 200);
    spin->setValue(slider->value());
    spin->setSuffix(" px");
    spin->setFixedWidth(64);
    spin->setStyleSheet(inputStyle());

    connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
    connect(slider, &QSlider::valueChanged, this, [tool](int v) {
        if (tool) tool->setStrokeWidth(v);
    });

    row->addWidget(slider, 1);
    row->addWidget(spin);
    sg->addLayout(row);
    m_contentLayout->addWidget(sizeGroup);

    QGroupBox *modeGroup = new QGroupBox("Mode");
    modeGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *mg = new QVBoxLayout(modeGroup);
    QComboBox *modeCombo = new QComboBox();
    modeCombo->setStyleSheet(inputStyle());
    modeCombo->addItem("Erase to Transparent");
    modeCombo->addItem("Erase to Background");
    mg->addWidget(modeCombo);
    m_contentLayout->addWidget(modeGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// Text — fully wired to TextTool
void ToolSettingsPanel::buildTextControls(Tool *genericTool)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    TextTool *tool = qobject_cast<TextTool*>(genericTool);

    // ── Font Family ───────────────────────────────────────────────────────────
    QGroupBox *fontGroup = new QGroupBox("Font");
    fontGroup->setStyleSheet(sectionStyle());
    QFormLayout *fl = new QFormLayout(fontGroup);
    fl->setSpacing(8);

    QFontComboBox *fontCombo = new QFontComboBox();
    fontCombo->setStyleSheet(inputStyle());
    if (tool) fontCombo->setCurrentFont(QFont(tool->fontFamily()));
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [tool](const QFont &f) {
        if (tool) tool->setFontFamily(f.family());
    });
    fl->addRow(makeLabel("Family:"), fontCombo);

    // ── Font Size ─────────────────────────────────────────────────────────────
    QSpinBox *sizeSpin = new QSpinBox();
    sizeSpin->setRange(4, 999);
    sizeSpin->setValue(tool ? tool->fontSize() : 48);
    sizeSpin->setSuffix(" pt");
    sizeSpin->setStyleSheet(inputStyle());
    connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [tool](int v) {
        if (tool) tool->setFontSize(v);
    });
    fl->addRow(makeLabel("Size:"), sizeSpin);

    m_contentLayout->addWidget(fontGroup);

    // ── Style Toggles ─────────────────────────────────────────────────────────
    QGroupBox *styleGroup = new QGroupBox("Style");
    styleGroup->setStyleSheet(sectionStyle());
    QHBoxLayout *styleRow = new QHBoxLayout(styleGroup);
    styleRow->setSpacing(6);

    auto makeToggle = [this](const QString &text, const QString &tip) {
        QToolButton *btn = new QToolButton();
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCheckable(true);
        btn->setFixedSize(30, 28);
        btn->setStyleSheet(toggleStyle());
        return btn;
    };

    QToolButton *boldBtn  = makeToggle("B", "Bold");
    QToolButton *italicBtn= makeToggle("I", "Italic");
    QToolButton *underBtn = makeToggle("U", "Underline");

    if (tool) {
        boldBtn->setChecked(tool->bold());
        italicBtn->setChecked(tool->italic());
        underBtn->setChecked(tool->underline());
    }
    boldBtn->setStyleSheet(boldBtn->styleSheet() + " QToolButton:checked { font-weight:bold; }");
    italicBtn->setStyleSheet(italicBtn->styleSheet() + " QToolButton:checked { font-style:italic; }");

    connect(boldBtn,  &QToolButton::toggled, this, [tool](bool b) { if (tool) tool->setBold(b); });
    connect(italicBtn,&QToolButton::toggled, this, [tool](bool b) { if (tool) tool->setItalic(b); });
    connect(underBtn, &QToolButton::toggled, this, [tool](bool b) { if (tool) tool->setUnderline(b); });

    styleRow->addWidget(boldBtn);
    styleRow->addWidget(italicBtn);
    styleRow->addWidget(underBtn);
    styleRow->addStretch();
    m_contentLayout->addWidget(styleGroup);

    // ── Alignment ─────────────────────────────────────────────────────────────
    QGroupBox *alignGroup = new QGroupBox("Alignment");
    alignGroup->setStyleSheet(sectionStyle());
    QHBoxLayout *alignRow = new QHBoxLayout(alignGroup);
    alignRow->setSpacing(6);

    auto makeAlignBtn = [this](const QString &text, const QString &tip) {
        QToolButton *btn = new QToolButton();
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCheckable(true);
        btn->setFixedSize(30, 28);
        btn->setStyleSheet(toggleStyle());
        return btn;
    };

    QToolButton *leftBtn   = makeAlignBtn("⇤", "Align Left");
    QToolButton *centerBtn = makeAlignBtn("⇔", "Align Center");
    QToolButton *rightBtn  = makeAlignBtn("⇥", "Align Right");

    // Only one active at a time
    QButtonGroup *alignGroup2 = new QButtonGroup(alignGroup);
    alignGroup2->setExclusive(true);
    alignGroup2->addButton(leftBtn);
    alignGroup2->addButton(centerBtn);
    alignGroup2->addButton(rightBtn);

    if (tool) {
        if (tool->alignment() == Qt::AlignCenter)     centerBtn->setChecked(true);
        else if (tool->alignment() == Qt::AlignRight)  rightBtn->setChecked(true);
        else                                            leftBtn->setChecked(true);
    } else {
        leftBtn->setChecked(true);
    }

    connect(leftBtn,   &QToolButton::clicked, this, [tool]() { if (tool) tool->setAlignment(Qt::AlignLeft); });
    connect(centerBtn, &QToolButton::clicked, this, [tool]() { if (tool) tool->setAlignment(Qt::AlignCenter); });
    connect(rightBtn,  &QToolButton::clicked, this, [tool]() { if (tool) tool->setAlignment(Qt::AlignRight); });

    alignRow->addWidget(leftBtn);
    alignRow->addWidget(centerBtn);
    alignRow->addWidget(rightBtn);
    alignRow->addStretch();
    m_contentLayout->addWidget(alignGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// Gradient
void ToolSettingsPanel::buildGradientControls(Tool *genericTool)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    GradientTool *tool = qobject_cast<GradientTool*>(genericTool);

    // ── Type ──────────────────────────────────────────────────────────────────
    QGroupBox *typeGroup = new QGroupBox("Gradient Type");
    typeGroup->setStyleSheet(sectionStyle());
    QHBoxLayout *typeRow = new QHBoxLayout(typeGroup);
    typeRow->setSpacing(6);

    QToolButton *linearBtn = new QToolButton();
    linearBtn->setText("⟶  Linear");
    linearBtn->setCheckable(true);
    linearBtn->setChecked(!tool || tool->gradientType() == GradientTool::Linear);
    linearBtn->setFixedHeight(28);
    linearBtn->setStyleSheet(toggleStyle());

    QToolButton *radialBtn = new QToolButton();
    radialBtn->setText("◎  Radial");
    radialBtn->setCheckable(true);
    radialBtn->setChecked(tool && tool->gradientType() == GradientTool::Radial);
    radialBtn->setFixedHeight(28);
    radialBtn->setStyleSheet(toggleStyle());

    QButtonGroup *typeBG = new QButtonGroup(typeGroup);
    typeBG->setExclusive(true);
    typeBG->addButton(linearBtn, 0);
    typeBG->addButton(radialBtn, 1);

    connect(linearBtn, &QToolButton::clicked, this, [tool](){ if (tool) tool->setGradientType(GradientTool::Linear); });
    connect(radialBtn, &QToolButton::clicked, this, [tool](){ if (tool) tool->setGradientType(GradientTool::Radial); });

    typeRow->addWidget(linearBtn, 1);
    typeRow->addWidget(radialBtn, 1);
    m_contentLayout->addWidget(typeGroup);

    // ── Colors ────────────────────────────────────────────────────────────────
    QGroupBox *colorGroup = new QGroupBox("Colors");
    colorGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *cg = new QVBoxLayout(colorGroup);
    cg->setSpacing(8);

    // Start color swatch
    QHBoxLayout *startRow = new QHBoxLayout();
    startRow->addWidget(makeLabel("Start:"));
    QPushButton *startSwatch = new QPushButton();
    QColor startCol = tool ? tool->startColor() : Qt::white;
    startSwatch->setFixedSize(48, 24);
    startSwatch->setStyleSheet(QString("background:%1; border:1px solid #555; border-radius:3px;").arg(startCol.name(QColor::HexArgb)));
    startSwatch->setToolTip("Click to change start color");
    connect(startSwatch, &QPushButton::clicked, this, [tool, startSwatch]() {
        QColor c = QColorDialog::getColor(tool ? tool->startColor() : Qt::white, nullptr,
                                          "Start Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            if (tool) tool->setStartColor(c);
            startSwatch->setStyleSheet(QString("background:%1; border:1px solid #555; border-radius:3px;").arg(c.name(QColor::HexArgb)));
        }
    });
    startRow->addWidget(startSwatch);
    startRow->addStretch();
    cg->addLayout(startRow);

    // End color swatch
    QHBoxLayout *endRow = new QHBoxLayout();
    endRow->addWidget(makeLabel("End:"));
    QPushButton *endSwatch = new QPushButton();
    QColor endCol = tool ? tool->endColor() : QColor(255,255,255,0);
    endSwatch->setFixedSize(48, 24);
    endSwatch->setStyleSheet(QString("background:%1; border:1px solid #555; border-radius:3px;").arg(endCol.name(QColor::HexArgb)));
    endSwatch->setToolTip("Click to change end color");
    connect(endSwatch, &QPushButton::clicked, this, [tool, endSwatch]() {
        QColor c = QColorDialog::getColor(tool ? tool->endColor() : QColor(255,255,255,0),
                                          nullptr, "End Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            if (tool) tool->setEndColor(c);
            endSwatch->setStyleSheet(QString("background:%1; border:1px solid #555; border-radius:3px;").arg(c.name(QColor::HexArgb)));
        }
    });
    endRow->addWidget(endSwatch);
    endRow->addStretch();
    cg->addLayout(endRow);

    m_contentLayout->addWidget(colorGroup);

    // ── Options ───────────────────────────────────────────────────────────────
    QGroupBox *optGroup = new QGroupBox("Options");
    optGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *og = new QVBoxLayout(optGroup);

    QCheckBox *repeatCheck = new QCheckBox("Repeat (tile gradient)");
    repeatCheck->setChecked(tool && tool->repeat());
    repeatCheck->setStyleSheet(checkStyle());
    connect(repeatCheck, &QCheckBox::toggled, this, [tool](bool b) { if (tool) tool->setRepeat(b); });
    og->addWidget(repeatCheck);

    QLabel *hint = new QLabel("Hold Shift while dragging\nto snap to 45° angles.");
    hint->setStyleSheet(QString("color:%1; font-size:10px;").arg(theme().bg3));
    hint->setWordWrap(true);
    og->addWidget(hint);

    m_contentLayout->addWidget(optGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// Select
void ToolSettingsPanel::buildSelectControls()
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    QGroupBox *transformGroup = new QGroupBox("Transform");
    transformGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *tg = new QVBoxLayout(transformGroup);
    tg->setSpacing(6);

    QFormLayout *tf = new QFormLayout();
    tf->setSpacing(6);

    QDoubleSpinBox *xSpin = new QDoubleSpinBox();
    xSpin->setRange(-9999, 9999); xSpin->setSuffix(" px"); xSpin->setStyleSheet(inputStyle());
    QDoubleSpinBox *ySpin = new QDoubleSpinBox();
    ySpin->setRange(-9999, 9999); ySpin->setSuffix(" px"); ySpin->setStyleSheet(inputStyle());
    QDoubleSpinBox *wSpin = new QDoubleSpinBox();
    wSpin->setRange(0, 9999); wSpin->setSuffix(" px"); wSpin->setStyleSheet(inputStyle());
    QDoubleSpinBox *hSpin = new QDoubleSpinBox();
    hSpin->setRange(0, 9999); hSpin->setSuffix(" px"); hSpin->setStyleSheet(inputStyle());
    QDoubleSpinBox *rotSpin = new QDoubleSpinBox();
    rotSpin->setRange(-360, 360); rotSpin->setSuffix(" °"); rotSpin->setStyleSheet(inputStyle());

    tf->addRow(makeLabel("X:"), xSpin);
    tf->addRow(makeLabel("Y:"), ySpin);
    tf->addRow(makeLabel("W:"), wSpin);
    tf->addRow(makeLabel("H:"), hSpin);
    tf->addRow(makeLabel("Rotation:"), rotSpin);

    tg->addLayout(tf);
    m_contentLayout->addWidget(transformGroup);
    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// Shape
void ToolSettingsPanel::buildShapeControls(Tool *tool)
{
    ShapeTool *shapeTool = qobject_cast<ShapeTool*>(tool);

    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    // ── Stroke Width ───────────────────────────────────────────────────────────
    QGroupBox *strokeGroup = new QGroupBox("Stroke");
    strokeGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *sg = new QVBoxLayout(strokeGroup);

    QHBoxLayout *row = new QHBoxLayout();
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 50); slider->setValue(2); slider->setStyleSheet(sliderStyle());
    QSpinBox *spin = new QSpinBox();
    spin->setRange(0, 50); spin->setValue(2); spin->setSuffix(" px"); spin->setFixedWidth(64);
    spin->setStyleSheet(inputStyle());
    connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
    connect(slider, &QSlider::valueChanged, this, [shapeTool](int v) {
        if (shapeTool) shapeTool->setStrokeWidth(v);
    });
    row->addWidget(slider, 1); row->addWidget(spin);
    sg->addLayout(row);

    // FIX #18: Stroke color swatch (separate from fill)
    QHBoxLayout *strokeColorRow = new QHBoxLayout();
    strokeColorRow->addWidget(makeLabel("Stroke Color:"));
    QPushButton *strokeColorBtn = new QPushButton();
    strokeColorBtn->setFixedSize(40, 24);
    QColor initialStroke = shapeTool ? shapeTool->strokeColor() : Qt::black;
    strokeColorBtn->setStyleSheet(QString(
        "QPushButton { background:%1; border:1px solid #666; border-radius:3px; }"
        "QPushButton:hover { border-color:white; }").arg(initialStroke.name()));
    connect(strokeColorBtn, &QPushButton::clicked, this, [this, strokeColorBtn, shapeTool]() {
        QColor c = QColorDialog::getColor(
            shapeTool ? shapeTool->strokeColor() : Qt::black,
            this, "Stroke Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            if (shapeTool) shapeTool->setStrokeColor(c);
            strokeColorBtn->setStyleSheet(QString(
                "QPushButton { background:%1; border:1px solid #666; border-radius:3px; }"
                "QPushButton:hover { border-color:white; }").arg(c.name()));
        }
    });
    strokeColorRow->addWidget(strokeColorBtn);
    strokeColorRow->addStretch();
    sg->addLayout(strokeColorRow);
    m_contentLayout->addWidget(strokeGroup);

    // ── Fill ───────────────────────────────────────────────────────────────────
    QGroupBox *fillGroup = new QGroupBox("Fill");
    fillGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *fg = new QVBoxLayout(fillGroup);

    QCheckBox *fillCheck = new QCheckBox("Fill Shape");
    fillCheck->setChecked(shapeTool ? shapeTool->fillByDefault() : true);
    fillCheck->setStyleSheet(checkStyle());
    connect(fillCheck, &QCheckBox::toggled, this, [shapeTool](bool checked) {
        if (shapeTool) shapeTool->setFillByDefault(checked);
    });
    fg->addWidget(fillCheck);

    // FIX #18: Fill color swatch (separate from stroke color)
    QHBoxLayout *fillColorRow = new QHBoxLayout();
    fillColorRow->addWidget(makeLabel("Fill Color:"));
    QPushButton *fillColorBtn = new QPushButton();
    fillColorBtn->setFixedSize(40, 24);
    QColor initialFill = shapeTool ? shapeTool->shapeFillColor() : QColor(100, 149, 237);
    fillColorBtn->setStyleSheet(QString(
        "QPushButton { background:%1; border:1px solid #666; border-radius:3px; }"
        "QPushButton:hover { border-color:white; }").arg(initialFill.name()));
    connect(fillColorBtn, &QPushButton::clicked, this, [this, fillColorBtn, shapeTool, fillCheck]() {
        QColor c = QColorDialog::getColor(
            shapeTool ? shapeTool->shapeFillColor() : QColor(100, 149, 237),
            this, "Fill Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            if (shapeTool) shapeTool->setShapeFillColor(c);
            fillColorBtn->setStyleSheet(QString(
                "QPushButton { background:%1; border:1px solid #666; border-radius:3px; }"
                "QPushButton:hover { border-color:white; }").arg(c.name()));
            // Enable fill automatically if user picks a color
            fillCheck->setChecked(true);
        }
    });
    fillColorRow->addWidget(fillColorBtn);
    fillColorRow->addStretch();
    fg->addLayout(fillColorRow);

    // Corner style (for rectangle)
    QComboBox *cornerCombo = new QComboBox();
    cornerCombo->setStyleSheet(inputStyle());
    cornerCombo->addItem("Sharp corners");
    cornerCombo->addItem("Rounded corners");
    connect(cornerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [shapeTool](int idx) {
        if (shapeTool) shapeTool->setRoundedCorners(idx == 1);
    });
    fg->addWidget(makeLabel("Corner style:"));
    fg->addWidget(cornerCombo);
    m_contentLayout->addWidget(fillGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// Line
void ToolSettingsPanel::buildLineControls(Tool *tool)
{
    LineTool *lineTool = qobject_cast<LineTool*>(tool);

    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    QGroupBox *lineGroup = new QGroupBox("Line Style");
    lineGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *lg = new QVBoxLayout(lineGroup);
    lg->setSpacing(8);

    // Width
    QHBoxLayout *row = new QHBoxLayout();
    QSlider *slider = new QSlider(Qt::Horizontal);
    int initW = lineTool ? static_cast<int>(lineTool->strokeWidth()) : 2;
    slider->setRange(1, 50); slider->setValue(initW); slider->setStyleSheet(sliderStyle());
    QSpinBox *spin = new QSpinBox();
    spin->setRange(1, 50); spin->setValue(initW); spin->setSuffix(" px"); spin->setFixedWidth(64);
    spin->setStyleSheet(inputStyle());
    connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
    connect(slider, &QSlider::valueChanged, this, [lineTool](int v) {
        if (lineTool) lineTool->setStrokeWidth(v);
    });
    row->addWidget(makeLabel("Width:")); row->addWidget(slider, 1); row->addWidget(spin);
    lg->addLayout(row);

    // FIX #19: Dash pattern — actually connected to the tool
    lg->addWidget(makeLabel("Pattern:"));
    QComboBox *dashCombo = new QComboBox();
    dashCombo->setStyleSheet(inputStyle());
    dashCombo->addItem("Solid",  static_cast<int>(PathDashStyle::Solid));
    dashCombo->addItem("Dashed", static_cast<int>(PathDashStyle::Dashed));
    dashCombo->addItem("Dotted", static_cast<int>(PathDashStyle::Dotted));
    // Restore current selection
    if (lineTool) {
        int idx = static_cast<int>(lineTool->lineDashStyle());
        dashCombo->setCurrentIndex(idx);
    }
    connect(dashCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [lineTool, dashCombo](int /*idx*/) {
        if (lineTool)
            lineTool->setLineDashStyle(static_cast<PathDashStyle>(dashCombo->currentData().toInt()));
    });
    lg->addWidget(dashCombo);

    // FIX #19: Arrow checkbox — actually connected to the tool
    QCheckBox *arrowCheck = new QCheckBox("Arrow at end");
    arrowCheck->setChecked(lineTool ? lineTool->lineArrowAtEnd() : false);
    arrowCheck->setStyleSheet(checkStyle());
    connect(arrowCheck, &QCheckBox::toggled, this, [lineTool](bool checked) {
        if (lineTool) lineTool->setLineArrowAtEnd(checked);
    });
    lg->addWidget(arrowCheck);
    m_contentLayout->addWidget(lineGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}


//Interpolation Mode Controls
void ToolSettingsPanel::showInterpolationControls(int totalFrames)
{
    m_interpolating = true;
    m_interpTotalFrames = totalFrames;
    m_interpNodeCount = 0;
    buildInterpolationControls(totalFrames);
}

void ToolSettingsPanel::hideInterpolationControls()
{
    m_interpolating = false;
    m_interpModeCombo  = nullptr;
    m_interpAdvGroup   = nullptr;
    m_interpFramesSpin = nullptr;
    m_interpNodeSpins.clear();
    buildSelectControls();
}

void ToolSettingsPanel::updateInterpolationNodes(int nodeCount)
{
    if (!m_interpolating || nodeCount == m_interpNodeCount) return;
    m_interpNodeCount = nodeCount;

    if (!m_interpAdvGroup || !m_interpModeCombo || m_interpModeCombo->currentIndex() != 1) return;

    // Clear the advanced group layout
    m_interpNodeSpins.clear();
    QLayout *av = m_interpAdvGroup->layout();
    if (!av) return;
    QLayoutItem *item;
    while ((item = av->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        if (item->layout()) {
            QLayoutItem *sub;
            while ((sub = item->layout()->takeAt(0)) != nullptr) {
                if (sub->widget()) sub->widget()->deleteLater();
                delete sub;
            }
        }
        delete item;
    }

    if (nodeCount < 2) {
        QLabel *hint = new QLabel("Place at least 2 nodes.");
        hint->setStyleSheet(QString("color:%1; font-size:10px;").arg(theme().bg3));
        hint->setAlignment(Qt::AlignCenter);
        av->addWidget(hint);
        return;
    }

    QFormLayout *form = new QFormLayout();
    form->setSpacing(4);
    int frameStep = qMax(1, m_interpTotalFrames / (nodeCount - 1));
    for (int i = 0; i < nodeCount; ++i) {
        QSpinBox *sp = new QSpinBox();
        sp->setRange(0, 9999);
        sp->setValue(i * frameStep);
        sp->setSuffix(" f");
        sp->setStyleSheet(
            inputStyle());
        m_interpNodeSpins.append(sp);
        form->addRow(makeLabel(QString("K%1:").arg(i + 1)), sp);
        connect(sp, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
            QList<int> times;
            for (QSpinBox *s : m_interpNodeSpins) times.append(s->value());
            emit interpolationSettingsChanged(m_interpTotalFrames, true, times);
        });
    }
    av->addItem(form);

    QList<int> times;
    for (QSpinBox *s : m_interpNodeSpins) times.append(s->value());
    emit interpolationSettingsChanged(m_interpTotalFrames, true, times);
}

void ToolSettingsPanel::buildInterpolationControls(int totalFrames)
{
    clearContent();
    m_interpNodeSpins.clear();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    QLabel *header = new QLabel("✦  MOTION PATH");
    header->setStyleSheet("color:#8b5cf6; font-size:10px; font-weight:bold; letter-spacing:2px;");
    m_contentLayout->addWidget(header);

    QGroupBox *modeGroup = new QGroupBox("Mode");
    modeGroup->setStyleSheet(
        "QGroupBox { color:#8b5cf6; font-weight:bold; font-size:10px; letter-spacing:1px;"
        " border:1px solid #8b5cf6; border-radius:4px; margin-top:12px; padding-top:10px; }"
        "QGroupBox::title { subcontrol-origin:margin; subcontrol-position:top left; padding:0 8px; }");
    QVBoxLayout *mg = new QVBoxLayout(modeGroup);
    m_interpModeCombo = new QComboBox();
    m_interpModeCombo->setStyleSheet(inputStyle());
    m_interpModeCombo->addItem("Basic  —  Total Frames");
    m_interpModeCombo->addItem("Advanced  —  Per Node");
    mg->addWidget(m_interpModeCombo);
    m_contentLayout->addWidget(modeGroup);

    QGroupBox *basicGroup = new QGroupBox("Duration");
    basicGroup->setStyleSheet(
        "QGroupBox { color:#8b5cf6; font-weight:bold; font-size:10px; letter-spacing:1px;"
        " border:1px solid #8b5cf6; border-radius:4px; margin-top:12px; padding-top:10px; }"
        "QGroupBox::title { subcontrol-origin:margin; subcontrol-position:top left; padding:0 8px; }");
    QFormLayout *bf = new QFormLayout(basicGroup);
    bf->setSpacing(8);
    m_interpFramesSpin = new QSpinBox();
    m_interpFramesSpin->setRange(2, 999);
    m_interpFramesSpin->setValue(totalFrames);
    m_interpFramesSpin->setSuffix(" frames");
    m_interpFramesSpin->setStyleSheet(inputStyle());
    bf->addRow(makeLabel("Total:"), m_interpFramesSpin);
    m_contentLayout->addWidget(basicGroup);

    m_interpAdvGroup = new QGroupBox("Per-Node Frame Times");
    m_interpAdvGroup->setStyleSheet(
        "QGroupBox { color:#8b5cf6; font-weight:bold; font-size:10px; letter-spacing:1px;"
        " border:1px solid #8b5cf6; border-radius:4px; margin-top:12px; padding-top:10px; }"
        "QGroupBox::title { subcontrol-origin:margin; subcontrol-position:top left; padding:0 8px; }");
    QVBoxLayout *av = new QVBoxLayout(m_interpAdvGroup);
    av->setSpacing(4);
    QLabel *advHint = new QLabel("Place nodes on canvas.\nFrame times appear here.");
    advHint->setStyleSheet(QString("color:%1; font-size:10px;").arg(theme().bg3));
    advHint->setAlignment(Qt::AlignCenter);
    advHint->setWordWrap(true);
    av->addWidget(advHint);
    m_contentLayout->addWidget(m_interpAdvGroup);
    m_interpAdvGroup->hide();

    QLabel *tip = new QLabel("Draw path on canvas,\nthen press Enter to commit.");
    tip->setStyleSheet(QString("color:%1; font-size:10px;").arg(theme().bg3));
    tip->setAlignment(Qt::AlignCenter);
    tip->setWordWrap(true);
    m_contentLayout->addWidget(tip);
    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);

    connect(m_interpModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [=](int idx) {
        basicGroup->setVisible(idx == 0);
        m_interpAdvGroup->setVisible(idx == 1);
        if (idx == 1 && m_interpNodeCount >= 2) {
            int saved = m_interpNodeCount;
            m_interpNodeCount = 0;
            updateInterpolationNodes(saved);
        }
        QList<int> times;
        for (QSpinBox *s : m_interpNodeSpins) times.append(s->value());
        emit interpolationSettingsChanged(m_interpFramesSpin->value(),
                                          idx == 1, times);
    });

    connect(m_interpFramesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [=](int val) {
        m_interpTotalFrames = val;
        QList<int> times;
        for (QSpinBox *s : m_interpNodeSpins) times.append(s->value());
        emit interpolationSettingsChanged(val, m_interpModeCombo->currentIndex() == 1, times);
    });

    emit interpolationSettingsChanged(totalFrames, false, {});
}


// Audio Layer Controls
void ToolSettingsPanel::showAudioLayerControls(Layer *layer)
{
    m_audioLayer = layer;
    buildAudioControls(layer);
}

void ToolSettingsPanel::buildAudioControls(Layer *layer)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color:%1;").arg(theme().bg1));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(10);

    if (!layer) {
        buildEmptyMessage("No audio layer selected");
        return;
    }

    // ── Clips list ────────────────────────────────────────
    QGroupBox *clipsGroup = new QGroupBox("Audio Clips");
    clipsGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *cg = new QVBoxLayout(clipsGroup);
    cg->setSpacing(4);

    const auto &clips = layer->audioClips();

    if (clips.isEmpty()) {
        QLabel *empty = new QLabel("No clips. Right-click the layer\nin the timeline to add audio.");
        empty->setStyleSheet(QString("color:%1; font-size:10px;").arg(theme().bg3));
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignCenter);
        cg->addWidget(empty);
    }

    for (int i = 0; i < clips.size(); ++i) {
        const AudioData &clip = clips[i];
        QFrame *clipFrame = new QFrame();
        clipFrame->setStyleSheet(QString(
            "QFrame { background:%1; border:1px solid %2; border-radius:4px; }")
            .arg(theme().bg2, theme().bg3));
        QVBoxLayout *cf = new QVBoxLayout(clipFrame);
        cf->setContentsMargins(8, 6, 8, 6);
        cf->setSpacing(4);

        // Clip name
        QString name = clip.isMidi
            ? "🎹 " + QFileInfo(clip.filePath).fileName()
            : "🎵 " + QFileInfo(clip.filePath).fileName();
        QLabel *nameLbl = new QLabel(name);
        nameLbl->setStyleSheet("color:white; font-size:10px; font-weight:bold;");
        nameLbl->setWordWrap(true);
        cf->addWidget(nameLbl);

        // Start frame
        QHBoxLayout *startRow = new QHBoxLayout();
        startRow->addWidget(makeLabel("Start:"));
        QSpinBox *startSpin = new QSpinBox();
        startSpin->setRange(1, 99999);
        startSpin->setValue(clip.startFrame);
        startSpin->setStyleSheet(inputStyle());
        startSpin->setFixedWidth(72);
        startRow->addWidget(startSpin);
        startRow->addStretch();
        cf->addLayout(startRow);

        // Duration
        QHBoxLayout *durRow = new QHBoxLayout();
        durRow->addWidget(makeLabel("Duration (frames):"));
        QSpinBox *durSpin = new QSpinBox();
        durSpin->setRange(-1, 99999);
        durSpin->setSpecialValueText("Auto");
        durSpin->setValue(clip.durationFrames);
        durSpin->setStyleSheet(inputStyle());
        durSpin->setFixedWidth(72);
        durSpin->setToolTip("-1 = use natural audio length");
        durRow->addWidget(durSpin);
        durRow->addStretch();
        cf->addLayout(durRow);

        // Volume
        QHBoxLayout *volRow = new QHBoxLayout();
        volRow->addWidget(makeLabel("Vol:"));
        QSlider *volSlider = new QSlider(Qt::Horizontal);
        volSlider->setRange(0, 100);
        volSlider->setValue(qRound(clip.volume * 100));
        volSlider->setStyleSheet(sliderStyle());
        QSpinBox *volSpin = new QSpinBox();
        volSpin->setRange(0, 100);
        volSpin->setValue(volSlider->value());
        volSpin->setSuffix("%");
        volSpin->setFixedWidth(56);
        volSpin->setStyleSheet(inputStyle());
        connect(volSlider, &QSlider::valueChanged, volSpin, &QSpinBox::setValue);
        connect(volSpin, QOverload<int>::of(&QSpinBox::valueChanged), volSlider, &QSlider::setValue);
        volRow->addWidget(volSlider, 1);
        volRow->addWidget(volSpin);
        cf->addLayout(volRow);

        // Mute
        QCheckBox *muteCheck = new QCheckBox("Mute this clip");
        muteCheck->setChecked(clip.muted);
        muteCheck->setStyleSheet(checkStyle());
        cf->addWidget(muteCheck);

        // Wire changes back to layer
        int clipIdx = i;
        auto applyClipChanges = [layer, clipIdx, startSpin, durSpin, volSpin, muteCheck]() {
            if (clipIdx >= layer->audioClips().size()) return;
            AudioData c = layer->audioClips()[clipIdx];
            c.startFrame     = startSpin->value();
            c.durationFrames = durSpin->value();
            c.volume         = volSpin->value() / 100.0f;
            c.muted          = muteCheck->isChecked();
            layer->setAudioClip(clipIdx, c);
        };

        connect(startSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [applyClipChanges](int){ applyClipChanges(); });
        connect(durSpin,   QOverload<int>::of(&QSpinBox::valueChanged), this, [applyClipChanges](int){ applyClipChanges(); });
        connect(volSpin,   QOverload<int>::of(&QSpinBox::valueChanged), this, [applyClipChanges](int){ applyClipChanges(); });
        connect(muteCheck, &QCheckBox::toggled, this, [applyClipChanges](bool){ applyClipChanges(); });

        cg->addWidget(clipFrame);
    }

    m_contentLayout->addWidget(clipsGroup);
    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);

    // Update header
    if (m_titleLabel)
        m_titleLabel->setText("AUDIO OPTIONS");
}

void ToolSettingsPanel::applyTheme()
{
    const ThemeColors &t = theme();

    setStyleSheet(QString("background-color: %1; border: none;").arg(t.bg0));

    if (m_headerWidget)
        m_headerWidget->setStyleSheet(
            QString("background-color: %1; border-bottom: 1px solid %2;")
            .arg(t.bg0, t.bg2));

    if (m_titleLabel)
        m_titleLabel->setStyleSheet(
            QString("color: %1; font-weight: bold; font-size: 10px; letter-spacing: 2px;")
            .arg(t.accent));

    m_scrollArea->setStyleSheet(
        QString("QScrollArea { border:none; background:%1; }"
                "QScrollBar:vertical { background:%1; width:6px; }"
                "QScrollBar::handle:vertical { background:%2; border-radius:3px; min-height:20px; }"
                "QScrollBar::handle:vertical:hover { background:%3; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0px; }")
        .arg(t.bg0, t.bg2, t.accent));

    // ── Force-rebuild the panel content so every widget picks up the new
    //    accent/background colours immediately.
    //    Fixes TODO #1: "stays Red & Grey until you switch tools".
    if (m_interpolating) {
        buildInterpolationControls(m_interpTotalFrames);
        if (m_interpNodeCount > 0) {
            int saved = m_interpNodeCount;
            m_interpNodeCount = 0;
            updateInterpolationNodes(saved);
        }
    } else if (m_audioLayer) {
        buildAudioControls(m_audioLayer);
    } else if (m_lastToolType != ToolType::None) {
        updateForTool(m_lastToolType, m_lastTool);
    } else if (m_contentWidget) {
        // Generic fallback: re-apply stylesheets on every child widget
        m_contentWidget->setStyleSheet(
            QString("background-color:%1;").arg(t.bg1));
        for (QGroupBox     *w : m_contentWidget->findChildren<QGroupBox*>())
            w->setStyleSheet(sectionStyle());
        for (QSlider       *w : m_contentWidget->findChildren<QSlider*>())
            w->setStyleSheet(sliderStyle());
        for (QSpinBox      *w : m_contentWidget->findChildren<QSpinBox*>())
            w->setStyleSheet(inputStyle());
        for (QDoubleSpinBox *w : m_contentWidget->findChildren<QDoubleSpinBox*>())
            w->setStyleSheet(inputStyle());
        for (QComboBox     *w : m_contentWidget->findChildren<QComboBox*>())
            w->setStyleSheet(inputStyle());
        for (QCheckBox     *w : m_contentWidget->findChildren<QCheckBox*>())
            w->setStyleSheet(checkStyle());
        for (QToolButton   *w : m_contentWidget->findChildren<QToolButton*>())
            w->setStyleSheet(toggleStyle());
    }
}
