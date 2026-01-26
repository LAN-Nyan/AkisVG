#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

#include <QPushButton>
#include <QSvgRenderer>
#include <QString>

class ToolButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ToolButton(const QString &iconPath, const QString &toolName, const QString &shortcut, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QSvgRenderer *m_svgRenderer;
    QString m_toolName;
    QString m_shortcut;
    bool m_hovered;
};

#endif // TOOLBUTTON_H
