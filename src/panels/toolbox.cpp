#include "toolbox.h"
#include "tools/tool.h"
#include "tools/selecttool.h"
#include "tools/penciltool.h"
#include "tools/brushtool.h"
#include "utils/thememanager.h"
#include "toolbutton.h"
#include "tools/erasertool.h"
#include "tools/shapetool.h"
#include "tools/texttool.h"
#include "tools/filltool.h"
#include "tools/gradienttool.h"
#include "tools/blendtool.h"
#include "tools/linetool.h"
#include "tools/liquifytool.h"
#include "tools/eyedroppertool.h"
#include "tools/lassotool.h"       // ← NEW
#include "tools/magicwandtool.h"   // ← NEW
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

void ToolBox::createTools()
{
    m_tools[ToolType::Select]      = new SelectTool(this);
    m_tools[ToolType::eyedropper]  = new EyedropperTool(this);
    m_tools[ToolType::Pencil]      = new PencilTool(this);
    m_tools[ToolType::Brush]       = new BrushTool(this);
    m_tools[ToolType::Eraser]      = new EraserTool(this);
    m_tools[ToolType::Rectangle]   = new ShapeTool(ShapeType::Rectangle, this);
    m_tools[ToolType::Ellipse]     = new ShapeTool(ShapeType::Ellipse, this);
    m_tools[ToolType::Line]        = new LineTool(this);
    m_tools[ToolType::Text]        = new TextTool(this);
    m_tools[ToolType::Fill]        = new FillTool(this);
    m_tools[ToolType::Gradient]    = new GradientTool(this);
    m_tools[ToolType::Blend]       = new BlendTool(this);
    m_tools[ToolType::Liquify]     = new LiquifyTool(this);
    m_tools[ToolType::Lasso]       = new LassoTool(this);      // ← NEW
    m_tools[ToolType::MagicWand]   = new MagicWandTool(this);  // ← NEW
}

void ToolBox::setupUI()
{
    const auto &t = theme();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(
        QString("QScrollArea { background-color: %1; border: none; }"
                "QScrollBar:vertical { background: %1; width: 8px; }"
                "QScrollBar::handle:vertical { background: %2; border-radius: 4px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
        .arg(t.bg0, t.bg1));

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QString("background-color: %1;").arg(t.bg0));

    QVBoxLayout *buttonLayout = new QVBoxLayout(m_contentWidget);
    buttonLayout->setContentsMargins(sc(8), sc(12), sc(8), sc(12));
    buttonLayout->setSpacing(sc(4));
    buttonLayout->setAlignment(Qt::AlignTop);

    m_toolButtons->setExclusive(true);

    auto addSectionLabel = [&](const QString &text, bool isAccent) {
        QLabel *label = new QLabel(text);
        QString color = isAccent ? t.accent : "#444444";
        // Font size and padding scale with UI scale
        int fs  = qRound(9  * uiScale());
        int pt  = qRound(12 * uiScale());
        int pb  = qRound(4  * uiScale());
        label->setStyleSheet(QString(
            "font-size: %2px; font-weight: bold; color: %1; "
            "letter-spacing: 1.5px; padding: %3px 0 %4px 2px;"
        ).arg(color).arg(fs).arg(pt).arg(pb));
        buttonLayout->addWidget(label);
        if (isAccent)
            m_accentLabels.append(label);
    };

    auto addToolButton = [&](ToolType type, const QString &icon, const QString &name, const QString &key) {
        QString path = ":/" + icon + ".svg";
        if (!QFile::exists(path)) path = IconConfig::getToolIconPath(icon);

        ToolButton *btn = new ToolButton(path, name, key, this);
        m_toolButtons->addButton(btn, static_cast<int>(type));
        buttonLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, type]() {
            if (m_tools.contains(type)) {
                m_currentTool = m_tools[type];
                emit toolChanged(m_currentTool);
            }
            if (m_settingsPanel)
                m_settingsPanel->updateForTool(type, m_currentTool);
        });
    };

    // ── TOOLS ────────────────────────────────────────────────────────────────
    addSectionLabel("TOOLS", true);
    addToolButton(ToolType::Select,      "select",      "Select",     "V");
    addToolButton(ToolType::eyedropper,  "eyedropper",  "Pick color", "I");

    // ── SELECTION ────────────────────────────────────────────────────────────
    addSectionLabel("SELECTION", false);
    addToolButton(ToolType::Lasso,       "lasso",       "Lasso",      "L");   // ← NEW
    addToolButton(ToolType::MagicWand,   "magicwand",   "Magic Wand", "W");   // ← NEW

    // ── DRAWING ──────────────────────────────────────────────────────────────
    addSectionLabel("DRAWING", false);
    addToolButton(ToolType::Pencil,      "pencil",      "Pencil",     "P");
    addToolButton(ToolType::Brush,       "brush",       "Brush",      "B");
    addToolButton(ToolType::Eraser,      "eraser",      "Eraser",     "E");
    addToolButton(ToolType::Fill,        "fill",        "Fill",       "G");
    addToolButton(ToolType::Gradient,    "gradient",    "Gradient",   "D");
    addToolButton(ToolType::Blend,       "blend",       "Blend",      "H");
    addToolButton(ToolType::Liquify,     "liquify",     "Liquify",    "K");

    // ── SHAPES ───────────────────────────────────────────────────────────────
    addSectionLabel("SHAPES", false);
    addToolButton(ToolType::Rectangle,   "rectangle",   "Rectangle",  "R");
    addToolButton(ToolType::Ellipse,     "ellipse",     "Ellipse",    "C");
    addToolButton(ToolType::Line,        "line",        "Line",       "U");
    addToolButton(ToolType::Text,        "text",        "Text",       "T");

    buttonLayout->addStretch();

    m_settingsPanel = new ToolSettingsPanel(nullptr);
    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

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

Tool* ToolBox::getTool(ToolType type) const
{
    return m_tools.value(type, nullptr);
}

void ToolBox::activateTool(ToolType type)
{
    if (!m_tools.contains(type)) return;
    m_currentTool = m_tools[type];
    // Update button check state
    if (auto *btn = m_toolButtons->button(static_cast<int>(type))) {
        btn->setChecked(true);
    }
    emit toolChanged(m_currentTool);
    if (m_settingsPanel)
        m_settingsPanel->updateForTool(type, m_currentTool);
}

void ToolBox::applyTheme()
{
    const ThemeColors &t = theme();

    m_scrollArea->setStyleSheet(
        QString("QScrollArea { background-color: %1; border: none; }"
                "QScrollBar:vertical { background: %1; width: 8px; }"
                "QScrollBar::handle:vertical { background: %2; border-radius: 4px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
        .arg(t.bg0, t.bg1));

    m_contentWidget->setStyleSheet(
        QString("background-color: %1;").arg(t.bg0));

    for (QLabel *label : m_accentLabels) {
        int fs = qRound(9  * uiScale());
        int pt = qRound(12 * uiScale());
        int pb = qRound(4  * uiScale());
        label->setStyleSheet(
            QString("font-size: %2px; font-weight: bold; color: %1; "
                    "letter-spacing: 1.5px; padding: %3px 0 %4px 2px;")
            .arg(t.accent).arg(fs).arg(pt).arg(pb));
    }

    for (QAbstractButton *btn : m_toolButtons->buttons()) {
        if (ToolButton *tb = qobject_cast<ToolButton*>(btn))
            tb->applyTheme();
    }

    if (m_settingsPanel)
        m_settingsPanel->applyTheme();
}
