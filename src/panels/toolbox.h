#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <QWidget>
#include <QButtonGroup>
#include <QMap>
#include <QPushButton>
#include <QComboBox>

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
    ToolSettingsPanel* settingsPanel() const { return m_settingsPanel; }

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
};

#endif // TOOLBOX_H
