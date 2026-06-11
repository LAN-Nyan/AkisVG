#ifndef DEBUGLOGPANEL_H
#define DEBUGLOGPANEL_H

#include <QWidget>

class QTextEdit;
class QPushButton;

class DebugLogPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DebugLogPanel(QWidget *parent = nullptr);

private:
    QTextEdit *m_logView = nullptr;
};

#endif
