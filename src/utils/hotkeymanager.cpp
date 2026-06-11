#include "hotkeymanager.h"
#include <QSettings>

HotkeyManager &HotkeyManager::instance()
{
    static HotkeyManager mgr;
    return mgr;
}

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
    ensureDefaults();
    load();
}

void HotkeyManager::ensureDefaults()
{
    m_shortcuts[ToolType::Select]     = QKeySequence(Qt::Key_V);
    m_shortcuts[ToolType::eyedropper] = QKeySequence(Qt::Key_I);
    m_shortcuts[ToolType::Lasso]      = QKeySequence(Qt::Key_L);
    m_shortcuts[ToolType::MagicWand]  = QKeySequence(Qt::Key_W);
    m_shortcuts[ToolType::Pencil]     = QKeySequence(Qt::Key_P);
    m_shortcuts[ToolType::Brush]      = QKeySequence(Qt::Key_B);
    m_shortcuts[ToolType::Eraser]     = QKeySequence(Qt::Key_E);
    m_shortcuts[ToolType::Fill]       = QKeySequence(Qt::Key_G);
    m_shortcuts[ToolType::Gradient]   = QKeySequence(Qt::Key_D);
    m_shortcuts[ToolType::Blend]      = QKeySequence(Qt::Key_H);
    m_shortcuts[ToolType::Liquify]    = QKeySequence(Qt::Key_K);
    m_shortcuts[ToolType::Rectangle]  = QKeySequence(Qt::Key_R);
    m_shortcuts[ToolType::Ellipse]    = QKeySequence(Qt::Key_C);
    m_shortcuts[ToolType::Line]       = QKeySequence(Qt::Key_U);
    m_shortcuts[ToolType::Text]       = QKeySequence(Qt::Key_T);
}

QKeySequence HotkeyManager::shortcutFor(ToolType type) const
{
    return m_shortcuts.value(type);
}

void HotkeyManager::setShortcut(ToolType type, const QKeySequence &seq)
{
    if (!configurableTools().contains(type))
        return;
    m_shortcuts[type] = seq;
    save();
    emit shortcutsChanged();
}

void HotkeyManager::resetToDefaults()
{
    m_shortcuts.clear();
    ensureDefaults();
    save();
    emit shortcutsChanged();
}

QString HotkeyManager::shortcutLabel(ToolType type) const
{
    const QKeySequence seq = shortcutFor(type);
    return seq.isEmpty() ? QString() : seq.toString(QKeySequence::NativeText);
}

QString HotkeyManager::toolDisplayName(ToolType type) const
{
    switch (type) {
    case ToolType::Select:     return QStringLiteral("Select");
    case ToolType::eyedropper: return QStringLiteral("Pick Color");
    case ToolType::Lasso:      return QStringLiteral("Lasso");
    case ToolType::MagicWand:  return QStringLiteral("Magic Wand");
    case ToolType::Pencil:     return QStringLiteral("Pencil");
    case ToolType::Brush:      return QStringLiteral("Brush");
    case ToolType::Eraser:     return QStringLiteral("Eraser");
    case ToolType::Fill:       return QStringLiteral("Fill");
    case ToolType::Gradient:   return QStringLiteral("Gradient");
    case ToolType::Blend:      return QStringLiteral("Blend");
    case ToolType::Liquify:    return QStringLiteral("Liquify");
    case ToolType::Rectangle:  return QStringLiteral("Rectangle");
    case ToolType::Ellipse:    return QStringLiteral("Ellipse");
    case ToolType::Line:       return QStringLiteral("Line");
    case ToolType::Text:       return QStringLiteral("Text");
    default:                   return QString();
    }
}

QList<ToolType> HotkeyManager::configurableTools() const
{
    return {
        ToolType::Select, ToolType::eyedropper, ToolType::Lasso, ToolType::MagicWand,
        ToolType::Pencil, ToolType::Brush, ToolType::Eraser, ToolType::Fill,
        ToolType::Gradient, ToolType::Blend, ToolType::Liquify,
        ToolType::Rectangle, ToolType::Ellipse, ToolType::Line, ToolType::Text
    };
}

void HotkeyManager::load()
{
    QSettings s("AkisVG", "AkisVG");
    ensureDefaults();
    for (ToolType type : configurableTools()) {
        const QString key = QStringLiteral("hotkeys/%1").arg(static_cast<int>(type));
        if (s.contains(key))
            m_shortcuts[type] = QKeySequence::fromString(s.value(key).toString());
    }
}

void HotkeyManager::save()
{
    QSettings s("AkisVG", "AkisVG");
    for (auto it = m_shortcuts.cbegin(); it != m_shortcuts.cend(); ++it) {
        const QString key = QStringLiteral("hotkeys/%1").arg(static_cast<int>(it.key()));
        s.setValue(key, it.value().toString());
    }
}
