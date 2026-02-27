#include "toolbox.h"
#include "tools/tool.h"
#include "tools/selecttool.h"
#include "tools/penciltool.h"
#include "tools/brushtool.h"
#include "tools/erasertool.h"
#include "tools/shapetool.h"
#include "tools/texttool.h"
#include "tools/filltool.h"
#include "tools/blendtool.h"
#include "tools/linetool.h"
#include "tools/liquifytool.h"
#include "tools/eyedroppertool.h"
#include "toolsettingspanel.h"
#include "toolbutton.h"
#include "config.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QCoreApplication>
#include <QFile>
#include <QScrollArea>

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
// Create objects
void ToolBox::createTools()
{
    m_tools[ToolType::Select] = new SelectTool(this);
    m_tools[ToolType::eyedropper] = new EyedropperTool(this);
    m_tools[ToolType::Pencil] = new PencilTool(this);
    m_tools[ToolType::Brush] = new BrushTool(this);
    m_tools[ToolType::Eraser] = new EraserTool(this);
    m_tools[ToolType::Rectangle] = new ShapeTool(ShapeType::Rectangle, this);
    m_tools[ToolType::Ellipse] = new ShapeTool(ShapeType::Ellipse, this);
    m_tools[ToolType::Line] = new LineTool(this);
    m_tools[ToolType::Text] = new TextTool(this);
    m_tools[ToolType::Fill] = new FillTool(this);
    m_tools[ToolType::Blend] = new BlendTool(this);
    m_tools[ToolType::Liquify] = new LiquifyTool(this);
}

void ToolBox::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Scrollable content wrapper
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background-color: #2d2d2d; border: none; }"
        "QScrollBar:vertical {"
        "   background: #2d2d2d;"
        "   width: 12px;"
        "   border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #555;"
        "   border-radius: 6px;"
        "   min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background: #666;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
    );

    QWidget *contentWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // --- TITLE ---
    QLabel *title = new QLabel("TOOLS");
    title->setStyleSheet("font-weight: bold; font-size: 11px; color: #888; letter-spacing: 1px; padding: 4px 0;");
    layout->addWidget(title);

    m_toolButtons->setExclusive(true);

    for (QAbstractButton* btn : m_toolButtons->buttons()) {
        int id = m_toolButtons->id(btn);
        ToolType type = static_cast<ToolType>(id);

        connect(btn, &QPushButton::clicked, this, [this, type]() {
            // Update current tool
            if (m_tools.contains(type)) {
                m_currentTool = m_tools[type];
                emit toolChanged(m_currentTool);
            }

            // Update settings panel
            if (m_settingsPanel) {
                m_settingsPanel->updateForTool(type, m_currentTool);
            }
        });
    }

    // Helper lambda for consistent button styling with SVG icons
    auto addToolButton = [&](ToolType type, const QString &iconName, const QString &text, const QString &shortcut) {
        // 1. Try the Resource System first (The ":" prefix is key!)
        QString iconPath = ":/" + iconName + ".svg";

        // 2. If it's not in resources, fallback to the physical disk (your current logic)
        if (!QFile::exists(iconPath)) {
            iconPath = IconConfig::getToolIconPath(iconName);
            // ... rest of your existing disk-check logic ...
        }

        ToolButton *btn = new ToolButton(iconPath, text, shortcut, this);
        m_toolButtons->addButton(btn, static_cast<int>(type));
        layout->addWidget(btn);
    };

    // --- SELECTION ---
    addToolButton(ToolType::Select, "select", "Select", "V");
    addToolButton(ToolType::eyedropper, "eyedropper", "Pick color", "p");
    layout->addSpacing(4);

    // --- DRAWING ---
    QLabel *drawLabel = new QLabel("DRAWING");
    drawLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 8px 0 4px 0;");
    layout->addWidget(drawLabel);

    addToolButton(ToolType::Pencil, "pencil", "Pencil", "P");
    addToolButton(ToolType::Brush, "brush", "Brush", "B");
    addToolButton(ToolType::Eraser, "eraser", "Eraser", "E");
    addToolButton(ToolType::Fill, "fill", "Fill", "G");
    addToolButton(ToolType::Blend, "blend", "Blend", "H");
    addToolButton(ToolType::Liquify, "liquify", "Liquify", "L");

    layout->addSpacing(4);

    // --- SHAPES ---
    QLabel *shapesLabel = new QLabel("SHAPES");
    shapesLabel->setStyleSheet("font-size: 10px; color: #666; font-weight: bold; padding: 8px 0 4px 0;");
    layout->addWidget(shapesLabel);

    addToolButton(ToolType::Rectangle, "rectangle", "Rectangle", "R");
    addToolButton(ToolType::Ellipse, "ellipse", "Ellipse", "C");
    addToolButton(ToolType::Line, "line", "Line", "L");
    addToolButton(ToolType::Text, "text", "Text", "T");

    layout->addStretch();

    contentWidget->setStyleSheet("background-color: #2d2d2d;");

    // Instantiate the tool settings panel so MainWindow can embed it
    m_settingsPanel = new ToolSettingsPanel(nullptr); // parent set by MainWindow

    // Set contentWidget as the scroll area's widget
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    connect(m_toolButtons, &QButtonGroup::idClicked, this, &ToolBox::onToolButtonClicked);
}

void ToolBox::onToolButtonClicked(int id)
{
    ToolType type = static_cast<ToolType>(id);
    if (m_tools.contains(type)) {
        m_currentTool = m_tools[type];
        emit toolChanged(m_currentTool);
    }
}

Tool* ToolBox::getTool(ToolType type) const {
    return m_tools.value(type, nullptr);
}
