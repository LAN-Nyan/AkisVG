#include "mainwindow.h"
#include "core/project.h"
#include "canvas/vectorcanvas.h"
#include "canvas/canvasview.h"
#include "canvas/objects/imageobject.h"
#include "panels/toolbox.h"
#include "panels/layerpanel.h"
#include "panels/colorpicker.h"
#include "panels/assetlibrary.h"
#include "panels/toolsettingspanel.h"
#include "timeline/timelinewidget.h"
#include "tools/tool.h"
#include "tools/selecttool.h"
#include "io/gifexporter.h"
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
#include <QMediaPlayer>
#include <QAudioOutput> // ADDED
#include <QTimer>
#include <QEventLoop>
#include <QUrl>
#include <QPushButton> // ADDED
#include <QMessageBox>

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
    setWindowTitle("AkisVG");
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



    // Force the menu bar to render inline (prevents KDE/GNOME global menu from stealing it)
    menuBar()->setNativeMenuBar(false);

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
    //NSTANT UNDO/REDO REFRESH
    connect(m_undoStack, &QUndoStack::indexChanged, this, [this]() {
        m_canvas->refreshFrame();
        if (m_layerPanel) {
            m_layerPanel->rebuildLayerList();
        }
        // Also mark project as modified when an action is performed
        if (!m_undoStack->isClean()) {
            m_isModified = true;
            updateWindowTitle();
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

    QAction *importAudioAct = m_fileMenu->addAction("Import &Audio...");
    importAudioAct->setShortcut(QKeySequence("Ctrl+Shift+A"));
    connect(importAudioAct, &QAction::triggered, this, &MainWindow::importAudio);

    QAction *importImageAct = m_fileMenu->addAction("Import &Image...");
    importImageAct->setShortcut(QKeySequence("Ctrl+Shift+I"));
    connect(importImageAct, &QAction::triggered, this, &MainWindow::importImage);

    m_fileMenu->addSeparator();

    QAction *exportVideoAct = m_fileMenu->addAction("Export to &Video (.mp4/.mkv)...");
    exportVideoAct->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(exportVideoAct, &QAction::triggered, this, &MainWindow::exportToMp4);

    QAction *exportGifKeyframesAct = m_fileMenu->addAction("Export to GIF (Keyframes)...");
    connect(exportGifKeyframesAct, &QAction::triggered, this, &MainWindow::exportGifKeyframes);

    QAction *exportGifAllAct = m_fileMenu->addAction("Export to GIF (All Frames)...");
    connect(exportGifAllAct, &QAction::triggered, this, &MainWindow::exportGifAllFrames);

    QAction *exportAct = m_fileMenu->addAction("&Export Frame...", this, &MainWindow::exportFrame);
    exportAct->setShortcut(QKeySequence("Ctrl+E"));

    m_fileMenu->addSeparator();

    QAction *quitAct = m_fileMenu->addAction("&Quit", this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    // Edit Menu
    m_editMenu = menuBar()->addMenu("&Edit");

    m_undoAction = m_undoStack->createUndoAction(this, "&Undo");
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setIcon(QIcon::fromTheme("edit-undo"));
    m_editMenu->addAction(m_undoAction);

    m_redoAction = m_undoStack->createRedoAction(this, "&Redo");
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setIcon(QIcon::fromTheme("edit-redo"));
    m_editMenu->addAction(m_redoAction);

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
    m_mainToolBar->addAction(m_undoAction);
    m_mainToolBar->addAction(m_redoAction);

    m_undoAction->setToolTip("Undo (Ctrl+Z)");
    m_redoAction->setToolTip("Redo (Ctrl+Y)");

    // Set better icons with fallback
    if (m_undoAction->icon().isNull()) {
      m_undoAction->setText("↶ Undo");  // Fallback text icon
    }
    if (m_redoAction->icon().isNull()) {
      m_redoAction->setText("↷ Redo");  // Fallback text icon
    }

    m_mainToolBar->addSeparator();

    // Add zoom controls
    QAction *zoomInAct = m_mainToolBar->addAction("Zoom In", m_canvasView, &CanvasView::zoomIn);
    zoomInAct->setToolTip("Zoom In (Ctrl++)");

    QAction *zoomOutAct = m_mainToolBar->addAction("Zoom Out", m_canvasView, &CanvasView::zoomOut);
    zoomOutAct->setToolTip("Zoom Out (Ctrl+-)");

    QAction *resetZoomAct = m_mainToolBar->addAction("Reset", m_canvasView, &CanvasView::resetZoom);
    resetZoomAct->setToolTip("Reset Zoom (Ctrl+0)");

    m_mainToolBar->addSeparator();
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
    QTabWidget *rightTabs = new QTabWidget();
    rightTabs->setObjectName("rightPanelTabs");

    rightTabs->setStyleSheet(
        "QTabWidget::pane { border: none; background-color: #2d2d2d; }"
        "QTabBar::tab { background-color: #3a3a3a; color: #aaa; padding: 8px 16px; border: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #2d2d2d; color: white; }"
        "QTabBar::tab:hover { background-color: #4a4a4a; }"
    );

    // 1. Layers Tab
    rightTabs->addTab(m_layerPanel, "Layers");

    // 2. Colors Tab
    rightTabs->addTab(m_colorPicker, "Colors");

    // 3. Tool Options Tab — grab the panel that ToolBox already owns
    ToolSettingsPanel *toolSettingsPanel = m_toolBox->settingsPanel();
    rightTabs->addTab(toolSettingsPanel, "Tool");

    // Switch to the Tool tab and update it whenever the active tool changes
    connect(m_toolBox, &ToolBox::toolChanged, this, [rightTabs, toolSettingsPanel](Tool *tool) {
        toolSettingsPanel->updateForTool(tool->type(), tool);
        // Optionally auto-switch to the Tool tab so the user sees the options
        int toolTabIndex = rightTabs->indexOf(toolSettingsPanel);
        if (toolTabIndex >= 0)
            rightTabs->setCurrentIndex(toolTabIndex);
    });

    // Seed the panel with the default tool that was selected at startup
    if (m_toolBox->currentTool()) {
        toolSettingsPanel->updateForTool(m_toolBox->currentTool()->type(),
                                         m_toolBox->currentTool());
    }

    // 4. Assets Tab
    rightTabs->addTab(m_assetLibrary, "Assets");

    m_projectSettings = new ProjectSettings(m_project, this);
    rightTabs->addTab(m_projectSettings, "Settings");

    // Connect the settings changed signal so the canvas updates in real-time
    connect(m_projectSettings, &ProjectSettings::settingsChanged, this, [this]() {
        m_canvasView->setFixedSize(m_project->width(), m_project->height());
        m_canvas->update();
        m_timeline->update();
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
  QString fileName = QFileDialog::getOpenFileName(
      this,
      "Open Project",
      "",
      "AkisVG Projects (*.avg *.akisvg);;All Files (*)",  // CHANGED: Support both .avg and .akisvg
      nullptr,
      QFileDialog::DontUseNativeDialog
  );

    if (!fileName.isEmpty()) {
        if (m_project->loadFromFile(fileName)) {
            m_currentFile = fileName;
            m_isModified = false;
            updateWindowTitle();
            m_canvas->refreshFrame();
            m_layerPanel->rebuildLayerList();
            statusBar()->showMessage("Project opened: " + fileName, 3000);
        } else {
            QMessageBox::critical(this, "Load Error",
                                "Failed to open project from:\n" + fileName +
                                "\n\nPlease ensure the file is a valid AkisVG project.");  // IMPROVED MESSAGE
        }
    }
}

void MainWindow::saveProjectAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Project As",
        "",
        "AkisVG Projects (*.avg);;Legacy Format (*.akisvg)");  // CHANGED: .avg is default

    if (!fileName.isEmpty()) {
        // Ensure correct extension
        if (!fileName.endsWith(".avg", Qt::CaseInsensitive) &&
            !fileName.endsWith(".akisvg", Qt::CaseInsensitive)) {
            fileName += ".avg";  // CHANGED: Default to .avg
        }
        m_currentFile = fileName;
        saveProject();
    }
}
// Removed old file save

void MainWindow::saveProject()
{
    if (m_currentFile.isEmpty()) {
        saveProjectAs();
        return;
    }

    if (m_project && m_project->saveToFile(m_currentFile)) {
        m_isModified = false;       // Reset the modified flag
        m_undoStack->setClean();    // Tell the undo stack we are at a "clean" save point
        updateWindowTitle();        // Remove the '*' from title
        statusBar()->showMessage(tr("Project saved to %1").arg(m_currentFile), 3000);
    } else {
        QMessageBox::critical(this, tr("Save Error"),
                             tr("Failed to save project to %1").arg(m_currentFile));
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
  QMessageBox::about(this, "About AkisVG",
      "<h2>AkisVG 0.3</h2>"
      "<p><i>A high-performance, lightweight vector animation suite.</i></p>"
      "<hr noshade size='1'>"
      "<p>AkisVG combines the speed of <b>C++17</b> with the flexibility of <b>Qt 6</b> "
      "to provide a fluid, hardware-accelerated creative environment.</p>"

      "<p><b>Core Capabilities:</b></p>"
      "<ul>"
      "<li><b>Expressive Drawing:</b> Versatile vector tools with rich brush textures.</li>"
      "<li><b>Smart Animation:</b> Layer-based workflow with automated interpolation.</li>"
      "<li><b>Precise Control:</b> Frame-by-frame timeline for traditional workflows.</li>"
      "<li><b>High Performance:</b> GPU-accelerated rendering for real-time previews.</li>"
      "</ul>"

      "<p><small>Optimized for efficiency and creative freedom.</small></p>");
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
    QString title = "AkisVG";
    if (!m_currentFile.isEmpty()) {
        title += " - " + QFileInfo(m_currentFile).fileName();
    }
    if (m_isModified) {
        title += " *";
    }
    setWindowTitle(title);
}

void MainWindow::exportToMp4()
{
    // Let user choose format
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export to Video",
        "",
        "MP4 Video (*.mp4);;MKV Video (*.mkv);;All Files (*)");  // ADDED .mkv option

    if (fileName.isEmpty()) {
        return;
    }

    // Detect format from extension
    QString format = "mp4";
    if (fileName.endsWith(".mkv", Qt::CaseInsensitive)) {
        format = "mkv";
    } else if (!fileName.endsWith(".mp4", Qt::CaseInsensitive)) {
        fileName += ".mp4";  // Default to mp4
    }

    // Get export range
    int startFrame = 1;
    int endFrame = m_project->totalFrames();
    int fps = m_project->fps();

    // Create progress dialog
    QProgressDialog progress("Exporting frames...", "Cancel", 0, endFrame - startFrame + 1, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    // Export frames to temporary directory
    QString tempDir = QDir::temp().filePath("akisvg_export_" +
                      QString::number(QDateTime::currentMSecsSinceEpoch()));
    QDir().mkpath(tempDir);

    // Render each frame
    for (int frame = startFrame; frame <= endFrame; ++frame) {
        if (progress.wasCanceled()) {
            QDir(tempDir).removeRecursively();
            return;
        }

        m_project->setCurrentFrame(frame);
        m_canvas->refreshFrame();

        // Render frame to image
        QImage image(m_project->width(), m_project->height(), QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        m_canvas->render(&painter);
        painter.end();

        // Save frame
        QString framePath = tempDir + QString("/frame_%1.png").arg(frame, 6, 10, QChar('0'));
        image.save(framePath, "PNG");

        progress.setValue(frame - startFrame + 1);
        QApplication::processEvents();
    }

    // Build ffmpeg command based on format
    QString ffmpegCmd;

    if (format == "mkv") {
        // MKV format with H.264 codec
        ffmpegCmd = QString("ffmpeg -y -framerate %1 -i \"%2/frame_%%06d.png\" "
                           "-c:v libx264 -preset medium -crf 18 "
                           "-pix_fmt yuv420p \"%3\"")
                    .arg(fps)
                    .arg(tempDir)
                    .arg(fileName);
    } else {
        // MP4 format (original)
        ffmpegCmd = QString("ffmpeg -y -framerate %1 -i \"%2/frame_%%06d.png\" "
                           "-c:v libx264 -preset slow -crf 18 "
                           "-pix_fmt yuv420p \"%3\"")
                    .arg(fps)
                    .arg(tempDir)
                    .arg(fileName);
    }

    // Execute ffmpeg
    progress.setLabelText(QString("Encoding %1 video...").arg(format.toUpper()));
    progress.setRange(0, 0);  // Indeterminate progress

    QProcess ffmpeg;
    ffmpeg.start(ffmpegCmd);

    if (!ffmpeg.waitForStarted()) {
        QMessageBox::critical(this, "Export Error",
                            "Failed to start ffmpeg.\n\n"
                            "Make sure ffmpeg is installed and in your system PATH.");
        QDir(tempDir).removeRecursively();
        return;
    }

    ffmpeg.waitForFinished(-1);  // Wait indefinitely

    // Check result
    if (ffmpeg.exitCode() == 0) {
        QMessageBox::information(this, "Export Complete",
                               QString("Video exported successfully to:\n%1\n\n"
                                      "Format: %2\n"
                                      "Frames: %3-%4\n"
                                      "FPS: %5")
                               .arg(fileName)
                               .arg(format.toUpper())
                               .arg(startFrame)
                               .arg(endFrame)
                               .arg(fps));
    } else {
        QString errorOutput = ffmpeg.readAllStandardError();
        QMessageBox::critical(this, "Export Error",
                            QString("ffmpeg failed with exit code %1\n\n%2")
                            .arg(ffmpeg.exitCode())
                            .arg(errorOutput));
    }

    // Cleanup
    QDir(tempDir).removeRecursively();

    // Restore current frame
    m_canvas->refreshFrame();

    statusBar()->showMessage(QString("%1 video exported: %2")
                            .arg(format.toUpper())
                            .arg(QFileInfo(fileName).fileName()),
                            5000);
}

void MainWindow::exportGifKeyframes() {
    QString filePath = QFileDialog::getSaveFileName(
        this, "Export GIF (Keyframes Only)", QString(), "GIF Files (*.gif)");

    if (filePath.isEmpty()) return;

    GifExporter exporter(m_project);
    exporter.setFrameDelay(1000 / m_project->fps());
    exporter.setLoop(true);

    QProgressDialog progress("Exporting GIF (Keyframes Only)...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    connect(&exporter, &GifExporter::exportStarted, [&progress](int totalFrames) {
        progress.setMaximum(totalFrames);
        progress.setValue(0);
    });

    connect(&exporter, &GifExporter::frameExported, [&progress](int current, int /*total*/) { // FIXED
        progress.setValue(current);
    });

    bool success = exporter.exportToGif(filePath, GifExporter::ExportMode::KeyframesOnly);

    progress.close();

    if (success) {
        QMessageBox::information(this, "Export Successful",
            QString("GIF exported successfully to:\n%1").arg(filePath));
        statusBar()->showMessage("GIF exported successfully", 5000);
    } else {
        QMessageBox::warning(this, "Export Failed",
            QString("Failed to export GIF:\n%1").arg(exporter.lastError()));
    }
}

void MainWindow::exportGifAllFrames() {
    QString filePath = QFileDialog::getSaveFileName(
        this, "Export GIF (All Frames)", QString(), "GIF Files (*.gif)");

    if (filePath.isEmpty()) return;

    GifExporter exporter(m_project);
    exporter.setFrameDelay(1000 / m_project->fps());
    exporter.setLoop(true);

    QProgressDialog progress("Exporting GIF (All Frames)...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    connect(&exporter, &GifExporter::exportStarted, [&progress](int totalFrames) {
        progress.setMaximum(totalFrames);
        progress.setValue(0);
    });

    connect(&exporter, &GifExporter::frameExported, [&progress](int current, int /*total*/) { // FIXED
        progress.setValue(current);
    });

    bool success = exporter.exportToGif(filePath, GifExporter::ExportMode::EveryFrame);

    progress.close();

    if (success) {
        QMessageBox::information(this, "Export Successful",
            QString("GIF exported successfully to:\n%1").arg(filePath));
        statusBar()->showMessage("GIF exported successfully", 5000);
    } else {
        QMessageBox::warning(this, "Export Failed",
            QString("Failed to export GIF:\n%1").arg(exporter.lastError()));
    }
}

void MainWindow::importAudio()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "Import Audio", "",
        "Audio Files (*.mp3 *.wav *.ogg *.m4a *.flac *.aac);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    // Check if file exists
    if (!QFile::exists(fileName)) {
        QMessageBox::warning(this, "Import Error",
            "Audio file not found: " + fileName);
        return;
    }

    Layer *currentLayer = m_project->currentLayer();
    if (!currentLayer) {
        QMessageBox::warning(this, "Import Error",
            "No layer selected. Please create or select a layer first.");
        return;
    }

    // Create a media player to get duration
    QMediaPlayer *player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this); // Required for Qt6
    player->setAudioOutput(audioOutput);
    player->setSource(QUrl::fromLocalFile(fileName));

    // Wait for media to load
    QEventLoop loop;
    bool loaded = false;

    connect(player, &QMediaPlayer::mediaStatusChanged, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::InvalidMedia) {
            loaded = true;
            loop.quit();
        }
    });

    // Timeout after 5 seconds
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    if (!loaded || player->mediaStatus() == QMediaPlayer::InvalidMedia) {
        QMessageBox::warning(this, "Import Error",
            "Failed to load audio file. The file may be corrupted or in an unsupported format.");
        delete player;
        return;
    }

    // Calculate duration in frames
    qint64 durationMs = player->duration();
    int fps = m_project->fps();
    int durationFrames = (durationMs * fps) / 1000;

    // Create audio data
    AudioData audioData(fileName, m_project->currentFrame(), durationFrames);
    audioData.volume = 1.0f;
    audioData.muted = false;

    // Set audio data to layer
    currentLayer->setAudioData(audioData);

    // Update UI
    m_canvas->refreshFrame();
    m_layerPanel->rebuildLayerList();

    statusBar()->showMessage(QString("Audio imported: %1 (%2 frames)")
        .arg(QFileInfo(fileName).fileName())
        .arg(durationFrames), 5000);

    QMessageBox::information(this, "Audio Imported",
        QString("Audio file imported successfully!\n\n"
                "File: %1\n"
                "Duration: %2 seconds (%3 frames at %4 FPS)\n"
                "Starting at frame: %5")
        .arg(QFileInfo(fileName).fileName())
        .arg(durationMs / 1000.0, 0, 'f', 2)
        .arg(durationFrames)
        .arg(fps)
        .arg(m_project->currentFrame()));

    delete player;
}

void MainWindow::importImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "Import Image", "",
        "Image Files (*.png *.jpg *.jpeg *.bmp *.svg *.webp *.gif);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    QImage image(fileName);
    if (image.isNull()) {
        QMessageBox::warning(this, "Import Error",
            "Failed to load image: " + fileName + "\n\n"
            "The file may be corrupted or in an unsupported format.");
        return;
    }

    Layer *currentLayer = m_project->currentLayer();
    if (!currentLayer) {
        QMessageBox::warning(this, "Import Error",
            "No layer selected. Please create or select a layer first.");
        return;
    }

    // Create ImageObject
    ImageObject *imgObj = new ImageObject();
    imgObj->setImage(QPixmap::fromImage(image));

    // Center the image on the canvas
    qreal x = (m_project->width() - image.width()) / 2.0;
    qreal y = (m_project->height() - image.height()) / 2.0;
    imgObj->setPos(x, y);

    // Add to current layer at current frame
    currentLayer->addObjectToFrame(m_project->currentFrame(), imgObj);

    // Update canvas
    m_canvas->refreshFrame();
    m_isModified = true;
    updateWindowTitle();

    statusBar()->showMessage(QString("Image imported: %1 (%2x%3)")
        .arg(QFileInfo(fileName).fileName())
        .arg(image.width())
        .arg(image.height()), 5000);
}
