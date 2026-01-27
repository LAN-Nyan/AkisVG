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
#include "panels/settingspanel.h"

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
#include <QProcess>
#include <QDir>
#include <QProcess>
#include <QProgressDialog>
#include <QBuffer>
#include <QPainter>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_undoStack(new QUndoStack(this))
    , m_project(new Project(this))
    , m_canvas(new VectorCanvas(m_project, m_undoStack, this))
    , m_canvasView(new CanvasView(m_canvas, this))
    , m_toolBox(new ToolBox(this))
    , m_layerPanel(new LayerPanel(m_project, m_undoStack, this))
    , m_colorPicker(new ColorPicker(this))
    , m_assetLibrary(new AssetLibrary(this))
    , m_timeline(new TimelineWidget(m_project, this))
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

   // connect(m_canvas, &VectorCanvas::audioDropped,
     //       m_timeline, &TimelineWidget::loadAudioTrack);

    // Connect color picker to toolbox
    connect(m_colorPicker, &ColorPicker::colorChanged, this, [this](const QColor &color) {
        if (m_toolBox->currentTool()) {
            m_toolBox->currentTool()->setStrokeColor(color);
        }
    });

    // Connect color picker texture to current tool
    connect(m_colorPicker, &ColorPicker::textureChanged,
            this, [this](int textureType) {
                if (m_toolBox->currentTool()) {
                    m_toolBox->currentTool()->setTexture(
                        static_cast<ToolTexture>(textureType));
                }
            });

    // Connect audio drop from canvas to timeline
   // connect(m_canvas, &VectorCanvas::audioDropped,
      //      m_timeline, &TimelineWidget::loadAudioTrack);

    // Show audio loaded message
    //connect(m_timeline, &TimelineWidget::audioLoaded,
            //this, [this](const QString &name) {
             //   statusBar()->showMessage("♪ Audio loaded: " + name, 3000);
           // });

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

    // Canvas area with grey background (infinite canvas feel)
    m_canvasView->setStyleSheet("QGraphicsView { background-color: #3c3c3c; border: none; }");
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

    QAction *exportVideoAct = m_fileMenu->addAction("Export to &Video (.mp4)...");
    exportVideoAct->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(exportVideoAct, &QAction::triggered, this, &MainWindow::exportToMp4);

    QAction *exportAct = m_fileMenu->addAction("&Export Frame...", this, &MainWindow::exportFrame);
    exportAct->setShortcut(QKeySequence("Ctrl+E"));

    m_fileMenu->addSeparator();

    QAction *quitAct = m_fileMenu->addAction("&Quit", this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    // Edit Menu
    m_editMenu = menuBar()->addMenu("&Edit");

    QAction *undoAct = m_undoStack->createUndoAction(this, "&Undo");
    undoAct->setShortcut(QKeySequence::Undo);
    undoAct->setIcon(QIcon::fromTheme("edit-undo"));
    m_editMenu->addAction(undoAct);

    QAction *redoAct = m_undoStack->createRedoAction(this, "&Redo");
    redoAct->setShortcut(QKeySequence::Redo);
    redoAct->setIcon(QIcon::fromTheme("edit-redo"));
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

    QAction *onionSkinAct = m_viewMenu->addAction("Show Onion Skin");
    onionSkinAct->setCheckable(true);
    onionSkinAct->setChecked(true);
    onionSkinAct->setShortcut(QKeySequence("Ctrl+O"));
    connect(onionSkinAct, &QAction::toggled, m_canvas, &VectorCanvas::setOnionSkinEnabled);

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

    m_mainToolBar->addSeparator();

    // NEW: Project Settings Shortcut
    QAction *settingsAct = m_mainToolBar->addAction("⚙️");
    settingsAct->setToolTip("Project Settings");

    // Logic to jump to the Settings Tab (Index 3)
    connect(settingsAct, &QAction::triggered, this, [this]() {
        // Find the QTabWidget we created in createDockWindows
        QTabWidget* tabs = findChild<QTabWidget*>("rightPanelTabs");
        if (tabs) tabs->setCurrentIndex(3);
    });
}

void MainWindow::createDockWindows()
{
    // --- LEFT SIDE: TOOLBOX ---
    QDockWidget *toolDock = new QDockWidget("Tools", this);
    toolDock->setWidget(m_toolBox);
    toolDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    toolDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    toolDock->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, toolDock);
    m_viewMenu->addAction(toolDock->toggleViewAction());

    // --- RIGHT SIDE: UTILITY TABS ---
    // We create a container tab widget to hold Layers, Colors, Assets, and Settings
    QTabWidget *rightTabs = new QTabWidget();
    rightTabs->setObjectName("rightPanelTabs"); // Named so the Toolbar can find it

    rightTabs->setStyleSheet(
        "QTabWidget::pane {"
        "    border: none;"
        "    background-color: #2d2d2d;"
        "}"
        "QTabBar::tab {"
        "    background-color: #3a3a3a;"
        "    color: #aaa;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "    margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #2d2d2d;"
        "    color: white;"
        "}"
        "QTabBar::tab:hover {"
        "    background-color: #4a4a4a;"
        "}"
        );

    // 1. Layers Tab
    rightTabs->addTab(m_layerPanel, "Layers");

    // 2. Colors Tab
    rightTabs->addTab(m_colorPicker, "Colors");

    // 3. Assets Tab
    rightTabs->addTab(m_assetLibrary, "Assets");

    // 4. Project Settings Tab (INTEGRATION)
    // We initialize it here and pass the project pointer so it can edit data
    m_projectSettings = new ProjectSettings(m_project, this);
    rightTabs->addTab(m_projectSettings, "Settings");

    // --- LOGIC: CONNECT SETTINGS TO CANVAS ---
    connect(m_projectSettings, &ProjectSettings::settingsChanged, this, [this]() {
        // Update the drawing area size based on the new project width/height
        m_canvasView->setFixedSize(m_project->width(), m_project->height());
        m_canvas->update(); // Tells the internal canvas to redraw for the new size

        // Refresh the canvas and timeline
        m_canvas->update();
        m_timeline->update();

        // Mark project as edited
        m_isModified = true;
        updateWindowTitle();
    });

    // Wrap the tabs in a Dock Widget
    QDockWidget *rightDock = new QDockWidget("Panel", this);
    rightDock->setWidget(rightTabs);
    rightDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    rightDock->setMinimumWidth(300);
    rightDock->setMaximumWidth(450);
    rightDock->setTitleBarWidget(new QWidget()); // Hides the bulky "Panel" title bar
    addDockWidget(Qt::RightDockWidgetArea, rightDock);

    // --- BOTTOM: TIMELINE ---
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

void MainWindow::exportToMp4() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export MP4", "", "Video Files (*.mp4)");

    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".mp4", Qt::CaseInsensitive)) {
        fileName += ".mp4";
    }

    // Verify FFmpeg is installed
    QProcess testProcess;
    testProcess.start("ffmpeg", QStringList() << "-version");
    if (!testProcess.waitForFinished(3000) || testProcess.exitCode() != 0) {
        QMessageBox::critical(this, "FFmpeg Not Found",
                              "FFmpeg is not installed.\n\n"
                              "Install it with:\nsudo pacman -S ffmpeg");
        return;
    }

    // Get project settings
    int fps = m_project->fps();
    int width = m_project->width();
    int height = m_project->height();
    int totalFrames = m_project->totalFrames();

    if (totalFrames == 0) {
        QMessageBox::warning(this, "No Frames",
                             "The project has no frames to export.");
        return;
    }

    // Ensure dimensions are even (required for H.264)
    if (width % 2 != 0) width++;
    if (height % 2 != 0) height++;

    // Create progress dialog
    QProgressDialog progress("Rendering frames...", "Cancel", 0, totalFrames, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    // Start FFmpeg process
    QProcess ffmpeg;
    QStringList args;
    args << "-y"                            // Overwrite output
         << "-f" << "image2pipe"            // Read images from pipe
         << "-vcodec" << "png"              // Input format
         << "-r" << QString::number(fps)    // Frame rate
         << "-i" << "-"                     // Read from stdin
         << "-vcodec" << "libx264"          // H.264 codec
         << "-preset" << "medium"           // Encoding speed/quality
         << "-pix_fmt" << "yuv420p"         // Pixel format
         << "-crf" << "18"                  // Quality (0-51, lower=better)
         << fileName;

    ffmpeg.start("ffmpeg", args);

    if (!ffmpeg.waitForStarted(5000)) {
        QMessageBox::critical(this, "Error",
                              "Failed to start FFmpeg process.");
        return;
    }

    // Save current frame to restore later
    int originalFrame = m_project->currentFrame();
    bool exportSuccess = true;

    // Render each frame
    for (int i = 0; i < totalFrames; ++i) {
        if (progress.wasCanceled()) {
            ffmpeg.kill();
            ffmpeg.waitForFinished();
            exportSuccess = false;
            break;
        }

        progress.setValue(i);
        progress.setLabelText(QString("Rendering frame %1 of %2...")
                                  .arg(i + 1).arg(totalFrames));
        QApplication::processEvents();

        // Set project to this frame
        m_project->setCurrentFrame(i);
        m_canvas->refreshFrame();

        // Create image and render canvas to it
        QImage frameImage(width, height, QImage::Format_ARGB32);
        frameImage.fill(Qt::white);

        QPainter painter(&frameImage);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Render the canvas scene to the image
        m_canvas->render(&painter,
                                  QRectF(0, 0, width, height),
                                  QRectF(0, 0, m_project->width(), m_project->height()));

        painter.end();

        // Convert to PNG and write to FFmpeg
        QByteArray pngData;
        QBuffer buffer(&pngData);
        buffer.open(QIODevice::WriteOnly);

        if (!frameImage.save(&buffer, "PNG")) {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to encode frame %1").arg(i));
            exportSuccess = false;
            break;
        }

        qint64 written = ffmpeg.write(pngData);
        if (written != pngData.size()) {
            QMessageBox::critical(this, "Error",
                                  "Failed to write frame data to FFmpeg");
            exportSuccess = false;
            break;
        }

        ffmpeg.waitForBytesWritten(5000);
    }

    // Close FFmpeg input and wait for completion
    if (exportSuccess) {
        progress.setLabelText("Finalizing video...");
        progress.setRange(0, 0); // Indeterminate

        ffmpeg.closeWriteChannel();

        if (!ffmpeg.waitForFinished(30000)) {
            QMessageBox::critical(this, "Error",
                                  "FFmpeg timed out while finalizing video");
            exportSuccess = false;
        } else if (ffmpeg.exitCode() != 0) {
            QString errorOutput = QString::fromUtf8(ffmpeg.readAllStandardError());
            QMessageBox::critical(this, "FFmpeg Error",
                                  "FFmpeg failed:\n\n" + errorOutput);
            exportSuccess = false;
        }
    }

    progress.close();

    // Restore original frame
    m_project->setCurrentFrame(originalFrame);
    m_canvas->refreshFrame();

    if (exportSuccess) {
        QMessageBox::information(this, "Export Complete",
                                 QString("Video exported successfully!\n\n%1\n\n"
                                         "Duration: %2 frames at %3 FPS")
                                     .arg(fileName)
                                     .arg(totalFrames)
                                     .arg(fps));
    }
}

void MainWindow::exportToMp4_Alternative() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export MP4", "", "Video Files (*.mp4)");

    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(".mp4")) fileName += ".mp4";

    // Check FFmpeg
    QProcess test;
    test.start("ffmpeg", QStringList() << "-version");
    if (!test.waitForFinished(3000)) {
        QMessageBox::critical(this, "Error",
                              "FFmpeg not found. Install: sudo pacman -S ffmpeg");
        return;
    }

    int fps = m_project->fps();
    int totalFrames = m_project->totalFrames();

    if (totalFrames == 0) {
        QMessageBox::warning(this, "No Frames", "No frames to export.");
        return;
    }

    QProgressDialog progress("Exporting...", "Cancel", 0, totalFrames, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    // Start FFmpeg
    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", QStringList()
                               << "-y" << "-f" << "image2pipe" << "-vcodec" << "png"
                               << "-r" << QString::number(fps) << "-i" << "-"
                               << "-vcodec" << "libx264" << "-preset" << "medium"
                               << "-pix_fmt" << "yuv420p" << "-crf" << "18"
                               << fileName);

    if (!ffmpeg.waitForStarted()) {
        QMessageBox::critical(this, "Error", "Failed to start FFmpeg");
        return;
    }

    int originalFrame = m_project->currentFrame();

    for (int i = 0; i < totalFrames; ++i) {
        if (progress.wasCanceled()) {
            ffmpeg.kill();
            break;
        }

        progress.setValue(i);
        QApplication::processEvents();

        m_project->setCurrentFrame(i);
        m_canvas->update();
        QApplication::processEvents();

        // Grab canvas as image
        QImage frame = m_canvasView->grab().toImage();

        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        frame.save(&buf, "PNG");

        ffmpeg.write(ba);
    }

    ffmpeg.closeWriteChannel();
    ffmpeg.waitForFinished(30000);

    m_project->setCurrentFrame(originalFrame);
    m_canvas->update();

    if (ffmpeg.exitCode() == 0) {
        QMessageBox::information(this, "Success",
                                 "Video exported to:\n" + fileName);
    } else {
        QMessageBox::critical(this, "Error",
                              "Export failed:\n" +
                                  QString::fromUtf8(ffmpeg.readAllStandardError()));
    }
}
