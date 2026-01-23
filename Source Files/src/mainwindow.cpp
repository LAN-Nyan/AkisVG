#include "mainwindow.h"
#include "core/project.h"
#include "canvas/vectorcanvas.h"
#include "canvas/canvasview.h"
#include "panels/toolbox.h"
#include "panels/layerpanel.h"
#include "panels/colorpicker.h"
#include "panels/assetlibrary.h"
#include "timeline/timelinewidget.h"
#include "tools/tool.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_project(new Project(this))
    , m_canvas(new VectorCanvas(m_project, this))
    , m_canvasView(new CanvasView(m_canvas, this))
    , m_toolBox(new ToolBox(this))
    , m_layerPanel(new LayerPanel(m_project, this))
    , m_colorPicker(new ColorPicker(this))
    , m_assetLibrary(new AssetLibrary(this))
    , m_timeline(new TimelineWidget(m_project, this))
    , m_undoStack(new QUndoStack(this))
    , m_isModified(false)
{
    setWindowTitle("Lumina Studio");
    resize(1800, 1000);
    setMinimumSize(1200, 700);

    // Set dark stylesheet
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1a1a1a;
        }
        QMenuBar {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border-bottom: 1px solid #000;
            padding: 4px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 6px 12px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #3a3a3a;
        }
        QMenu {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #000;
        }
        QMenu::item {
            padding: 6px 24px 6px 12px;
        }
        QMenu::item:selected {
            background-color: #2a82da;
        }
        QToolBar {
            background-color: #2d2d2d;
            border: none;
            spacing: 4px;
            padding: 4px;
        }
        QStatusBar {
            background-color: #2d2d2d;
            color: #a0a0a0;
            border-top: 1px solid #000;
        }
        QDockWidget {
            color: #e0e0e0;
            titlebar-close-icon: url(close.png);
            titlebar-normal-icon: url(float.png);
        }
        QDockWidget::title {
            background-color: #3a3a3a;
            padding: 6px;
            border-bottom: 1px solid #000;
        }
    )");

    createActions();
    createMenus();
    createToolBars();
    setupCanvas();
    createDockWindows();

    // Connect signals
    connect(m_toolBox, &ToolBox::toolChanged, m_canvas, &VectorCanvas::setCurrentTool);
    connect(m_project, &Project::modified, this, [this]() {
        m_isModified = true;
        updateWindowTitle();
    });

    // Connect color picker to toolbox
    connect(m_colorPicker, &ColorPicker::colorChanged, this, [this](const QColor &color) {
        if (m_toolBox->currentTool()) {
            m_toolBox->currentTool()->setStrokeColor(color);
        }
    });

    statusBar()->showMessage("Ready - Use Pencil tool to draw", 5000);
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupCanvas()
{
    // Create central widget with proper layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Canvas area with background
    m_canvasView->setStyleSheet("QGraphicsView { background-color: #1a1a1a; border: none; }");
    layout->addWidget(m_canvasView);

    setCentralWidget(centralWidget);
}

void MainWindow::createActions()
{
    // Actions are created in createMenus
}

void MainWindow::createMenus()
{
    // File Menu
    m_fileMenu = menuBar()->addMenu("&File");

    QAction *newAct = m_fileMenu->addAction("&New Project", this, &MainWindow::newProject);
    newAct->setShortcut(QKeySequence::New);

    QAction *openAct = m_fileMenu->addAction("&Open...", this, &MainWindow::openProject);
    openAct->setShortcut(QKeySequence::Open);

    QAction *saveAct = m_fileMenu->addAction("&Save", this, &MainWindow::saveProject);
    saveAct->setShortcut(QKeySequence::Save);

    QAction *saveAsAct = m_fileMenu->addAction("Save &As...", this, &MainWindow::saveProjectAs);
    saveAsAct->setShortcut(QKeySequence::SaveAs);

    m_fileMenu->addSeparator();

    QAction *exportAct = m_fileMenu->addAction("&Export Frame...", this, &MainWindow::exportFrame);
    exportAct->setShortcut(QKeySequence("Ctrl+E"));

    m_fileMenu->addSeparator();

    QAction *quitAct = m_fileMenu->addAction("&Quit", this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    // Edit Menu
    m_editMenu = menuBar()->addMenu("&Edit");

    QAction *undoAct = m_undoStack->createUndoAction(this, "&Undo");
    undoAct->setShortcut(QKeySequence::Undo);
    m_editMenu->addAction(undoAct);

    QAction *redoAct = m_undoStack->createRedoAction(this, "&Redo");
    redoAct->setShortcut(QKeySequence::Redo);
    m_editMenu->addAction(redoAct);

    m_editMenu->addSeparator();

    QAction *clearAct = m_editMenu->addAction("Clear Frame", this, [this]() {
        m_canvas->clearCurrentFrame();
    });
    clearAct->setShortcut(QKeySequence("Ctrl+Shift+X"));

    // View Menu
    m_viewMenu = menuBar()->addMenu("&View");

    QAction *zoomInAct = m_viewMenu->addAction("Zoom In", m_canvasView, &CanvasView::zoomIn);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);

    QAction *zoomOutAct = m_viewMenu->addAction("Zoom Out", m_canvasView, &CanvasView::zoomOut);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);

    QAction *resetZoomAct = m_viewMenu->addAction("Reset Zoom", m_canvasView, &CanvasView::resetZoom);
    resetZoomAct->setShortcut(QKeySequence("Ctrl+0"));

    m_viewMenu->addSeparator();

    // Help Menu
    m_helpMenu = menuBar()->addMenu("&Help");
    m_helpMenu->addAction("&About", this, &MainWindow::about);
    m_helpMenu->addAction("&Keyboard Shortcuts", this, [this]() {
        QMessageBox::information(this, "Keyboard Shortcuts",
                                 "<h3>Drawing</h3>"
                                 "<b>V</b> - Select Tool<br>"
                                 "<b>P</b> - Pencil Tool<br><br>"
                                 "<h3>View</h3>"
                                 "<b>Ctrl+Wheel</b> - Zoom<br>"
                                 "<b>Space+Drag</b> - Pan<br>"
                                 "<b>Ctrl+0</b> - Reset Zoom<br><br>"
                                 "<h3>Edit</h3>"
                                 "<b>Ctrl+Z</b> - Undo<br>"
                                 "<b>Ctrl+Y</b> - Redo<br>"
                                 "<b>Delete</b> - Delete Selected<br>"
                                 "<b>Ctrl+Shift+X</b> - Clear Frame");
    });
}

void MainWindow::createToolBars()
{
    m_mainToolBar = addToolBar("Main");
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setIconSize(QSize(20, 20));

    // Add undo/redo
    m_mainToolBar->addAction(m_undoStack->createUndoAction(this, "Undo"));
    m_mainToolBar->addAction(m_undoStack->createRedoAction(this, "Redo"));

    m_mainToolBar->addSeparator();

    // Add zoom controls
    QAction *zoomInAct = m_mainToolBar->addAction("Zoom In", m_canvasView, &CanvasView::zoomIn);
    zoomInAct->setToolTip("Zoom In (Ctrl++)");

    QAction *zoomOutAct = m_mainToolBar->addAction("Zoom Out", m_canvasView, &CanvasView::zoomOut);
    zoomOutAct->setToolTip("Zoom Out (Ctrl+-)");

    QAction *resetZoomAct = m_mainToolBar->addAction("Reset", m_canvasView, &CanvasView::resetZoom);
    resetZoomAct->setToolTip("Reset Zoom (Ctrl+0)");
}

void MainWindow::createDockWindows()
{
    // Left side - Toolbox
    QDockWidget *toolDock = new QDockWidget("Tools", this);
    toolDock->setWidget(m_toolBox);
    toolDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    toolDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    toolDock->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, toolDock);
    m_viewMenu->addAction(toolDock->toggleViewAction());

    // Right side - Tab widget for Layers, Colors, Assets
    QTabWidget *rightTabs = new QTabWidget();
    rightTabs->setStyleSheet(
        "QTabWidget::pane {"
        "   border: none;"
        "   background-color: #2d2d2d;"
        "}"
        "QTabBar::tab {"
        "   background-color: #3a3a3a;"
        "   color: #aaa;"
        "   padding: 8px 16px;"
        "   border: none;"
        "   border-top-left-radius: 4px;"
        "   border-top-right-radius: 4px;"
        "   margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "   background-color: #2d2d2d;"
        "   color: white;"
        "}"
        "QTabBar::tab:hover {"
        "   background-color: #4a4a4a;"
        "}"
        );

    rightTabs->addTab(m_layerPanel, "Layers");
    rightTabs->addTab(m_colorPicker, "Colors");
    rightTabs->addTab(m_assetLibrary, "Assets");

    QDockWidget *rightDock = new QDockWidget("Panel", this);
    rightDock->setWidget(rightTabs);
    rightDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    rightDock->setMinimumWidth(300);
    rightDock->setMaximumWidth(450);
    rightDock->setTitleBarWidget(new QWidget()); // Hide title bar
    addDockWidget(Qt::RightDockWidgetArea, rightDock);

    // Bottom - Timeline
    QDockWidget *timelineDock = new QDockWidget("Timeline", this);
    timelineDock->setWidget(m_timeline);
    timelineDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    timelineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    timelineDock->setMinimumHeight(200);
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock);
    m_viewMenu->addAction(timelineDock->toggleViewAction());
}

void MainWindow::newProject()
{
    if (m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "Unsaved Changes", "Save current project?",
                                                                  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {
            saveProject();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    m_project->createNew(1920, 1080, 24);
    m_canvas->refreshFrame();
    m_isModified = false;
    m_currentFile.clear();
    updateWindowTitle();
    statusBar()->showMessage("New project created", 3000);
}

void MainWindow::openProject()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open Project", "", "Lumina Projects (*.lumina);;All Files (*)");

    if (!fileName.isEmpty()) {
        // TODO: Implement project loading
        m_currentFile = fileName;
        m_isModified = false;
        updateWindowTitle();
        statusBar()->showMessage("Project opened: " + fileName, 3000);
    }
}

void MainWindow::saveProject()
{
    if (m_currentFile.isEmpty()) {
        saveProjectAs();
    } else {
        // TODO: Implement project saving
        m_isModified = false;
        updateWindowTitle();
        statusBar()->showMessage("Project saved", 3000);
    }
}

void MainWindow::saveProjectAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Project As", "", "Lumina Projects (*.lumina)");

    if (!fileName.isEmpty()) {
        m_currentFile = fileName;
        saveProject();
    }
}

void MainWindow::exportFrame()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Export Frame", "", "SVG Files (*.svg);;PNG Files (*.png)");

    if (!fileName.isEmpty()) {
        // TODO: Implement frame export
        statusBar()->showMessage("Frame exported: " + fileName, 3000);
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, "About Lumina Studio",
                       "<h2>Lumina Studio 1.0</h2>"
                       "<p>Professional vector animation suite</p>"
                       "<p>Built with Qt 6 and C++17</p>"
                       "<p><b>Features:</b></p>"
                       "<ul>"
                       "<li>Vector drawing with multiple tools</li>"
                       "<li>Layer-based animation</li>"
                       "<li>Frame-by-frame timeline</li>"
                       "<li>Hardware-accelerated rendering</li>"
                       "</ul>");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "Unsaved Changes",
                                                                  "Do you want to save your changes?",
                                                                  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {
            saveProject();
            event->accept();
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
        } else {
            event->accept();
        }
    } else {
        event->accept();
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = "Lumina Studio";
    if (!m_currentFile.isEmpty()) {
        title += " - " + QFileInfo(m_currentFile).fileName();
    }
    if (m_isModified) {
        title += " *";
    }
    setWindowTitle(title);
}
