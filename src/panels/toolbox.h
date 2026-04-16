#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <QWidget>
#include <QButtonGroup>
#include <QMap>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QScrollArea>
#include <QList>

#include "toolsettingspanel.h"

class Tool;
enum class ToolType;

class ToolBox : public QWidget
{
    Q_OBJECT

public:
    explicit ToolBox(QWidget *parent = nullptr);
    ~ToolBox();

    Tool* getTool(ToolType type) const;
    Tool* currentTool() const { return m_currentTool; }
    // Programmatically switch to a tool (e.g. from Pull mode)
    void activateTool(ToolType type);
    ToolSettingsPanel* settingsPanel() const { return m_settingsPanel; }
    void applyTheme();

signals:
    void toolChanged(Tool *tool);

private slots:
    void onToolButtonClicked(int id);

private:
    ToolSettingsPanel *m_settingsPanel = nullptr;

    void createTools();
    void setupUI();

    QMap<ToolType, Tool*> m_tools;
    Tool *m_currentTool;
    QButtonGroup *m_toolButtons;
    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget;
    QList<QLabel*> m_accentLabels; // section labels that use accent color
};

#endif // TOOLBOX_H
