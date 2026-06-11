#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QMap>
#include <QKeySequence>
#include "tools/tool.h"

class HotkeyManager : public QObject
{
    Q_OBJECT

public:
    static HotkeyManager &instance();

    QKeySequence shortcutFor(ToolType type) const;
    void setShortcut(ToolType type, const QKeySequence &seq);
    void resetToDefaults();

    QString shortcutLabel(ToolType type) const;
    QString toolDisplayName(ToolType type) const;
    QList<ToolType> configurableTools() const;

    void load();
    void save();

signals:
    void shortcutsChanged();

private:
    explicit HotkeyManager(QObject *parent = nullptr);
    void ensureDefaults();

    QMap<ToolType, QKeySequence> m_shortcuts;
};

#endif
