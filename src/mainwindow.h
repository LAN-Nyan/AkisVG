#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QUndoStack>

// Forward declarations
class CanvasView;
class VectorCanvas;
class ToolBox;
class LayerPanel;
class ColorPicker;
class AssetLibrary;
class TimelineWidget;
class Project;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    private slots:
    void exportToMp4();               // Fixes error C2039
    void exportToMp4_Alternative();   // Fixes error C2039
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportFrame();
    void about();

private:
    Project* m_project;                // Needed for error C2065
    VectorCanvas* m_canvas;           // Needed for error C2065
    CanvasView* m_canvasView;         // Needed for error C2065
    ProjectSettings* m_projectSettings;
    void createActions();
    void createMenus();
    void createToolBars();
    void createDockWindows();
    void setupCanvas();
    void updateWindowTitle();

    // YOU I D I O T WHY DID I PUT THIS AT THE BOTTOM
    QUndoStack *m_undoStack;


    // Core components
    Project *m_project;
    VectorCanvas *m_canvas;
    CanvasView *m_canvasView;
    ToolBox *m_toolBox;
    LayerPanel *m_layerPanel;
    ColorPicker *m_colorPicker;
    AssetLibrary *m_assetLibrary;
    TimelineWidget *m_timeline;

    // UI elements
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;

    QToolBar *m_mainToolBar;
    QStatusBar *m_statusBar;

    QString m_currentFile;
    bool m_isModified;
};

#endif // MAINWINDOW_H
