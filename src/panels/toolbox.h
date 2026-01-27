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
    
    Tool* currentTool() const { return m_currentTool; }

signals:
    void toolChanged(Tool *tool);

private slots:
    void onToolButtonClicked(int id);

private:
    ToolSettingsPanel *m_settingsPanel = nullptr;

    void createTools();
    void setupUI();

    QPushButton *m_colorButton; // Declare the button
    void updateColorButton(const QColor &color);
    QComboBox *m_textureSelector;

    QMap<ToolType, Tool*> m_tools;
    Tool *m_currentTool;
    QButtonGroup *m_toolButtons;
};

#endif // TOOLBOX_H
