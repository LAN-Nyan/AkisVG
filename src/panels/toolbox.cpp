#include "toolbox.h"
#include "tools/tool.h"
#include "tools/selecttool.h"
#include "tools/penciltool.h"
#include "tools/brushtool.h"
#include "tools/erasertool.h"
#include "tools/shapetool.h"
#include "tools/texttool.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QColorDialog>
#include <QSlider>
#include <QSpinBox>
#include <QGridLayout>

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

    // Title
    QLabel *title = new QLabel("TOOLS");
    title->setStyleSheet(
        "font-weight: bold; "
        "font-size: 11px; "
        "color: #888; "
        "letter-spacing: 1px; "
        "padding: 4px 0;"
        );
    layout->addWidget(title);

    // Tool buttons container
    m_toolButtons->setExclusive(true);

    auto addToolButton = [&](ToolType type, const QString &icon, const QString &text, const QString &shortcut) {
        QPushButton *btn = new QPushButton(QString("%1  %2").arg(icon, text));
        btn->setCheckable(true);
        btn->setMinimumHeight(44);
        btn->setToolTip(QString("%1 (%2)").arg(text, shortcut));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            "   text-align: left;"
            "   padding: 10px 14px;"
            "   border: 2px solid #3a3a3a;"
            "   border-radius: 6px;"
            "   background-color: #2d2d2d;"
            "   color: #e0e0e0;"
            "   font-size: 13px;"
            "   font-weight: 500;"
            "}"
            "QPushButton:hover {"
            "   background-color: #3a3a3a;"
            "   border-color: #4a4a4a;"
            "}"
            "QPushButton:checked {"
            "   background-color: #2a82da;"
            "   border-color: #2a82da;"
            "   color: white;"
            "   font-weight: 600;"
            "}"
            );
        m_toolButtons->addButton(btn, static_cast<int>(type));
        layout->addWidget(btn);
    };

    addToolButton(ToolType::Select, "🔍", "Select", "V");
    layout->addSpacing(4);

    QLabel *drawLabel = new QLabel("DRAWING");
    drawLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 8px 0 4px 0;");
    layout->addWidget(drawLabel);

    addToolButton(ToolType::Pencil, "✏️", "Pencil", "P");
    addToolButton(ToolType::Brush, "🖌️", "Brush", "B");
    addToolButton(ToolType::Eraser, "🧹", "Eraser", "E");

    layout->addSpacing(4);

    QLabel *shapesLabel = new QLabel("SHAPES");
    shapesLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 8px 0 4px 0;");
    layout->addWidget(shapesLabel);

    addToolButton(ToolType::Rectangle, "▢", "Rectangle", "R");
    addToolButton(ToolType::Ellipse, "⬭", "Ellipse", "C");
    addToolButton(ToolType::Text, "T", "Text", "T");

    // Separator
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #3a3a3a; min-height: 1px; max-height: 1px; margin: 12px 0;");
    layout->addWidget(separator);

    // Color pickers
    QLabel *colorLabel = new QLabel("COLORS");
    colorLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 4px 0;");
    layout->addWidget(colorLabel);

    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->setSpacing(8);

    // Stroke color
    QVBoxLayout *strokeLayout = new QVBoxLayout();
    QLabel *strokeLabel = new QLabel("Stroke");
    strokeLabel->setStyleSheet("font-size: 10px; color: #aaa;");
    strokeLayout->addWidget(strokeLabel);

    QPushButton *strokeBtn = new QPushButton();
    strokeBtn->setFixedSize(50, 50);
    strokeBtn->setCursor(Qt::PointingHandCursor);
    strokeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #000000;"
        "   border: 2px solid #555;"
        "   border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "   border-color: #2a82da;"
        "}"
        );
    connect(strokeBtn, &QPushButton::clicked, this, [this, strokeBtn]() {
        QColor color = QColorDialog::getColor(Qt::black, this, "Choose Stroke Color");
        if (color.isValid()) {
            strokeBtn->setStyleSheet(QString(
                                         "QPushButton {"
                                         "   background-color: %1;"
                                         "   border: 2px solid #555;"
                                         "   border-radius: 6px;"
                                         "}"
                                         "QPushButton:hover {"
                                         "   border-color: #2a82da;"
                                         "}"
                                         ).arg(color.name()));

            if (m_currentTool) {
                m_currentTool->setStrokeColor(color);
            }
        }
    });
    strokeLayout->addWidget(strokeBtn);
    colorLayout->addLayout(strokeLayout);

    // Fill color
    QVBoxLayout *fillLayout = new QVBoxLayout();
    QLabel *fillLabel = new QLabel("Fill");
    fillLabel->setStyleSheet("font-size: 10px; color: #aaa;");
    fillLayout->addWidget(fillLabel);

    QPushButton *fillBtn = new QPushButton();
    fillBtn->setFixedSize(50, 50);
    fillBtn->setCursor(Qt::PointingHandCursor);
    fillBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: 2px solid #555;"
        "   border-radius: 6px;"
        "   background-image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTAwJSIgaGVpZ2h0PSIxMDAlIj48bGluZSB4MT0iMCIgeTE9IjEwMCUiIHgyPSIxMDAlIiB5Mj0iMCIgc3Ryb2tlPSJyZWQiIHN0cm9rZS13aWR0aD0iMiIvPjwvc3ZnPg==);"
        "}"
        "QPushButton:hover {"
        "   border-color: #2a82da;"
        "}"
        );
    connect(fillBtn, &QPushButton::clicked, this, [this, fillBtn]() {
        QColor color = QColorDialog::getColor(Qt::transparent, this, "Choose Fill Color",
                                              QColorDialog::ShowAlphaChannel);
        if (color.isValid()) {
            fillBtn->setStyleSheet(QString(
                                       "QPushButton {"
                                       "   background-color: %1;"
                                       "   border: 2px solid #555;"
                                       "   border-radius: 6px;"
                                       "}"
                                       "QPushButton:hover {"
                                       "   border-color: #2a82da;"
                                       "}"
                                       ).arg(color.name()));

            if (m_currentTool) {
                m_currentTool->setFillColor(color);
            }
        }
    });
    fillLayout->addWidget(fillBtn);
    colorLayout->addLayout(fillLayout);

    layout->addLayout(colorLayout);

    // Stroke width slider
    layout->addSpacing(8);
    QLabel *widthLabel = new QLabel("STROKE WIDTH");
    widthLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 4px 0;");
    layout->addWidget(widthLabel);

    QHBoxLayout *widthLayout = new QHBoxLayout();
    QSlider *widthSlider = new QSlider(Qt::Horizontal);
    widthSlider->setMinimum(1);
    widthSlider->setMaximum(50);
    widthSlider->setValue(2);
    widthSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   background: #3a3a3a;"
        "   height: 6px;"
        "   border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #2a82da;"
        "   width: 16px;"
        "   height: 16px;"
        "   margin: -5px 0;"
        "   border-radius: 8px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "   background: #3a92ea;"
        "}"
        );

    QSpinBox *widthSpin = new QSpinBox();
    widthSpin->setMinimum(1);
    widthSpin->setMaximum(50);
    widthSpin->setValue(2);
    widthSpin->setFixedWidth(60);
    widthSpin->setSuffix("px");
    widthSpin->setStyleSheet(
        "QSpinBox {"
        "   background-color: #2d2d2d;"
        "   border: 2px solid #3a3a3a;"
        "   border-radius: 4px;"
        "   padding: 4px 8px;"
        "   color: #e0e0e0;"
        "}"
        );

    connect(widthSlider, &QSlider::valueChanged, this, [this, widthSpin](int value) {
        widthSpin->setValue(value);
        if (m_currentTool) {
            m_currentTool->setStrokeWidth(value);
        }
    });
    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), widthSlider, &QSlider::setValue);

    widthLayout->addWidget(widthSlider);
    widthLayout->addWidget(widthSpin);
    layout->addLayout(widthLayout);

    layout->addStretch();

    // Quick help
    QLabel *helpLabel = new QLabel(
        "<div style='background: #3a3a3a; padding: 8px; border-radius: 4px;'>"
        "<b style='color: #2a82da;'>Quick Tips:</b><br>"
        "<span style='color: #aaa; font-size: 10px;'>"
        "• Ctrl+Wheel: Zoom<br>"
        "• Space+Drag: Pan<br>"
        "• Delete: Clear<br>"
        "</span>"
        "</div>"
        );
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);

    contentWidget->setStyleSheet("background-color: #2d2d2d;");
    mainLayout->addWidget(contentWidget);

    // Connect signals
    connect(m_toolButtons, &QButtonGroup::idClicked,
            this, &ToolBox::onToolButtonClicked);
}

void ToolBox::onToolButtonClicked(int id)
{
    ToolType type = static_cast<ToolType>(id);
    if (m_tools.contains(type)) {
        m_currentTool = m_tools[type];
        emit toolChanged(m_currentTool);
    }
}
