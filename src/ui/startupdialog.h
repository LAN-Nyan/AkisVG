#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>

/**
 * StartupDialog — shown on launch before MainWindow becomes visible.
 *
 * Two panels:
 *   LEFT:  Recent projects list + Open button
 *   RIGHT: New project settings (name, size, FPS) + Create button
 *
 * Result is read via the public getters after exec() returns Accepted.
 */
class StartupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartupDialog(QWidget *parent = nullptr);

    enum class Action { NewProject, OpenProject, ImportVideo };
    Action action() const { return m_action; }

    // New-project settings
    QString projectName()  const;
    int     canvasWidth()  const;
    int     canvasHeight() const;
    int     fps()          const;

    // Open-project — path chosen by user
    QString openPath()     const { return m_openPath; }

private slots:
    void onCreateClicked();
    void onOpenClicked();
    void onRecentDoubleClicked(QListWidgetItem *item);
    void onPresetChanged(int index);

private:
    void setupUI();
    void loadRecents();
    void saveRecent(const QString &path);
    void paintEvent(QPaintEvent *event) override;

    QLineEdit   *m_nameEdit;
    QSpinBox    *m_widthSpin;
    QSpinBox    *m_heightSpin;
    QComboBox   *m_fpsComboo;
    QComboBox   *m_presetCombo;
    QListWidget *m_recentList;
    QPushButton *m_createBtn;
    QPushButton *m_openBtn;
    QComboBox   *m_themeCombo = nullptr;

    Action  m_action   = Action::NewProject;
    QString m_openPath;
};

#endif // STARTUPDIALOG_H
