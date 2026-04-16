#ifndef STARTUPSCREEN_H
#define STARTUPSCREEN_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStackedWidget>

// =========================================================
//  StartupScreen
//  Shows on application launch.  The user can:
//    • Create a new project (with resolution / FPS options)
//    • Open an existing project (.akvg file)
//    • Pick from a list of recent files
// =========================================================
class StartupScreen : public QDialog
{
    Q_OBJECT

public:
    enum class Result {
        NewProject,
        OpenProject,
        OpenRecent,
        Cancelled
    };

    explicit StartupScreen(QWidget *parent = nullptr);

    // Call these after exec() to retrieve what the user chose
    Result      chosenResult()      const { return m_result; }
    QString     chosenFilePath()    const { return m_chosenFilePath; }
    int         newProjectWidth()   const { return m_newWidth; }
    int         newProjectHeight()  const { return m_newHeight; }
    int         newProjectFps()     const { return m_newFps; }
    QString     newProjectName()    const { return m_newName; }

private slots:
    void onCreateClicked();
    void onOpenClicked();
    void onRecentItemDoubleClicked(QListWidgetItem *item);
    void onOpenRecentClicked();
    void onPresetSelected(int index);

private:
    void setupUI();
    void loadRecentFiles();
    void saveRecentFiles();

    // Widgets
    QListWidget   *m_recentList     = nullptr;
    QSpinBox      *m_widthSpin      = nullptr;
    QSpinBox      *m_heightSpin     = nullptr;
    QComboBox     *m_fpsCombо       = nullptr;
    QComboBox     *m_presetCombo    = nullptr;
    QLineEdit     *m_projectNameEdit = nullptr;
    QPushButton   *m_createBtn      = nullptr;
    QPushButton   *m_openBtn        = nullptr;
    QPushButton   *m_openRecentBtn  = nullptr;

    // Stored result values
    Result  m_result         = Result::Cancelled;
    QString m_chosenFilePath;
    int     m_newWidth  = 1920;
    int     m_newHeight = 1080;
    int     m_newFps    = 24;
    QString m_newName   = "Untitled";
};

#endif // STARTUPSCREEN_H
