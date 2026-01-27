#include "toolbox.h"
#include "tools/tool.h"
#include "tools/selecttool.h"
#include "tools/penciltool.h"
#include "tools/brushtool.h"
#include "tools/erasertool.h"
#include "tools/shapetool.h"
#include "tools/texttool.h"
#include "tools/filltool.h"
#include "toolsettingspanel.h"
#include "toolbutton.h"
#include "config.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QColorDialog>
#include <QSlider>
#include <QSpinBox>
#include <QGridLayout>
#include <QColorDialog>
#include <QCoreApplication>
#include <QFile>

ToolBox::ToolBox(QWidget *parent)
    : QWidget(parent)
    , m_currentTool(nullptr)
    , m_toolButtons(new QButtonGroup(this))
{
    createTools();
    setupUI();

    // Select pencil tool by default
    m_currentTool = m_tools[ToolType::Pencil];
    m_toolButtons->button(static_cast<int>(ToolType::Pencil))->setChecked(true);
    emit toolChanged(m_currentTool);
}

ToolBox::~ToolBox()
{
    qDeleteAll(m_tools);
}

void ToolBox::createTools()
{
    m_tools[ToolType::Select] = new SelectTool(this);
    m_tools[ToolType::Pencil] = new PencilTool(this);
    m_tools[ToolType::Brush] = new BrushTool(this);
    m_tools[ToolType::Eraser] = new EraserTool(this);
    m_tools[ToolType::Rectangle] = new ShapeTool(ShapeType::Rectangle, this);
    m_tools[ToolType::Ellipse] = new ShapeTool(ShapeType::Ellipse, this);
    m_tools[ToolType::Text] = new TextTool(this);
    m_tools[ToolType::Fill] = new FillTool(this);
    m_textureSelector = new QComboBox(this);
    m_textureSelector->addItem("Smooth", (int)ToolTexture::Smooth);
    m_textureSelector->addItem("Grainy", (int)ToolTexture::Grainy);
    m_textureSelector->addItem("Chalk", (int)ToolTexture::Chalk);
    m_textureSelector->addItem("Canvas", (int)ToolTexture::Canvas);
}

void ToolBox::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Scrollable content
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // --- TITLE ---
    QLabel *title = new QLabel("TOOLS");
    title->setStyleSheet("font-weight: bold; font-size: 11px; color: #888; letter-spacing: 1px; padding: 4px 0;");
    layout->addWidget(title);

    m_toolButtons->setExclusive(true);
    m_colorButton = new QPushButton(this);
    m_colorButton->setFixedSize(30, 30);
    updateColorButton(Qt::black); // Default color

    connect(m_colorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(m_currentTool->strokeColor(), this, "Pick a Color");
        if (color.isValid()) {
            m_currentTool->setStrokeColor(color);
            m_currentTool->setFillColor(color);
            updateColorButton(color);
        }
    });


    layout->addWidget(m_colorButton);

    for (QAbstractButton* btn : m_toolButtons->buttons()) {
        int id = m_toolButtons->id(btn);
        ToolType type = static_cast<ToolType>(id);

        connect(btn, &QPushButton::clicked, this, [this, type]() {
            // Update current tool
            if (m_tools.contains(type)) {
                m_currentTool = m_tools[type];
                emit toolChanged(m_currentTool);
            }

            // Update settings panel if it exists
            if (m_settingsPanel) {
                m_settingsPanel->updateForTool(type);
                m_settingsPanel->show();
            }
        });
    }

    // Helper lambda for consistent button styling with SVG icons
    auto addToolButton = [&](ToolType type, const QString &iconName, const QString &text, const QString &shortcut) {
        // Use IconConfig to get the icon path - EASY TO CUSTOMIZE!
        QString iconPath = IconConfig::getToolIconPath(iconName);

        // If not found in config, try direct path
        if (iconPath.isEmpty() || !QFile::exists(iconPath)) {
            // Fallback: try relative to executable
            QString appDir = QCoreApplication::applicationDirPath();
            iconPath = appDir + "/../" + IconConfig::iconBasePath() + iconName + ".svg";

            // Fallback: try current directory
            if (!QFile::exists(iconPath)) {
                iconPath = "./" + IconConfig::iconBasePath() + iconName + ".svg";
            }
        }

        ToolButton *btn = new ToolButton(iconPath, text, shortcut, this);
        m_toolButtons->addButton(btn, static_cast<int>(type));
        layout->addWidget(btn);
    };

    // --- SELECTION ---
    addToolButton(ToolType::Select, "select", "Select", "V");
    layout->addSpacing(4);

    // --- DRAWING ---
    QLabel *drawLabel = new QLabel("DRAWING");
    drawLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 8px 0 4px 0;");
    layout->addWidget(drawLabel);

    addToolButton(ToolType::Pencil, "pencil", "Pencil", "P");
    addToolButton(ToolType::Brush, "brush", "Brush", "B");
    addToolButton(ToolType::Eraser, "eraser", "Eraser", "E");
    addToolButton(ToolType::Fill, "fill", "Fill", "G");

    layout->addSpacing(4);

    // --- SHAPES ---
    QLabel *shapesLabel = new QLabel("SHAPES");
    shapesLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 8px 0 4px 0;");
    layout->addWidget(shapesLabel);

    addToolButton(ToolType::Rectangle, "rectangle", "Rectangle", "R");
    addToolButton(ToolType::Ellipse, "ellipse", "Ellipse", "C");
    addToolButton(ToolType::Text, "text", "Text", "T");

    // --- COLOR PICKER (OVERLAP STYLE) ---
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #3a3a3a; min-height: 1px; max-height: 1px; margin: 12px 0;");
    layout->addWidget(separator);

    QLabel *colorLabel = new QLabel("COLORS");
    colorLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 4px 0;");
    layout->addWidget(colorLabel);

    QWidget *overlapContainer = new QWidget();
    overlapContainer->setFixedSize(110, 80);

    // The Background Square (Fill)
    QPushButton *fillBtn = new QPushButton(overlapContainer);
    fillBtn->setGeometry(35, 25, 50, 50);
    fillBtn->setCursor(Qt::PointingHandCursor);
    fillBtn->setToolTip("Fill Color");
    fillBtn->setStyleSheet(
        "QPushButton { background-color: transparent; border: 3px solid #1a1a1a; border-radius: 6px; "
        "background-image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTAwJSIgaGVpZ2h0PSIxMDAlIj48bGluZSB4MT0iMCIgeTE9IjEwMCUiIHgyPSIxMDAlIiB5Mj0iMCIgc3Ryb2tlPSJyZWQiIHN0cm9rZS13aWR0aD0iMiIvPjwvc3ZnPg==); }"
        "QPushButton:hover { border-color: #2a82da; }"
        );

    // The Foreground Square (Stroke)
    QPushButton *strokeBtn = new QPushButton(overlapContainer);
    strokeBtn->setGeometry(10, 5, 50, 50);
    strokeBtn->setCursor(Qt::PointingHandCursor);
    strokeBtn->setToolTip("Stroke Color");
    strokeBtn->setStyleSheet("QPushButton { background-color: #000000; border: 3px solid #555; border-radius: 6px; } QPushButton:hover { border-color: #2a82da; }");

    connect(strokeBtn, &QPushButton::clicked, this, [this, strokeBtn]() {
        QColor color = QColorDialog::getColor(Qt::black, this, "Choose Stroke Color");
        if (color.isValid()) {
            strokeBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 3px solid #555; border-radius: 6px; } QPushButton:hover { border-color: #2a82da; }").arg(color.name()));
            if (m_currentTool) m_currentTool->setStrokeColor(color);
        }
    });

    connect(fillBtn, &QPushButton::clicked, this, [this, fillBtn]() {
        QColor color = QColorDialog::getColor(Qt::transparent, this, "Choose Fill Color", QColorDialog::ShowAlphaChannel);
        if (color.isValid()) {
            fillBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 3px solid #1a1a1a; border-radius: 6px; } QPushButton:hover { border-color: #2a82da; }").arg(color.name()));
            if (m_currentTool) m_currentTool->setFillColor(color);
        }
    });

    connect(m_textureSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_currentTool) {
            ToolTexture tex = static_cast<ToolTexture>(m_textureSelector->itemData(index).toInt());
            m_currentTool->setTexture(tex);
        }
    });

    layout->addWidget(overlapContainer);

    // --- STROKE WIDTH ---
    QLabel *widthLabel = new QLabel("STROKE WIDTH");
    widthLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 4px 0;");
    layout->addWidget(widthLabel);

    QHBoxLayout *widthLayout = new QHBoxLayout();
    QSlider *widthSlider = new QSlider(Qt::Horizontal);
    widthSlider->setRange(1, 50);
    widthSlider->setValue(2);
    widthSlider->setStyleSheet("QSlider::groove:horizontal { background: #3a3a3a; height: 6px; border-radius: 3px; } "
                               "QSlider::handle:horizontal { background: #2a82da; width: 16px; height: 16px; margin: -5px 0; border-radius: 8px; }");

    QSpinBox *widthSpin = new QSpinBox();
    widthSpin->setRange(1, 50);
    widthSpin->setValue(2);
    widthSpin->setSuffix("px");
    widthSpin->setStyleSheet("QSpinBox { background-color: #2d2d2d; border: 2px solid #3a3a3a; border-radius: 4px; padding: 4px; color: #e0e0e0; }");

    connect(widthSlider, &QSlider::valueChanged, this, [this, widthSpin](int v) {
        widthSpin->setValue(v);
        if (m_currentTool) m_currentTool->setStrokeWidth(v);
    });
    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), widthSlider, &QSlider::setValue);

    widthLayout->addWidget(widthSlider);
    widthLayout->addWidget(widthSpin);
    layout->addLayout(widthLayout);

    layout->addStretch();

    // --- QUICK HELP ---
    QLabel *helpLabel = new QLabel(
        "<div style='background: #3a3a3a; padding: 8px; border-radius: 4px;'>"
        "<b style='color: #2a82da;'>Quick Tips:</b><br>"
        "<span style='color: #aaa; font-size: 10px;'>• Ctrl+Wheel: Zoom<br>• Space+Drag: Pan</span>"
        "</div>"
        );
    layout->addWidget(helpLabel);

    contentWidget->setStyleSheet("background-color: #2d2d2d;");
    mainLayout->addWidget(contentWidget);

    connect(m_toolButtons, &QButtonGroup::idClicked, this, &ToolBox::onToolButtonClicked);
}

void ToolBox::updateColorButton(const QColor &color) {
    m_colorButton->setStyleSheet(QString("background-color: %1; border: 2px solid #555; border-radius: 5px;")
                                     .arg(color.name()));
}

void ToolBox::onToolButtonClicked(int id)
{
    ToolType type = static_cast<ToolType>(id);
    if (m_tools.contains(type)) {
        m_currentTool = m_tools[type];
        emit toolChanged(m_currentTool);
    }
}
