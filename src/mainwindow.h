#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUndoStack>
#include <QList>

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
class SplineOverlay;
class ObjectGroup;
class EyedropperTool;
// ── NEW TOOL INCLUDES ────────────────────────────────────────────────────────
class LassoTool;
class MagicWandTool;
class SelectTool;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Called by main() after startup dialog
    void applyStartupSettings(const QString &name, int width, int height, int fps);
    void openProjectFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportFrame();
    void about();
    void updateWindowTitle();

    void onSplineCommitted(const QList<QPointF> &nodes);
    void onInstanceGroupRequested(ObjectGroup *group);
    // Import functions
    void importAudio();
    void importImage();

    // Export functions
    void exportToMp4();
    void exportGifKeyframes();
    void exportGifAllFrames();

    // ── NEW: Lasso / Magic Wand action slots ─────────────────────────────────
    void onLassoFill(const QPolygonF &poly, const QColor &color);
    void onLassoCut (const QPolygonF &poly);
    void onLassoCopy(const QPolygonF &poly);
    void onLassoPull(const QPolygonF &poly, QPointF dragStart);
    void provideWandSnapshot(MagicWandTool *wand);
    void addToRecentFiles(const QString &path);
    void updateRecentFilesMenu();

private:
   // void createActions();
    void createMenus();
    void createToolBars();
    void createDockWindows();
    void setupCanvas();
    void startInterpolationMode();

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
    SplineOverlay *m_splineOverlay = nullptr;
    EyedropperTool *m_eyedropperTool;

    // ── NEW TOOLS ────────────────────────────────────────────────────────────
    LassoTool     *m_lassoTool     = nullptr;
    MagicWandTool *m_magicWandTool = nullptr;

    // Clipboard for cut/copy
    QList<VectorObject*> m_clipboard;
    QList<VectorObject*> m_interpolationTargets;
    int  m_interpTotalFrames = 24;
    bool m_interpAdvanced    = false;
    QList<int> m_interpKeyframeTimes;

    QMenu *m_fileMenu;
    QMenu *m_recentMenu = nullptr;
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
