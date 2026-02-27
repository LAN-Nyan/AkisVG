#include "toolsettingspanel.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QScrollArea>

// ── helpers ───────────────────────────────────────────────
static QString sectionStyle() {
    return
        "QGroupBox {"
        "  color: #aaa; font-weight: bold; font-size: 10px; letter-spacing: 0.5px;"
        "  border: 1px solid #3a3a3a; border-radius: 5px;"
        "  margin-top: 8px; padding-top: 6px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left; padding: 0 6px;"
        "}";
}

static QString inputStyle() {
    return
        "QSpinBox, QDoubleSpinBox, QComboBox {"
        "  background-color: #1e1e1e; color: white;"
        "  border: 1px solid #444; border-radius: 4px; padding: 3px 6px;"
        "}"
        "QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover { border-color: #2a82da; }"
        "QComboBox::drop-down { border: none; width: 16px; }"
        "QComboBox QAbstractItemView { background:#2d2d2d; color:white; selection-background-color:#2a82da; }";
}

static QString sliderStyle() {
    return
        "QSlider::groove:horizontal { background:#1e1e1e; height:4px; border-radius:2px; }"
        "QSlider::handle:horizontal { background:#2a82da; width:12px; margin:-4px 0; border-radius:6px; }"
        "QSlider::handle:horizontal:hover { background:#3a92ea; }"
        "QSlider::sub-page:horizontal { background:#2a82da; border-radius:2px; }";
}

static QLabel* makeLabel(const QString &text) {
    QLabel *l = new QLabel(text);
    l->setStyleSheet("color:#aaa; font-size:10px;");
    return l;
}

// ── constructor ───────────────────────────────────────────
ToolSettingsPanel::ToolSettingsPanel(QWidget *parent) : QFrame(parent)
{
    setStyleSheet("background-color: #2d2d2d; color: white;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Header bar
    QWidget *header = new QWidget();
    header->setStyleSheet("background-color:#3a3a3a; border-bottom:1px solid #000;");
    header->setFixedHeight(36);
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);
    QLabel *title = new QLabel("TOOL OPTIONS");
    title->setStyleSheet("color:white; font-weight:bold; font-size:10px; letter-spacing:1px;");
    hl->addWidget(title);
    hl->addStretch();
    outer->addWidget(header);

    // Scroll area
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border:none; background:transparent; }"
        "QScrollBar:vertical { background:#1a1a1a; width:8px; border-radius:4px; }"
        "QScrollBar::handle:vertical { background:#444; border-radius:4px; min-height:20px; }"
        "QScrollBar::handle:vertical:hover { background:#2a82da; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0px; }"
    );
    outer->addWidget(m_scrollArea, 1);

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
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
    QVBoxLayout *l = new QVBoxLayout(m_contentWidget);
    l->setAlignment(Qt::AlignCenter);
    QLabel *lbl = new QLabel(msg);
    lbl->setStyleSheet("color:#555; font-size:11px;");
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setWordWrap(true);
    l->addWidget(lbl);
    m_scrollArea->setWidget(m_contentWidget);
}

// ── updateForTool ─────────────────────────────────────────
void ToolSettingsPanel::updateForTool(ToolType type, Tool *tool)
{
    switch (type) {
    case ToolType::Pencil:
    case ToolType::Brush:
        buildPencilBrushControls(tool);
        break;
    case ToolType::Eraser:
        buildEraserControls(tool);
        break;
    case ToolType::Text:
        buildTextControls();
        break;
    case ToolType::Select:
        buildSelectControls();
        break;
    case ToolType::Rectangle:
    case ToolType::Ellipse:
        buildShapeControls();
        break;
    case ToolType::Line:
        buildLineControls();
        break;
    default:
        buildEmptyMessage("No options for this tool");
        break;
    }
}

// ── Pencil / Brush ────────────────────────────────────────
void ToolSettingsPanel::buildPencilBrushControls(Tool *tool)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
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
    opSlider->setValue(100);
    opSlider->setStyleSheet(sliderStyle());

    QSpinBox *opSpin = new QSpinBox();
    opSpin->setRange(1, 100);
    opSpin->setValue(100);
    opSpin->setSuffix(" %");
    opSpin->setFixedWidth(64);
    opSpin->setStyleSheet(inputStyle());

    connect(opSlider, &QSlider::valueChanged, opSpin, &QSpinBox::setValue);
    connect(opSpin, QOverload<int>::of(&QSpinBox::valueChanged), opSlider, &QSlider::setValue);

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
    smSlider->setValue(50);
    smSlider->setStyleSheet(sliderStyle());
    smg->addWidget(smSlider);

    QCheckBox *pressureCheck = new QCheckBox("Simulate Pressure");
    pressureCheck->setStyleSheet(
        "QCheckBox { color:#ccc; font-size:11px; spacing:6px; }"
        "QCheckBox::indicator { width:14px; height:14px; border:1px solid #555; border-radius:3px; background:#1e1e1e; }"
        "QCheckBox::indicator:checked { background:#2a82da; border-color:#2a82da; }"
    );
    smg->addWidget(pressureCheck);
    m_contentLayout->addWidget(smGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// ── Eraser ────────────────────────────────────────────────
void ToolSettingsPanel::buildEraserControls(Tool *tool)
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
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

// ── Text ──────────────────────────────────────────────────
void ToolSettingsPanel::buildTextControls()
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    QGroupBox *fontGroup = new QGroupBox("Font");
    fontGroup->setStyleSheet(sectionStyle());
    QFormLayout *fl = new QFormLayout(fontGroup);
    fl->setSpacing(8);

    QFontComboBox *fontCombo = new QFontComboBox();
    fontCombo->setStyleSheet(inputStyle());
    fl->addRow(makeLabel("Family:"), fontCombo);

    QSpinBox *sizeSpin = new QSpinBox();
    sizeSpin->setRange(4, 999);
    sizeSpin->setValue(48);
    sizeSpin->setSuffix(" pt");
    sizeSpin->setStyleSheet(inputStyle());
    fl->addRow(makeLabel("Size:"), sizeSpin);

    m_contentLayout->addWidget(fontGroup);

    QGroupBox *styleGroup = new QGroupBox("Style");
    styleGroup->setStyleSheet(sectionStyle());
    QHBoxLayout *styleRow = new QHBoxLayout(styleGroup);
    styleRow->setSpacing(6);

    auto makeToggle = [](const QString &text, const QString &tip) {
        QToolButton *btn = new QToolButton();
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCheckable(true);
        btn->setFixedSize(30, 28);
        btn->setStyleSheet(
            "QToolButton { background:#2a2a2a; color:#aaa; border:1px solid #444; border-radius:4px; font-weight:bold; }"
            "QToolButton:hover { background:#3a3a3a; }"
            "QToolButton:checked { background:#2a82da; color:white; border-color:#2a82da; }"
        );
        return btn;
    };

    styleRow->addWidget(makeToggle("B", "Bold"));
    styleRow->addWidget(makeToggle("I", "Italic"));
    styleRow->addWidget(makeToggle("U", "Underline"));
    styleRow->addStretch();
    m_contentLayout->addWidget(styleGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// ── Select ────────────────────────────────────────────────
void ToolSettingsPanel::buildSelectControls()
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
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

// ── Shape ────────────────────────────────────────────────
void ToolSettingsPanel::buildShapeControls()
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

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
    row->addWidget(slider, 1); row->addWidget(spin);
    sg->addLayout(row);
    m_contentLayout->addWidget(strokeGroup);

    QGroupBox *fillGroup = new QGroupBox("Fill");
    fillGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *fg = new QVBoxLayout(fillGroup);
    QCheckBox *fillCheck = new QCheckBox("Fill Shape");
    fillCheck->setChecked(true);
    fillCheck->setStyleSheet(
        "QCheckBox { color:#ccc; font-size:11px; spacing:6px; }"
        "QCheckBox::indicator { width:14px; height:14px; border:1px solid #555; border-radius:3px; background:#1e1e1e; }"
        "QCheckBox::indicator:checked { background:#2a82da; border-color:#2a82da; }"
    );
    fg->addWidget(fillCheck);

    QComboBox *cornerCombo = new QComboBox();
    cornerCombo->setStyleSheet(inputStyle());
    cornerCombo->addItem("Sharp corners");
    cornerCombo->addItem("Rounded corners");
    fg->addWidget(makeLabel("Corner style:"));
    fg->addWidget(cornerCombo);
    m_contentLayout->addWidget(fillGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// ── Line ─────────────────────────────────────────────────
void ToolSettingsPanel::buildLineControls()
{
    clearContent();
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color:#2d2d2d;");
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 12, 10, 12);
    m_contentLayout->setSpacing(12);

    QGroupBox *lineGroup = new QGroupBox("Line Style");
    lineGroup->setStyleSheet(sectionStyle());
    QVBoxLayout *lg = new QVBoxLayout(lineGroup);
    lg->setSpacing(8);

    QHBoxLayout *row = new QHBoxLayout();
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 50); slider->setValue(2); slider->setStyleSheet(sliderStyle());
    QSpinBox *spin = new QSpinBox();
    spin->setRange(1, 50); spin->setValue(2); spin->setSuffix(" px"); spin->setFixedWidth(64);
    spin->setStyleSheet(inputStyle());
    connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
    row->addWidget(makeLabel("Width:")); row->addWidget(slider, 1); row->addWidget(spin);
    lg->addLayout(row);

    QComboBox *dashCombo = new QComboBox();
    dashCombo->setStyleSheet(inputStyle());
    dashCombo->addItem("Solid");
    dashCombo->addItem("Dashed");
    dashCombo->addItem("Dotted");
    lg->addWidget(makeLabel("Pattern:"));
    lg->addWidget(dashCombo);

    QCheckBox *arrowCheck = new QCheckBox("Arrow at end");
    arrowCheck->setStyleSheet(
        "QCheckBox { color:#ccc; font-size:11px; spacing:6px; }"
        "QCheckBox::indicator { width:14px; height:14px; border:1px solid #555; border-radius:3px; background:#1e1e1e; }"
        "QCheckBox::indicator:checked { background:#2a82da; border-color:#2a82da; }"
    );
    lg->addWidget(arrowCheck);
    m_contentLayout->addWidget(lineGroup);

    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}
