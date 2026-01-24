#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <QWidget>
#include <QButtonGroup>
#include <QMap>

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
    void createTools();
    void setupUI();
    
    QMap<ToolType, Tool*> m_tools;
    Tool *m_currentTool;
    QButtonGroup *m_toolButtons;
};

#endif // TOOLBOX_H
