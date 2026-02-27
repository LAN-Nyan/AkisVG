#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUndoStack>

class Project;
class VectorCanvas;
class CanvasView;
class ToolBox;
class LayerPanel;
class ColorPicker;
class AssetLibrary;
class TimelineWidget;
class QMenu;
class QToolBar;
class VectorObject;
class QPushButton;
class QWidget;
class ProjectSettings;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportFrame();
    void about();
    void updateWindowTitle();

    // Import functions
    void importAudio();
    void importImage();

    // Export functions
    void exportToMp4();
    void exportGifKeyframes();
    void exportGifAllFrames();


private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createDockWindows();
    void setupCanvas();

    bool maybeSave();

    QUndoStack *m_undoStack;
    Project *m_project;
    VectorCanvas *m_canvas;
    CanvasView *m_canvasView;
    ToolBox *m_toolBox;
    LayerPanel *m_layerPanel;
    ColorPicker *m_colorPicker;
    AssetLibrary *m_assetLibrary;
    TimelineWidget *m_timeline;
    ProjectSettings *m_projectSettings;

    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;
    QToolBar *m_mainToolBar;

    QAction *m_undoAction;
    QAction *m_redoAction;

    QString m_currentFile;
    bool m_isModified;

};

#endif // MAINWINDOW_H
