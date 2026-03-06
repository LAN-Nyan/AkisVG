//Headers
#include "mainwindow.h"
#include "core/project.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/pathobject.h"
#include "canvas/objects/shapeobject.h"
#include "canvas/canvasview.h"
#include "canvas/objects/objectgroup.h"
#include "canvas/splineoverlay.h"
#include "panels/toolbox.h"
#include "panels/layerpanel.h"
#include "panels/colorpicker.h"
#include "panels/assetlibrary.h"
#include "panels/toolsettingspanel.h"
#include "timeline/timelinewidget.h"
#include "tools/tool.h"
#include "tools/selecttool.h"
#include "tools/lassotool.h"
#include "tools/magicwandtool.h"
#include "canvas/objects/transformableimageobject.h"
#include "io/gifexporter.h"
#include "panels/settingspanel.h"
#include "tools/eyedroppertool.h"
#include "utils/thememanager.h"
// Includes
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QInputDialog>
#include <QDialog>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QCloseEvent>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QProcess>
#include <QProgressDialog>
#include <QBuffer>
#include <QPainter>
#include <QSvgGenerator>
#include <QApplication>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QEventLoop>
#include <QUrl>
#include <QPushButton>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPolygonF>
#include <QShortcut>
#include <QClipboard>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
// Constructor
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

    // Set Application Icon
    setWindowIcon(QIcon(":/icons/pencil.svg"));

    // Set dark stylesheet
    setStyleSheet(R"(
        /* Main Window & Central Widget */
        QMainWindow {
            background-color: #1e1e1e;
        }

        /* Menu Bar */
        QMenuBar {
            background-color: #1e1e1e;
            color: #e0e0e0;
            border-bottom: 1px solid #282828;
            padding: 2px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 6px 12px;
            border-radius: 2px;
        }
        QMenuBar::item:selected {
            background-color: #282828;
            color: #c0392b;
        }

        /* Dropdown Menus */
        QMenu {
            background-color: #242424;
            color: #e0e0e0;
            border: 1px solid #282828;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px 6px 12px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #c0392b;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background: #333333;
            margin: 4px 8px;
        }

        /* ToolBars */
        QToolBar {
            background-color: #1e1e1e;
            border-bottom: 1px solid #282828;
            border-top: none;
            spacing: 6px;
            padding: 4px;
        }
        QToolBar::separator {
            width: 1px;
            background: #333333;
            margin: 6px;
        }

        /* Dock Widgets */
        QDockWidget {
            color: #999;
            font-weight: bold;
            text-transform: uppercase;
            font-size: 10px;
        }
        QDockWidget::title {
            background-color: #1e1e1e;
            padding: 8px;
            border-bottom: 1px solid #282828;
            text-align: left;
            letter-spacing: 1.5px;
        }

        /* Status Bar */
        QStatusBar {
            background-color: #1e1e1e;
            color: #666;
            border-top: 1px solid #282828;
            font-size: 10px;
        }
        QStatusBar::item { border: none; }

        /* Tool Buttons (inside toolbars/panels) */
        QToolButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 4px;
        }
        QToolButton:hover {
            background-color: #282828;
            border: 1px solid #c0392b;
        }
        QToolButton:checked {
            background-color: #c0392b;
            color: white;
        }
    )");


    // Force the menu bar to render inline (prevents KDE/GNOME global menu from stealing it)
    menuBar()->setNativeMenuBar(false);

    createMenus();
    createToolBars();
    setupCanvas();
    createDockWindows();

    // Connect signals
    connect(m_toolBox, &ToolBox::toolChanged, m_canvas, &VectorCanvas::setCurrentTool);

    connect(m_toolBox, &ToolBox::toolChanged, this, [this](Tool *tool) {
        if (!tool) return;
        QColor toolColor = tool->strokeColor();
        if (m_colorPicker->currentColor() != toolColor) {
            // Temporarily disconnect the colorChanged to tool update path
            disconnect(m_colorPicker, &ColorPicker::colorChanged, this, nullptr);
            m_colorPicker->setColor(toolColor);
            // Reconnect
            connect(m_colorPicker, &ColorPicker::colorChanged, this, [this](const QColor &color) {
                if (m_toolBox->currentTool()) {
                    m_toolBox->currentTool()->setStrokeColor(color);
                    m_toolBox->currentTool()->setFillColor(color);
                }
            });
        }
    });
    connect(m_project, &Project::modified, this, [this]() {
        m_isModified = true;
        updateWindowTitle();
    });

    // Re-sync the selected image pointer after every refreshFrame.
    connect(m_project, &Project::currentFrameChanged,
            m_canvasView, &CanvasView::syncSelectedImage);
    connect(m_project, &Project::modified,
            m_canvasView, &CanvasView::syncSelectedImage);

    // Initial color picker to tool connection (also re-established in toolChanged handler above)
    connect(m_colorPicker, &ColorPicker::colorChanged, this, [this](const QColor &color) {
        if (m_toolBox->currentTool()) {
            m_toolBox->currentTool()->setStrokeColor(color);
            m_toolBox->currentTool()->setFillColor(color);
        }
    });
    // Undo and Redo Stack
    connect(m_undoStack, &QUndoStack::indexChanged, this, [this]() {
        m_canvas->refreshFrame();
        if (m_layerPanel) {
            m_layerPanel->rebuildLayerList();
        }
        // Mark project as modified when an action is performed
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
    // Connect Lasso, and Magic Wand
    m_lassoTool     = qobject_cast<LassoTool*>(m_toolBox->getTool(ToolType::Lasso));
    m_magicWandTool = qobject_cast<MagicWandTool*>(m_toolBox->getTool(ToolType::MagicWand));

    if (m_lassoTool) {
        connect(m_lassoTool, &LassoTool::actionFill, this, &MainWindow::onLassoFill);
        connect(m_lassoTool, &LassoTool::actionCut,  this, &MainWindow::onLassoCut);
        connect(m_lassoTool, &LassoTool::actionCopy, this, &MainWindow::onLassoCopy);
        // Start marching-ants animation when a selection is formed
        connect(m_lassoTool, &LassoTool::selectionChanged, this, [this]() {
            if (m_lassoTool->hasSelection())
                m_canvasView->startAntTimer();
        });
        connect(m_lassoTool, &LassoTool::actionPull, this, &MainWindow::onLassoPull);
    }
    if (m_magicWandTool) {
        connect(m_magicWandTool, &MagicWandTool::actionFill, this, &MainWindow::onLassoFill);
        connect(m_magicWandTool, &MagicWandTool::actionCut,  this, &MainWindow::onLassoCut);
        connect(m_magicWandTool, &MagicWandTool::actionCopy, this, &MainWindow::onLassoCopy);
    }

    // Feed Magic Wand a fresh canvas snapshot on tool switch and before each click
    connect(m_toolBox, &ToolBox::toolChanged, this, [this](Tool *tool) {
        if (tool && tool->type() == ToolType::MagicWand) {
            if (auto *wand = qobject_cast<MagicWandTool*>(tool))
                provideWandSnapshot(wand);
        }
    });
    connect(m_canvasView, &CanvasView::wandAboutToClick, this, [this]() {
        if (m_magicWandTool)
            provideWandSnapshot(m_magicWandTool);
    });

    // Escape cancels in-progress lasso/wand
    new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]() {
        if (m_lassoTool)     m_lassoTool->cancel();
        if (m_magicWandTool) m_magicWandTool->cancel();
        m_canvasView->viewport()->update();
    });
}
// Mainwindow
MainWindow::~MainWindow()
{
}

void MainWindow::setupCanvas()
{
    // I Layout Setup
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_canvasView->setStyleSheet("QGraphicsView { background-color: #3c3c3c; border: none; }");
    layout->addWidget(m_canvasView);
    setCentralWidget(centralWidget);

    // Initialize Spline Overlay ONCE and parent to Viewport
    m_splineOverlay = new SplineOverlay(m_canvasView->viewport());
    m_splineOverlay->hide();
    m_canvasView->setSplineOverlay(m_splineOverlay);

    // Re-link the Enter/Esc keys
    connect(m_splineOverlay, &SplineOverlay::committed, this, &MainWindow::onSplineCommitted);
    connect(m_splineOverlay, &SplineOverlay::exitRequested, this, [this]() {
        m_interpolationTargets.clear();
        m_canvas->exitInterpolationMode();
    });

    // Forward node changes to settings panel for advanced per-node controls
    connect(m_splineOverlay, &SplineOverlay::splineChanged, this, [this](const QList<QPointF> &nodes) {
        if (m_toolBox && m_toolBox->settingsPanel())
            m_toolBox->settingsPanel()->updateInterpolationNodes(nodes.size());
    });

    connect(m_canvasView, &CanvasView::viewportResized, this, [this]() {
        m_splineOverlay->resize(m_canvasView->viewport()->size());
    });

    connect(m_canvasView, &CanvasView::frameNavigationRequested, this, [this](int delta) {
        if (!m_project) return;
        int next = qBound(1, m_project->currentFrame() + delta, m_project->totalFrames());
        m_project->setCurrentFrame(next);
    });

    // Handle the Interpolation Mode State (We use m_spline overlay here found in src/canvas/splineoverlay.cpp)
    connect(m_canvas, &VectorCanvas::interpolationModeChanged, this, [this](bool active) {
        if (active) {
            m_splineOverlay->resize(m_canvasView->viewport()->size());
            m_splineOverlay->show();

            // THE FOCUS TRAP Don't fuck with this
            m_splineOverlay->setFocusPolicy(Qt::StrongFocus);
            m_splineOverlay->setFocus();
            m_splineOverlay->raise();

            setWindowTitle("AkisVG - Interpolating…");

            // Show interpolation controls in tool settings panel (THIS SHIT TOOK FOREVER TO GET RIGHT)
            if (m_toolBox && m_toolBox->settingsPanel())
                m_toolBox->settingsPanel()->showInterpolationControls(m_interpTotalFrames);
        } else {
            m_splineOverlay->hide();
            m_canvasView->setFocus();
            updateWindowTitle();

            // Restore normal select controls DO NOT TRY TO FIX
            if (m_toolBox && m_toolBox->settingsPanel())
                m_toolBox->settingsPanel()->hideInterpolationControls();
        }
    });

    // Listen for settings changes from the panel
    if (m_toolBox && m_toolBox->settingsPanel()) {
        connect(m_toolBox->settingsPanel(), &ToolSettingsPanel::interpolationSettingsChanged,
                this, [this](int totalFrames, bool advanced, QList<int> keyframeTimes) {
            m_interpTotalFrames  = totalFrames;
            m_interpAdvanced     = advanced;
            m_interpKeyframeTimes = keyframeTimes;
        });
    }

    // Tool also = Asset Connections
    m_eyedropperTool = qobject_cast<EyedropperTool*>(m_toolBox->getTool(ToolType::eyedropper));
    if (m_eyedropperTool) {
        connect(m_eyedropperTool, &EyedropperTool::colorPicked, this, [this](const QColor &stroke, const QColor &fill){
            Q_UNUSED(fill);
            if (m_colorPicker) m_colorPicker->setColor(stroke);
        });
    }

    connect(m_colorPicker, &ColorPicker::colorChanged, this, [this](const QColor &newColor){
        Tool *activeTool = m_toolBox->currentTool();
        if (activeTool) {
            activeTool->setStrokeColor(newColor);
            activeTool->setFillColor(newColor);
            m_canvas->update();
        }
    });

    connect(m_assetLibrary, &AssetLibrary::groupInstanceRequested, this, &MainWindow::onInstanceGroupRequested);

    // Context Menu
    connect(m_canvas, &VectorCanvas::contextMenuRequestedAt, this, [this](const QPoint &globalPos, const QPointF &scenePos) {

    // Always prefer SelectTool's list.

        QList<VectorObject*> selected;

        // Try SelectTool's tracked selection first
        if (SelectTool *sel = qobject_cast<SelectTool*>(m_toolBox->getTool(ToolType::Select))) {
            for (VectorObject *src : sel->selectedObjects()) {
                if (src) selected.append(src);
            }
        }

        //  If nothing selected via SelectTool, pick topmost object under cursor
        if (selected.isEmpty()) {
            for (QGraphicsItem *it : m_canvas->items(scenePos, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder)) {
                if (VectorObject *vo = dynamic_cast<VectorObject*>(it)) {
                    VectorObject *src = m_canvas->sourceObject(vo);
                    if (src) selected.append(src);
                    else     selected.append(vo);
                    break;
                }
            }
        }

        QMenu menu(this);
        menu.setStyleSheet("QMenu { background:#2d2d2d; color:white; border:1px solid #1a1a1a; }");

        // Restore Actions
        QAction *groupAct  = menu.addAction("Group Selected");
        QAction *interpAct = menu.addAction("Interpolate");
        menu.addSeparator();
        QAction *cutAct    = menu.addAction("Cut");
        QAction *copyAct   = menu.addAction("Copy");
        menu.addSeparator();
        QAction *deleteAct = menu.addAction("Delete");
        menu.addSeparator();
        QAction *saveAct   = menu.addAction("Save as Asset…");

        bool hasSel = !selected.isEmpty();
        groupAct->setEnabled(selected.size() >= 2);
        interpAct->setEnabled(hasSel);
        cutAct->setEnabled(hasSel);
        copyAct->setEnabled(hasSel);
        deleteAct->setEnabled(hasSel);
        saveAct->setEnabled(hasSel);

        QAction *chosen = menu.exec(globalPos);
        if (!chosen) return;

        if (chosen == groupAct) {
            bool ok;
            QString name = QInputDialog::getText(this, "Group", "Name:", QLineEdit::Normal, "Group", &ok);
            if (ok) {
                ObjectGroup *grp = m_canvas->groupObjects(selected, name);
                if (grp) m_assetLibrary->addObjectGroup(grp);
            }
        } else if (chosen == interpAct) {
            // resolve each display clone to its layer-owned source object.
            m_interpolationTargets.clear();
            for (VectorObject *obj : selected)
                m_interpolationTargets.append(m_canvas->sourceObject(obj));
            m_canvas->enterInterpolationMode();
        } else if (chosen == cutAct) {
            m_clipboard = selected;
            for (VectorObject *obj : selected) m_canvas->removeObject(obj);
        } else if (chosen == copyAct) {
            m_clipboard = selected;
        } else if (chosen == deleteAct) {
            for (VectorObject *obj : selected) m_canvas->removeObject(obj);
        } else if (chosen == saveAct) {
            bool ok;
            QString name = QInputDialog::getText(this, "Save Asset", "Name:", QLineEdit::Normal, "Asset", &ok);
            if (ok && !name.isEmpty()) {
                ObjectGroup *grp = m_canvas->groupObjects(selected, name);
                if (grp) m_assetLibrary->addObjectGroup(grp);
            }
        }
    });
}

void MainWindow::createMenus()
{
    // File Menu
    m_fileMenu = menuBar()->addMenu("&File");

    QAction *newAct = m_fileMenu->addAction("&New Project", this, &MainWindow::newProject);
    newAct->setShortcut(QKeySequence::New);
    newAct->setIcon(QIcon(":/icons/addframe.svg"));

    QAction *openAct = m_fileMenu->addAction("&Open...", this, &MainWindow::openProject);
    openAct->setShortcut(QKeySequence::Open);
    // Use SVG icon for Open action instead of default system folder icon
    openAct->setIcon(QIcon(":/icons/newlayer.svg"));

    m_recentMenu = m_fileMenu->addMenu("Open &Recent");
    updateRecentFilesMenu();

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

    m_editMenu->addSeparator();

    QAction *groupAct = m_editMenu->addAction("Group Selected", this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, "Group Name",
                                             "Name this group:", QLineEdit::Normal,
                                             "Group", &ok);
        if (!ok) name = "Group";

        QList<VectorObject*> sel;
        if (SelectTool *st = qobject_cast<SelectTool*>(m_toolBox->getTool(ToolType::Select)))
            sel = st->selectedObjects();
        ObjectGroup *grp = sel.isEmpty()
            ? m_canvas->groupSelectedObjects(name)   // fallback
            : m_canvas->groupObjects(sel, name);
        if (grp) m_assetLibrary->addObjectGroup(grp);
    });
    groupAct->setShortcut(QKeySequence("Ctrl+G"));

    QAction *ungroupAct = m_editMenu->addAction("Ungroup", this, [this]() {
        m_canvas->ungroupSelected();
    });
    ungroupAct->setShortcut(QKeySequence("Ctrl+Shift+G"));

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
    connect(onionSkinAct, &QAction::toggled, this, [this](bool enabled) {
        // Update both the project's setting and the canvas flag, then refresh
        m_project->setOnionSkinEnabled(enabled);
        m_canvas->setOnionSkinEnabled(enabled);
        m_canvas->refreshFrame();   // redraw immediately, no geometry change = no jump
    });

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
        m_undoAction->setText("↶");
    }
    if (m_redoAction->icon().isNull()) {
        m_redoAction->setText("↷");
    }

    m_mainToolBar->addSeparator();

    // Icon-only zoom controls
    auto getInvertedIcon = [](const QString& path) -> QIcon {
        QIcon original(path);
        // Render the SVG to a pixmap at a standard icon size (24x24
        QPixmap pix = original.pixmap(QSize(24, 24));
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
        img.invertPixels(QImage::InvertRgb);
        return QIcon(QPixmap::fromImage(img));
    };

    // 2. Apply to your actions using the aliases from your .qrc
    QAction *zoomInAct = m_mainToolBar->addAction(getInvertedIcon(":/icons/zoomin.svg"),
                                                  "Zoom In", m_canvasView, &CanvasView::zoomIn);
    zoomInAct->setToolTip("Zoom In (Ctrl++)");
    zoomInAct->setShortcut(QKeySequence::ZoomIn);

    QAction *zoomOutAct = m_mainToolBar->addAction(getInvertedIcon(":/icons/zoomout.svg"),
                                                   "Zoom Out", m_canvasView, &CanvasView::zoomOut);
    zoomOutAct->setToolTip("Zoom Out (Ctrl+-)");
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);

    QAction *resetZoomAct = m_mainToolBar->addAction(getInvertedIcon(":/icons/zoomreset.svg"),
                                                     "Reset Zoom", m_canvasView, &CanvasView::resetZoom);
    resetZoomAct->setToolTip("Reset Zoom (Ctrl+0)");
    resetZoomAct->setShortcut(tr("Ctrl+0"));

    m_mainToolBar->addSeparator();

    // Toolbar style
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_mainToolBar->addSeparator();

    // Settings popup button
    QAction *settingsAct = m_mainToolBar->addAction(getInvertedIcon(":/icons/settings.svg"), "Settings");
    settingsAct->setToolTip("Project Settings (canvas size, FPS, onion skin)");
    connect(settingsAct, &QAction::triggered, this, [this]() {
        // Guard: if m_projectSettings not yet created, skip
        if (!m_projectSettings) return;

        // Create a new dialog each time (cheap — settings widget is kept, dialog is not)
        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle("Project Settings");
        dlg->setModal(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->resize(380, 540);
        dlg->setStyleSheet(styleSheet());

        QVBoxLayout *lay = new QVBoxLayout(dlg);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        // Re-parent settings widget into dialog
        m_projectSettings->setParent(dlg);
        m_projectSettings->show();
        lay->addWidget(m_projectSettings, 1);

        const ThemeColors &ct = theme();
        QPushButton *closeBtn = new QPushButton("Close");
        closeBtn->setStyleSheet(
            QString("QPushButton { background:%1; color:white; border:none; border-radius:4px; padding:7px 24px; }"
                    "QPushButton:hover { background:%2; }").arg(ct.accent, ct.accentHover));
        connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->setContentsMargins(12, 8, 12, 12);
        btnRow->addStretch();
        btnRow->addWidget(closeBtn);
        lay->addLayout(btnRow);

        // When dialog closes, rescue the widget back to MainWindow so it
        // isn't destroyed (WA_DeleteOnClose would delete the dialog, not the widget)
        connect(dlg, &QDialog::finished, this, [this](int) {
            if (m_projectSettings) {
                m_projectSettings->setParent(this);
                m_projectSettings->hide(); // hide it — it's not in any layout now
            }
        });

        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    });
}

void MainWindow::createDockWindows()
{
    // Toolbox
    QDockWidget *toolDock = new QDockWidget("Tools", this);
    toolDock->setWidget(m_toolBox);
    toolDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    toolDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    toolDock->setMinimumWidth(sc(155));
    addDockWidget(Qt::LeftDockWidgetArea, toolDock);
    m_viewMenu->addAction(toolDock->toggleViewAction());

    // Utility
    QTabWidget *rightTabs = new QTabWidget();
    rightTabs->setObjectName("rightPanelTabs");

    rightTabs->setStyleSheet(
        "QTabWidget::pane { border: none; background-color: #282828; }"
        "QTabBar::tab { background-color: #333333; color: #aaa; padding: 8px 16px; border: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #282828; color: white; }"
        "QTabBar::tab:hover { background-color: #4a4a4a; }"
    );

    // Layers Tab
    rightTabs->addTab(m_layerPanel, "Layers");

    // Colors Tab
    rightTabs->addTab(m_colorPicker, "Colors");

    // Tool Options Tab — grab the panel that ToolBox already owns
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

    // When current layer changes to/from an audio layer, update tool settings panel
    connect(m_project, &Project::currentLayerChanged, this,
            [this, rightTabs, toolSettingsPanel](Layer *layer) {
        if (layer && layer->layerType() == LayerType::Audio) {
            toolSettingsPanel->showAudioLayerControls(layer);
            int idx = rightTabs->indexOf(toolSettingsPanel);
            if (idx >= 0) rightTabs->setCurrentIndex(idx);
        } else if (m_toolBox->currentTool()) {
            toolSettingsPanel->updateForTool(m_toolBox->currentTool()->type(),
                                              m_toolBox->currentTool());
        }
    });

    // Assets Tab
    rightTabs->addTab(m_assetLibrary, "Assets");

    m_projectSettings = new ProjectSettings(m_project, this);
    m_projectSettings->hide();

    // Connect the settings changed signal so the canvas updates in real-time
    connect(m_projectSettings, &ProjectSettings::themeChanged, this, [this](int index) {
        // Update the global ThemeManager first so all widgets can read new colors
        ThemeManager::instance().setTheme(index);
        const ThemeColors &t = theme();

        // Rebuild the global stylesheet with the new bg colors
        setStyleSheet(QString(R"(
            QMainWindow { background-color: %1; }
            QMenuBar { background-color: %1; color: #e0e0e0; border-bottom: 1px solid %2; padding: 2px; }
            QMenuBar::item { background-color: transparent; padding: 6px 12px; border-radius: 2px; }
            QMenuBar::item:selected { background-color: %2; color: %3; }
            QMenu { background-color: %4; color: #e0e0e0; border: 1px solid %2; padding: 4px; }
            QMenu::item { padding: 6px 24px 6px 12px; border-radius: 2px; }
            QMenu::item:selected { background-color: %3; color: white; }
            QMenu::separator { height: 1px; background: %5; margin: 4px 8px; }
            QToolBar { background-color: %1; border-bottom: 1px solid %2; border-top: none; spacing: 6px; padding: 4px; }
            QToolBar::separator { width: 1px; background: %5; margin: 6px; }
            QDockWidget { color: #999; font-weight: bold; font-size: 10px; }
            QDockWidget::title { background-color: %1; padding: 8px; border-bottom: 1px solid %2; text-align: left; letter-spacing: 1.5px; }
            QStatusBar { background-color: %1; color: #666; border-top: 1px solid %2; font-size: 10px; }
            QStatusBar::item { border: none; }
            QToolButton { background: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; }
            QToolButton:hover { background-color: %2; border: 1px solid %3; }
            QToolButton:checked { background-color: %3; color: white; }
        )").arg(t.bg0, t.bg1, t.accent, t.bg4, t.bg2));

        // Repaint dynamic painted widgets so their QPainter colors update
        if (m_timeline) { m_timeline->applyTheme(); m_timeline->update(); }
        if (m_layerPanel) m_layerPanel->applyTheme();
        if (m_colorPicker) m_colorPicker->applyTheme();
        if (m_toolBox) m_toolBox->applyTheme();
        if (m_toolBox && m_toolBox->settingsPanel()) m_toolBox->settingsPanel()->applyTheme();
        if (m_assetLibrary) m_assetLibrary->applyTheme();
        if (m_projectSettings) m_projectSettings->applyTheme();
    });
    connect(m_projectSettings, &ProjectSettings::settingsChanged, this, [this]() {
        m_canvas->setSceneRect(0, 0, m_project->width(), m_project->height());
        m_canvasView->resetCachedContent();
        m_canvasView->centerOn(m_canvas->sceneRect().center());
        m_canvas->update();
        m_timeline->update();
        // If the MIDI soundfont path changed, re-render all MIDI clips immediately
        m_timeline->rerenderMidiClips();
        m_isModified = true;
        updateWindowTitle();
    });

    // Wrap the tabs in a Dock Widget
    QDockWidget *rightDock = new QDockWidget("Panel", this);
    rightDock->setWidget(rightTabs);
    rightDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    rightDock->setMinimumWidth(sc(260));
    rightDock->setMaximumWidth(sc(480));
    rightDock->setTitleBarWidget(new QWidget()); // Hides the bulky "Panel" title bar
    addDockWidget(Qt::RightDockWidgetArea, rightDock);

    // timeline
    QDockWidget *timelineDock = new QDockWidget("Timeline", this);
    timelineDock->setWidget(m_timeline);
    timelineDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    timelineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    timelineDock->setMinimumHeight(sc(160));
    // Cap max height so the timeline never covers more than ~30% of the screen.
    // Users can still resize it up by dragging the dock separator.
    timelineDock->setMaximumHeight(sc(320));
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock);
    m_viewMenu->addAction(timelineDock->toggleViewAction());

    // Load persisted preferences on startup
    QSettings settings("AkisVG", "AkisVG");
    int savedTheme = settings.value("theme/index", 0).toInt();
    if (savedTheme != 0) {
        ThemeManager::instance().setTheme(savedTheme);
        // Re-apply theme to all panels now that they've been created
        if (m_timeline)        m_timeline->applyTheme();
        if (m_layerPanel)      m_layerPanel->applyTheme();
        if (m_colorPicker)     m_colorPicker->applyTheme();
        if (m_toolBox)         m_toolBox->applyTheme();
        if (m_toolBox && m_toolBox->settingsPanel()) m_toolBox->settingsPanel()->applyTheme();
        if (m_assetLibrary)    m_assetLibrary->applyTheme();
        if (m_projectSettings) m_projectSettings->applyTheme();
    }
    if (settings.contains("window/geometry"))
        restoreGeometry(settings.value("window/geometry").toByteArray());
    if (settings.contains("window/state"))
        restoreState(settings.value("window/state").toByteArray());
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
    m_canvas->clearDisplay();
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
      "AkisVG Projects (*.avg *.akisvg);;All Files (*)",  // Support both .avg and .akisvg
      nullptr,
      QFileDialog::DontUseNativeDialog
  );

    if (!fileName.isEmpty()) {
        if (m_project->loadFromFile(fileName)) {
            m_currentFile = fileName;
            m_isModified = false;
            updateWindowTitle();
            m_canvas->clearDisplay();
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
        "AkisVG Projects (*.avg);;Legacy Format (*.akisvg)");  // set .avg is default

    if (!fileName.isEmpty()) {
        // Ensure correct extension
        if (!fileName.endsWith(".avg", Qt::CaseInsensitive) &&
            !fileName.endsWith(".akisvg", Qt::CaseInsensitive)) {
            fileName += ".avg";
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
        addToRecentFiles(m_currentFile);
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
        // Determine export format from extension
        QString ext = QFileInfo(fileName).suffix().toLower();

        // Grab the current frame by rendering the scene into a QImage
        int w = m_project->width();
        int h = m_project->height();
        QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        m_canvas->render(&painter, QRectF(0, 0, w, h), QRectF(0, 0, w, h));
        painter.end();

        bool success = false;
        if (ext == "svg") {
            // Export as SVG using QSvgGenerator (available whenever QtSvg is linked)
#ifdef QT_SVG_LIB
            QSvgGenerator generator;
            generator.setFileName(fileName);
            generator.setSize(QSize(w, h));
            generator.setViewBox(QRect(0, 0, w, h));
            generator.setTitle(m_project->name());
            generator.setDescription("Exported by AkisVG");
            QPainter svgPainter(&generator);
            svgPainter.setRenderHint(QPainter::Antialiasing);
            m_canvas->render(&svgPainter, QRectF(0, 0, w, h), QRectF(0, 0, w, h));
            svgPainter.end();
            success = true;
#else
            // Fallback: save as PNG when QtSvg is not linked
            QString pngPath = QFileInfo(fileName).absolutePath() + "/" +
                              QFileInfo(fileName).completeBaseName() + ".png";
            success = image.save(pngPath, "PNG");
            statusBar()->showMessage("SVG export unavailable — saved as PNG instead.", 4000);
            return;
#endif
        } else {
            // PNG, JPEG, BMP, etc. — delegate to Qt's image writer
            success = image.save(fileName);
        }

        if (success) {
            statusBar()->showMessage("Frame exported: " + fileName, 3000);
        } else {
            QMessageBox::warning(this, "Export Failed",
                                 "Could not write file:\n" + fileName);
        }
    }
}

void MainWindow::applyStartupSettings(const QString &name, int width, int height, int fps)
{
    if (!name.isEmpty())
        setWindowTitle(QString("AkisVG  —  %1").arg(name));

    // Apply canvas size and fps to the project
    if (m_project) {
        if (width  > 0) m_project->setWidth(width);
        if (height > 0) m_project->setHeight(height);
        if (fps    > 0) m_project->setFps(fps);
    }

    // Sync canvas scene rect
    if (m_canvas)
        m_canvas->setSceneRect(0, 0, m_project->width(), m_project->height());

    m_isModified = false;
    updateWindowTitle();
}

void MainWindow::openProjectFile(const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) return;

    if (m_project->loadFromFile(path)) {
        m_currentFile = path;
        m_isModified  = false;
        updateWindowTitle();
        m_canvas->clearDisplay();
        m_canvas->refreshFrame();
        m_layerPanel->rebuildLayerList();
        statusBar()->showMessage("Project opened: " + path, 3000);
        addToRecentFiles(path);
    } else {
        QMessageBox::critical(this, "Load Error",
                              "Failed to open project from:\n" + path);
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, "About AkisVG",
        "<h2>AkisVG 0.4 <span style='color: #2a82da;'>Experimental</span></h2>"
        "<p><i>A high-performance, lightweight vector animation suite built for the modern creator.</i></p>"
        "<hr noshade size='1' style='background-color: #444;'>"

        "<p>AkisVG leverages <b>C++17</b> and <b>Qt 6</b> to provide a fluid, hardware-accelerated "
        "environment for both traditional and automated animation workflows.</p>"

        "<p><b>Latest Features:</b></p>"
        "<ul>"
        "<li><b>Smart Interpolation:</b> Automated motion tweening between keyframes using path-based logic.</li>"
        "<li><b>Pro Export Engine:</b> High-quality video rendering via <b>FFmpeg</b> (MP4/MKV) and optimized GIF generation.</li>"
        "<li><b>Multimedia Integration:</b> Full support for synchronized audio layers and image asset imports.</li>"
        "<li><b>Infinite Canvas:</b> GPU-accelerated 2D engine with real-time onion skinning and zoom.</li>"
        "</ul>"

        "<p style='color: #a0a0a0;'><small><b>System Info:</b> Running on Qt " QT_VERSION_STR " with FFmpeg Integration.</small></p>"
        "<p><small>Optimized for efficiency, precision, and creative freedom.</small></p>");
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

    // Persist preferences (theme, window geometry, etc.) on close
    if (event->isAccepted()) {
        QSettings settings("AkisVG", "AkisVG");
        settings.setValue("theme/index", ThemeManager::instance().currentIndex());
        settings.setValue("window/geometry", saveGeometry());
        settings.setValue("window/state", saveState());
    }
}

// MainWindow level before they get lost.
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Don't steal shortcuts when typing in input widgets
    QWidget *focus = QApplication::focusWidget();
    if (focus && (qobject_cast<QLineEdit*>(focus) ||
                  qobject_cast<QTextEdit*>(focus)  ||
                  qobject_cast<QSpinBox*>(focus)   ||
                  qobject_cast<QDoubleSpinBox*>(focus) ||
                  qobject_cast<QComboBox*>(focus))) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    // Don't intercept modifier-combos (those are menu shortcuts)
    if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    static const QMap<int, ToolType> keyToolMap = {
        { Qt::Key_V, ToolType::Select    },
        { Qt::Key_I, ToolType::eyedropper},
        { Qt::Key_L, ToolType::Lasso     },
        { Qt::Key_W, ToolType::MagicWand },
        { Qt::Key_P, ToolType::Pencil    },
        { Qt::Key_B, ToolType::Brush     },
        { Qt::Key_E, ToolType::Eraser    },
        { Qt::Key_G, ToolType::Fill      },
        { Qt::Key_D, ToolType::Gradient  },
        { Qt::Key_H, ToolType::Blend     },
        { Qt::Key_K, ToolType::Liquify   },
        { Qt::Key_R, ToolType::Rectangle },
        { Qt::Key_C, ToolType::Ellipse   },
        { Qt::Key_U, ToolType::Line      },
        { Qt::Key_T, ToolType::Text      },
    };

    auto it = keyToolMap.find(event->key());
    if (it != keyToolMap.end()) {
        m_toolBox->activateTool(it.value());
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
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
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export to Video", "",
        "MP4 Video (*.mp4);;MKV Video (*.mkv);;All Files (*)");

    if (fileName.isEmpty())
        return;

    QString format = "mp4";
    if (fileName.endsWith(".mkv", Qt::CaseInsensitive))
        format = "mkv";
    else if (!fileName.endsWith(".mp4", Qt::CaseInsensitive))
        fileName += ".mp4";

    // Export only up to the last frame that has actual content
    int startFrame = 1;
    int endFrame   = qMax(1, m_project->highestUsedFrame());
    int fps        = m_project->fps();

    //  Locate ffmpeg binary (QProcess::start(singleString) does NOT invoke
    //    a shell on Linux, so we must pass program + args separately)
    QString ffmpegBin = QStandardPaths::findExecutable("ffmpeg");
    if (ffmpegBin.isEmpty()) {
        for (const char *p : {"/usr/bin/ffmpeg", "/usr/local/bin/ffmpeg", "/bin/ffmpeg"})
            if (QFile::exists(p)) { ffmpegBin = p; break; }
    }
    if (ffmpegBin.isEmpty()) {
        QMessageBox::critical(this, "Export Error",
            "ffmpeg was not found.\n\nInstall it with:  sudo pacman -S ffmpeg");
        return;
    }

    QProgressDialog progress("Exporting frames…", "Cancel", 0, endFrame, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    QString tempDir = QDir::temp().filePath("akisvg_export_" +
                      QString::number(QDateTime::currentMSecsSinceEpoch()));
    QDir().mkpath(tempDir);

    int savedFrame = m_project->currentFrame();

    for (int frame = startFrame; frame <= endFrame; ++frame) {
        if (progress.wasCanceled()) {
            QDir(tempDir).removeRecursively();
            m_project->setCurrentFrame(savedFrame);
            m_canvas->refreshFrame();
            return;
        }

        m_project->setCurrentFrame(frame);
        m_canvas->refreshFrame();

        QImage image(m_project->width(), m_project->height(), QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        m_canvas->render(&painter,
                          QRectF(0, 0, m_project->width(), m_project->height()),
                          QRectF(0, 0, m_project->width(), m_project->height()));
        painter.end();

        image.save(tempDir + QString("/frame_%1.png").arg(frame, 6, 10, QChar('0')), "PNG");
        progress.setValue(frame);
        QApplication::processEvents();
    }

    // Build argument list NOT a shell string
    QStringList args;
    args << "-y"
         << "-framerate"    << QString::number(fps)
         << "-start_number" << QString::number(startFrame)
         << "-i"            << (tempDir + "/frame_%06d.png")
         << "-c:v"          << "libx264"
         << "-preset"       << (format == "mkv" ? "medium" : "slow")
         << "-crf"          << "18"
         << "-pix_fmt"      << "yuv420p"
         << fileName;

    progress.setLabelText(QString("Encoding %1…").arg(format.toUpper()));
    progress.setRange(0, 0);
    QApplication::processEvents();

    QProcess ffmpeg;
    ffmpeg.setProgram(ffmpegBin);
    ffmpeg.setArguments(args);
    ffmpeg.setProcessChannelMode(QProcess::MergedChannels);
    ffmpeg.start();

    if (!ffmpeg.waitForStarted(5000)) {
        QMessageBox::critical(this, "Export Error",
            QString("Failed to launch ffmpeg:\n%1\n\n%2")
            .arg(ffmpegBin, ffmpeg.errorString()));
        QDir(tempDir).removeRecursively();
        m_project->setCurrentFrame(savedFrame);
        m_canvas->refreshFrame();
        return;
    }

    while (!ffmpeg.waitForFinished(200)) {
        QApplication::processEvents();
        if (progress.wasCanceled()) { ffmpeg.kill(); break; }
    }

    QDir(tempDir).removeRecursively();
    m_project->setCurrentFrame(savedFrame);
    m_canvas->refreshFrame();

    if (ffmpeg.exitCode() == 0) {
        statusBar()->showMessage(
            QString("%1 exported: %2").arg(format.toUpper(), QFileInfo(fileName).fileName()), 5000);
        QMessageBox::information(this, "Export Complete",
            QString("Exported to:\n%1\n\n%2  |  %3 frames  |  %4 fps")
            .arg(fileName, format.toUpper()).arg(endFrame).arg(fps));
    } else {
        QMessageBox::critical(this, "Export Error",
            QString("ffmpeg exited %1.\n\n%2")
            .arg(ffmpeg.exitCode()).arg(QString(ffmpeg.readAll()).right(2000)));
    }
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

    connect(&exporter, &GifExporter::frameExported, [&progress](int current, int /*total*/) {
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
    // Support all common audio formats + MIDI
    QString fileName = QFileDialog::getOpenFileName(
        this, "Import Audio / Sound", "",
        "All Supported Audio (*.mp3 *.wav *.ogg *.m4a *.flac *.aac *.opus *.wma *.mid *.midi);;"
        "WAV Files (*.wav);;"
        "MP3 Files (*.mp3);;"
        "OGG / Vorbis (*.ogg);;"
        "FLAC Lossless (*.flac);;"
        "AAC / M4A (*.aac *.m4a);;"
        "Opus (*.opus);;"
        "MIDI Files (*.mid *.midi);;"
        "All Files (*)");

    if (fileName.isEmpty()) return;

    if (!QFile::exists(fileName)) {
        QMessageBox::warning(this, "Import Error", "Audio file not found: " + fileName);
        return;
    }

    Layer *currentLayer = m_project->currentLayer();
    if (!currentLayer) {
        QMessageBox::warning(this, "Import Error",
            "No layer selected. Please create or select a layer first.");
        return;
    }

    // Ensure the layer is an audio layer
    if (currentLayer->layerType() != LayerType::Audio) {
        QMessageBox::warning(this, "Import Error",
            "Selected layer is not an Audio layer.\nAdd an audio layer first.");
        return;
    }

    bool isMidi = fileName.endsWith(".mid", Qt::CaseInsensitive) ||
                  fileName.endsWith(".midi", Qt::CaseInsensitive);

    // For MIDI: check that FluidSynth + a soundfont are available before importing
    if (isMidi) {
        QString fluidsynth = QStandardPaths::findExecutable("fluidsynth");
        if (fluidsynth.isEmpty()) {
            QMessageBox::warning(this, "MIDI Import — FluidSynth Not Found",
                "MIDI playback requires FluidSynth to render audio.\n\n"
                "Install FluidSynth and a GM soundfont:\n"
                "  • Arch:    sudo pacman -S fluidsynth soundfont-fluid\n"
                "  • Ubuntu:  sudo apt install fluidsynth fluid-soundfont-gm\n"
                "  • Fedora:  sudo dnf install fluidsynth fluid-soundfont\n\n"
                "You can also set a custom soundfont path in Settings → Audio → MIDI Soundfont.\n\n"
                "Without FluidSynth the MIDI clip will be shown in the timeline\n"
                "but will play silently.");
        }
    }

    // Create audio clip — duration = -1 (Auto, uses natural length)
    AudioData audioData(fileName, m_project->currentFrame(), -1);
    audioData.volume = 1.0f;
    audioData.muted  = false;
    audioData.isMidi = isMidi;

    // Add clip to layer (multi-clip)
    currentLayer->addAudioClip(audioData);

    // Update UI
    m_canvas->refreshFrame();
    m_layerPanel->rebuildLayerList();

    statusBar()->showMessage(
        QString("Audio clip added: %1").arg(QFileInfo(fileName).fileName()), 5000);
}

void MainWindow::importImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "Import Image", "",
        "Image Files (*.png *.jpg *.jpeg *.bmp *.webp *.gif);;All Files (*)");

    if (fileName.isEmpty())
        return;

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

    // ── Use TransformableImageObject so the user gets interactive handles ──
    auto *imgObj = new TransformableImageObject(image);

    // Centre on canvas
    imgObj->setPosition(QPointF(m_project->width()  / 2.0,
                                m_project->height() / 2.0));

    currentLayer->addObjectToFrame(m_project->currentFrame(), imgObj);

    // Select immediately so handles appear right away
    m_canvasView->setSelectedImage(imgObj);

    m_canvas->refreshFrame();
    m_isModified = true;
    updateWindowTitle();

    statusBar()->showMessage(QString("Image imported: %1 (%2×%3 px)")
        .arg(QFileInfo(fileName).fileName())
        .arg(image.width())
        .arg(image.height()), 5000);
}

void MainWindow::onSplineCommitted(const QList<QPointF> &nodes)
{
    if (nodes.size() < 2 || !m_project || !m_project->currentLayer()) {
        statusBar()->showMessage(QString("Spline commit failed: nodes=%1 project=%2 layer=%3")
            .arg(nodes.size()).arg(m_project ? 1 : 0)
            .arg((m_project && m_project->currentLayer()) ? 1 : 0), 5000);
        return;
    }

    // Use the targets snapshotted when interpolation mode was entered —
    // the live selection is empty by now because the overlay stole focus.
    QList<VectorObject*> targets = m_interpolationTargets;
    m_interpolationTargets.clear();

    if (targets.isEmpty()) {
        statusBar()->showMessage("Spline commit failed: no targets were saved — select objects BEFORE right-clicking Interpolate", 5000);
        m_splineOverlay->clearNodes();
        m_splineOverlay->hide();
        m_canvas->exitInterpolationMode();
        return;
    }

    QPainterPath scenePath;
    scenePath.moveTo(m_canvasView->mapToScene(nodes.first().toPoint()));
    for (int i = 1; i < nodes.size(); ++i)
        scenePath.lineTo(m_canvasView->mapToScene(nodes[i].toPoint()));

    int startFrame = m_project->currentFrame();
    int duration   = m_interpTotalFrames;

    // Compute each object's scene-space center on the start frame so we can
    // move it by the delta to the path point — not re-center from boundingRect origin.
    // (PathObject stores strokes in scene coords with pos()=(0,0), so boundingRect()
    //  is already in scene space and setPos() would double-offset it.)
    QList<QPointF> startCenters;
    for (VectorObject *obj : targets) {
        QRectF br = obj->boundingRect();
        QPointF localCenter = br.center();
        QPointF sceneCenter = obj->mapToScene(localCenter);
        startCenters.append(sceneCenter);
    }

    QPointF pathStart = scenePath.pointAtPercent(0.0);

    // Build the list of (targetFrame, t) pairs to generate
    // Advanced mode: use per-node times to map each path node to a specific frame,
    //   then interpolate between nodes for in-between frames.
    // Basic mode: evenly distribute duration frames along the path.
    QList<QPair<int,qreal>> frames; // {targetFrame, t along scenePath}

    if (m_interpAdvanced && m_interpKeyframeTimes.size() >= 2
        && m_interpKeyframeTimes.size() == nodes.size()) {
        // Advanced: for each segment between consecutive nodes, generate frames
        int totalNodes = nodes.size();
        for (int seg = 0; seg < totalNodes - 1; ++seg) {
            int frameA = m_interpKeyframeTimes[seg];
            int frameB = m_interpKeyframeTimes[seg + 1];
            qreal tA = static_cast<qreal>(seg)     / (totalNodes - 1);
            qreal tB = static_cast<qreal>(seg + 1) / (totalNodes - 1);
            int segFrames = qAbs(frameB - frameA);
            for (int f = (seg == 0 ? 1 : 0); f <= segFrames; ++f) {
                qreal segT = static_cast<qreal>(f) / segFrames;
                qreal t = tA + segT * (tB - tA);
                frames.append({startFrame + frameA + f, t});
            }
        }
        duration = frames.isEmpty() ? 0 : (frames.last().first - startFrame);
    } else {
        // Basic: evenly space `duration` frames
        for (int i = 1; i <= duration; ++i) {
            qreal t = static_cast<qreal>(i) / duration;
            frames.append({startFrame + i, t});
        }
    }

    for (auto &[targetFrame, t] : frames) {
        QPointF pathPoint = scenePath.pointAtPercent(t);
        QPointF delta = pathPoint - pathStart;

        for (int oi = 0; oi < targets.size(); ++oi) {
            VectorObject *obj = targets[oi];
            VectorObject *frameClone = obj->clone();
            QPointF newSceneCenter = startCenters[oi] + delta;
            QRectF br = frameClone->boundingRect();
            QPointF localCenter = br.center();
            frameClone->setPos(newSceneCenter - localCenter);
            m_project->currentLayer()->addMotionPathObjectToFrame(targetFrame, frameClone);
        }
    }

    m_splineOverlay->clearNodes();
    m_splineOverlay->hide();
    m_canvas->exitInterpolationMode();
    m_canvas->refreshFrame();

    statusBar()->showMessage(QString("Created %1 frame animation with %2 object(s)")
        .arg(duration).arg(targets.size()));
}

void MainWindow::onInstanceGroupRequested(ObjectGroup *group)
{
    if (!group || !m_project) return;

    // Change qobject_cast to dynamic_cast
    ObjectGroup *newInstance = dynamic_cast<ObjectGroup*>(group->clone());

    if (!newInstance) return; // Safety check

    // Determine the target position (Center of the visible screen)
    QPointF targetPos = m_canvasView->mapToScene(m_canvasView->viewport()->rect().center());

    // Offset the position so the center of the art is at the cursor
    QRectF bounds = newInstance->boundingRect();
    newInstance->setPos(targetPos.x() - bounds.width()/2,
                        targetPos.y() - bounds.height()/2);

    Layer *currentLayer = m_project->currentLayer();
    if (currentLayer) {
        currentLayer->addObjectToFrame(m_project->currentFrame(), newInstance);
    }

    m_canvas->addItem(newInstance);
    m_canvas->update();
}

void MainWindow::startInterpolationMode() {
    if (!m_splineOverlay) return;
    // canvasView->geometry() which is in the parent widget's coordinate space.
    m_splineOverlay->resize(m_canvasView->viewport()->size());
    m_splineOverlay->move(0, 0);
    m_splineOverlay->show();

    // These two lines stop the "stuck" behavior
    m_splineOverlay->raise();
    m_splineOverlay->setFocus(Qt::OtherFocusReason);

    statusBar()->showMessage("Draw motion path. [ENTER] to Finish, [ESC] to Cancel.");
}

// simple Douglas-Peucker-style decimation before passing it to boolean ops.

static QPolygonF decimatePolygon(const QPolygonF &poly, int maxPts)
{
    // Fast path: already small enough
    if (poly.size() <= maxPts) return poly;

    // Uniform stride decimation — good enough for intersection/subtraction.
    // For a true Douglas-Peucker we'd need more code; stride is adequate here.
    QPolygonF out;
    out.reserve(maxPts + 1);
    double step = static_cast<double>(poly.size() - 1) / (maxPts - 1);
    for (int i = 0; i < maxPts; ++i) {
        int idx = qRound(i * step);
        out << poly.at(qBound(0, idx, poly.size() - 1));
    }
    if (out.last() != poly.last()) out << poly.last(); // always include endpoint
    return out;
}

// Convert a QPainterPath to a flat polygon (element-wise), decimate if large,
// then back to a QPainterPath suitable for safe boolean ops.
static QPainterPath safePath(const QPainterPath &src)
{
    constexpr int MAX_ELEMS = 1500;
    if (src.elementCount() <= MAX_ELEMS) return src;

    // Flatten to polygon
    const QPolygonF flat = src.toFillPolygon();
    const QPolygonF slim = decimatePolygon(flat, MAX_ELEMS);
    QPainterPath out;
    out.addPolygon(slim);
    out.closeSubpath();
    return out;
}

// Returns true if the display clone `obj` (in the scene) intersects `poly` (scene coords).
// PathObject stores paths in scene coords with pos()=(0,0), so we use scenePos()+boundingRect().
static bool objectIntersectsPoly(VectorObject *obj, const QPolygonF &poly)
{
    // Map the object's bounding rect to scene coordinates
    QRectF br     = obj->boundingRect();
    QPointF sp    = obj->scenePos();
    QRectF sceneR = br.translated(sp);

    // Quick reject
    if (!poly.boundingRect().intersects(sceneR))
        return false;

    // Test all four corners + center; also test whether the poly center is inside the rect
    QVector<QPointF> testPts = {
        sceneR.topLeft(), sceneR.topRight(),
        sceneR.bottomLeft(), sceneR.bottomRight(),
        sceneR.center()
    };
    for (const QPointF &pt : testPts)
        if (poly.containsPoint(pt, Qt::WindingFill))
            return true;

    // Also test poly vertices inside the bounding rect
    for (const QPointF &pt : poly)
        if (sceneR.contains(pt))
            return true;

    return false;
}

void MainWindow::onLassoFill(const QPolygonF &poly, const QColor &color)
{
    if (!m_project || !m_project->currentLayer()) return;

    if (m_lassoTool && m_lassoTool->fillMode()) {
        if (poly.size() < 3) return; // need at least a triangle

        // Build a closed QPainterPath from the placed points
        QPainterPath polyPath;
        polyPath.moveTo(poly.first());
        for (int i = 1; i < poly.size(); ++i)
            polyPath.lineTo(poly[i]);
        polyPath.closeSubpath();

        PathObject *obj = new PathObject();
        obj->setPath(polyPath);
        obj->setFillColor(color);
        // No stroke by default in fill-tool mode — use a thin same-colour outline
        obj->setStrokeColor(color.darker(130));
        obj->setStrokeWidth(1.0);
        obj->setObjectOpacity(1.0);

        Layer *layer = m_project->currentLayer();
        layer->addObjectToFrame(m_project->currentFrame(), obj);
        m_canvas->refreshFrame();
        m_isModified = true;
        updateWindowTitle();
        return;
    }

    // Normal lasso mode: fill colour of intersecting objects
    bool changed = false;
    for (QGraphicsItem *item : m_canvas->items()) {
        auto *obj = dynamic_cast<VectorObject*>(item);
        if (!obj) continue;
        if (!objectIntersectsPoly(obj, poly)) continue;

        VectorObject *src = m_canvas->sourceObject(obj);
        if (!src) src = obj;
        src->setFillColor(color);
        changed = true;
    }
    if (changed) m_canvas->refreshFrame();
}

void MainWindow::onLassoCut(const QPolygonF &poly)
{

    if (!m_project || !m_project->currentLayer()) return;

    Layer *layer = m_project->currentLayer();
    int   frame  = m_project->currentFrame();

    // Build the clip path from the lasso polygon (scene coords)
    QPainterPath lassoPath;
    lassoPath.addPolygon(poly);
    lassoPath.closeSubpath();

    // Collect display items first (don't modify while iterating)
    QVector<VectorObject*> displayItems;
    for (QGraphicsItem *item : m_canvas->items()) {
        if (auto *obj = dynamic_cast<VectorObject*>(item))
            displayItems << obj;
    }

    bool anyChange = false;

    for (VectorObject *display : displayItems) {
        if (!objectIntersectsPoly(display, poly)) continue;

        VectorObject *src = m_canvas->sourceObject(display);
        if (!src) src = display;

        // PathObject split
        if (auto *path = dynamic_cast<PathObject*>(src)) {

            // segfault Qt's internal edge-list algorithm at > ~2000 elements.
            QPainterPath pp = safePath(path->path());
            QPointF offset = src->pos();
            QPainterPath lassoLocal = lassoPath.translated(-offset);

            QPainterPath insidePart  = pp.intersected(lassoLocal);
            QPainterPath outsidePart = pp.subtracted(lassoLocal);

            bool hasInside  = !insidePart.isEmpty()  && insidePart.boundingRect().width()  > 0.5;
            bool hasOutside = !outsidePart.isEmpty() && outsidePart.boundingRect().width() > 0.5;

            if (!hasInside && !hasOutside) {
                // Fully inside lasso — remove
                m_canvas->removeObject(display);
                anyChange = true;
                continue;
            }

            if (hasInside && hasOutside) {
                // add a new object for the inside piece
                path->setPath(outsidePart);
                path->update();

                PathObject *insideObj = new PathObject();
                insideObj->setPath(insidePart);
                insideObj->setPos(src->pos());
                insideObj->setStrokeColor(path->strokeColor());
                insideObj->setFillColor(path->fillColor());
                insideObj->setStrokeWidth(path->strokeWidth());
                insideObj->setObjectOpacity(path->objectOpacity());
                insideObj->setZValue(path->zValue() + 0.001);
                layer->addObjectToFrame(frame, insideObj);
                anyChange = true;

            } else if (hasInside && !hasOutside) {
                // Object is entirely inside the lasso — just remove it
                m_canvas->removeObject(display);
                anyChange = true;

            } else {
                // Object is entirely outside — nothing to do
            }
            continue;
        }

        // ShapeObject split
        if (auto *shape = dynamic_cast<ShapeObject*>(src)) {
            // Shapes store their path differently — build a painter path from the rect
            QPainterPath shapePath;
            QPointF offset = src->pos();
            if (shape->shapeType() == ShapeObject::Rectangle)
                shapePath.addRect(shape->rect().translated(offset));
            else
                shapePath.addEllipse(shape->rect().translated(offset));

            QPainterPath insidePart  = shapePath.intersected(lassoPath);
            QPainterPath outsidePart = shapePath.subtracted(lassoPath);

            bool hasInside  = !insidePart.isEmpty()  && insidePart.boundingRect().width()  > 0.5;
            bool hasOutside = !outsidePart.isEmpty() && outsidePart.boundingRect().width() > 0.5;

            if (!hasInside) continue; // lasso doesn't cover the shape

            if (hasInside && hasOutside) {
                // Create two PathObjects from the split shape
                PathObject *insideObj = new PathObject();
                insideObj->setPath(insidePart.translated(-offset));
                insideObj->setPos(offset);
                insideObj->setFillColor(shape->fillColor());
                insideObj->setStrokeColor(shape->strokeColor());
                insideObj->setStrokeWidth(shape->strokeWidth());
                insideObj->setObjectOpacity(shape->objectOpacity());
                insideObj->setZValue(shape->zValue() + 0.001);

                PathObject *outsideObj = new PathObject();
                outsideObj->setPath(outsidePart.translated(-offset));
                outsideObj->setPos(offset);
                outsideObj->setFillColor(shape->fillColor());
                outsideObj->setStrokeColor(shape->strokeColor());
                outsideObj->setStrokeWidth(shape->strokeWidth());
                outsideObj->setObjectOpacity(shape->objectOpacity());
                outsideObj->setZValue(shape->zValue());

                // Remove original shape, add both pieces
                m_canvas->removeObject(display);
                layer->addObjectToFrame(frame, insideObj);
                layer->addObjectToFrame(frame, outsideObj);
                anyChange = true;

            } else if (hasInside && !hasOutside) {
                m_canvas->removeObject(display);
                anyChange = true;
            }
            continue;
        }

        // ImageObject or other — just remove
        m_canvas->removeObject(display);
        anyChange = true;
    }

    if (anyChange) {
        m_canvas->refreshFrame();
        m_isModified = true;
        updateWindowTitle();
    }
}

void MainWindow::onLassoCopy(const QPolygonF &poly)
{
    // Render the full canvas to an image, then mask to the lasso polygon.
    // so it can be pasted anywhere (other apps, or back into AkisVG via Import Image).
    QRectF sceneR(0, 0, m_project->width(), m_project->height());

    QImage snapshot(sceneR.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    snapshot.fill(Qt::transparent);
    QPainter p(&snapshot);
    p.setRenderHint(QPainter::Antialiasing);
    m_canvas->render(&p, sceneR, sceneR);
    p.end();

    // Clip to polygon shape
    QImage masked(snapshot.size(), QImage::Format_ARGB32_Premultiplied);
    masked.fill(Qt::transparent);
    QPainter mp(&masked);
    mp.setRenderHint(QPainter::Antialiasing);
    QPainterPath clipPath;
    clipPath.addPolygon(poly);
    mp.setClipPath(clipPath);
    mp.drawImage(0, 0, snapshot);
    mp.end();

    // Crop to the polygon bounding rect so the clipboard image isn't canvas-sized
    QRect cropRect = poly.boundingRect().toRect().intersected(snapshot.rect());
    QGuiApplication::clipboard()->setImage(masked.copy(cropRect));

    statusBar()->showMessage(
        QString("Copied selection (%1×%2 px) — paste with Ctrl+V or File > Import Image")
        .arg(cropRect.width()).arg(cropRect.height()), 4000);
}

void MainWindow::onLassoPull(const QPolygonF &poly, QPointF /*dragStart*/)
{
    //    This uses the same boolean-op split logic as onLassoCut, but instead
    //    of discarding the pieces it hands them to SelectTool for dragging.

    if (!m_project || !m_project->currentLayer()) return;

    Layer *layer = m_project->currentLayer();
    int    frame = m_project->currentFrame();

    QPainterPath lassoPath;
    lassoPath.addPolygon(poly);
    lassoPath.closeSubpath();

    // Snapshot display items before we modify the layer
    QVector<VectorObject*> displayItems;
    for (QGraphicsItem *item : m_canvas->items()) {
        if (auto *obj = dynamic_cast<VectorObject*>(item))
            displayItems << obj;
    }

    QList<VectorObject*> pulledPieces; // pieces that end up inside the lasso
    bool anyChange = false;

    for (VectorObject *display : displayItems) {
        if (!objectIntersectsPoly(display, poly)) continue;

        VectorObject *src = m_canvas->sourceObject(display);
        if (!src) src = display;

        // PathObject
        if (auto *path = dynamic_cast<PathObject*>(src)) {
            // Guard against segfault on pressure-data-heavy paths
            QPainterPath pp = safePath(path->path());
            QPointF offset  = src->pos();
            QPainterPath lassoLocal = lassoPath.translated(-offset);

            QPainterPath insidePart  = pp.intersected(lassoLocal);
            QPainterPath outsidePart = pp.subtracted(lassoLocal);

            bool hasInside  = !insidePart.isEmpty()
                              && insidePart.boundingRect().width()  > 0.5;
            bool hasOutside = !outsidePart.isEmpty()
                              && outsidePart.boundingRect().width() > 0.5;

            if (hasInside && hasOutside) {
                // Modify original to keep the outside piece
                path->setPath(outsidePart);
                path->update();

                // Create a new object for the inside piece — this is what gets pulled
                PathObject *insideObj = new PathObject();
                insideObj->setPath(insidePart);
                insideObj->setPos(offset);
                insideObj->setStrokeColor(path->strokeColor());
                insideObj->setFillColor(path->fillColor());
                insideObj->setStrokeWidth(path->strokeWidth());
                insideObj->setObjectOpacity(path->objectOpacity());
                insideObj->setZValue(path->zValue() + 0.001);
                layer->addObjectToFrame(frame, insideObj);
                pulledPieces << insideObj;
                anyChange = true;

            } else if (hasInside && !hasOutside) {
                // Whole object is inside — pull the entire object
                pulledPieces << src;
                // Don't remove — it stays, just gets selected for dragging
                anyChange = true;
            }
            continue;
        }

        // ShapeObject
        if (auto *shape = dynamic_cast<ShapeObject*>(src)) {
            QPointF offset = src->pos();
            QPainterPath shapePath;
            if (shape->shapeType() == ShapeObject::Rectangle)
                shapePath.addRect(shape->rect().translated(offset));
            else
                shapePath.addEllipse(shape->rect().translated(offset));

            QPainterPath insidePart  = shapePath.intersected(lassoPath);
            QPainterPath outsidePart = shapePath.subtracted(lassoPath);

            bool hasInside  = !insidePart.isEmpty()
                              && insidePart.boundingRect().width()  > 0.5;
            bool hasOutside = !outsidePart.isEmpty()
                              && outsidePart.boundingRect().width() > 0.5;

            if (!hasInside) continue;

            if (hasInside && hasOutside) {
                PathObject *insideObj = new PathObject();
                insideObj->setPath(insidePart.translated(-offset));
                insideObj->setPos(offset);
                insideObj->setFillColor(shape->fillColor());
                insideObj->setStrokeColor(shape->strokeColor());
                insideObj->setStrokeWidth(shape->strokeWidth());
                insideObj->setObjectOpacity(shape->objectOpacity());
                insideObj->setZValue(shape->zValue() + 0.001);

                PathObject *outsideObj = new PathObject();
                outsideObj->setPath(outsidePart.translated(-offset));
                outsideObj->setPos(offset);
                outsideObj->setFillColor(shape->fillColor());
                outsideObj->setStrokeColor(shape->strokeColor());
                outsideObj->setStrokeWidth(shape->strokeWidth());
                outsideObj->setObjectOpacity(shape->objectOpacity());
                outsideObj->setZValue(shape->zValue());

                m_canvas->removeObject(display);
                layer->addObjectToFrame(frame, insideObj);
                layer->addObjectToFrame(frame, outsideObj);
                pulledPieces << insideObj;
                anyChange = true;

            } else if (hasInside && !hasOutside) {
                pulledPieces << src;
                anyChange = true;
            }
            continue;
        }
    }

    if (!anyChange || pulledPieces.isEmpty()) return;

    // Refresh the canvas so the new split objects appear as display clones
    m_canvas->refreshFrame();
    m_isModified = true;
    updateWindowTitle();

    // Switch to Select tool and pre-select the pulled pieces
    m_toolBox->activateTool(ToolType::Select);

    SelectTool *sel = qobject_cast<SelectTool*>(m_toolBox->getTool(ToolType::Select));
    if (sel) {
        sel->clearSelection();
        sel->setSelectedObjects(pulledPieces);
    }

    statusBar()->showMessage(
        QString("Pulled %1 piece(s) — drag to reposition, click elsewhere to deselect")
        .arg(pulledPieces.size()), 4000);
}

void MainWindow::provideWandSnapshot(MagicWandTool *wand)
{
    if (!wand || !m_project) return;
    QRectF sceneR(0, 0, m_project->width(), m_project->height());
    QImage snapshot(sceneR.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    snapshot.fill(Qt::white);  // white background matches canvas
    QPainter p(&snapshot);
    p.setRenderHint(QPainter::Antialiasing);
    m_canvas->render(&p, sceneR, sceneR);
    p.end();
    wand->setCanvasSnapshot(snapshot, sceneR.topLeft());
}

// Recent files
void MainWindow::addToRecentFiles(const QString &path)
{
    QSettings s("AkisVG", "AkisVG");
    QStringList r = s.value("recentFiles").toStringList();
    r.removeAll(path);
    r.prepend(path);
    if (r.size() > 10) r.resize(10);
    s.setValue("recentFiles", r);
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    if (!m_recentMenu) return;
    m_recentMenu->clear();
    QSettings s("AkisVG", "AkisVG");
    QStringList r = s.value("recentFiles").toStringList();
    if (r.isEmpty()) {
        auto *e = m_recentMenu->addAction("(No recent files)");
        e->setEnabled(false);
        return;
    }
    for (const QString &p : r) {
        QAction *a = m_recentMenu->addAction(QFileInfo(p).fileName() + "   \xe2\x80\x94   " + p);
        connect(a, &QAction::triggered, this, [this, p]() {
            if (QFile::exists(p)) openProjectFile(p);
            else QMessageBox::warning(this, "File Not Found", "Cannot find:\n" + p);
        });
    }
    m_recentMenu->addSeparator();
    auto *cl = m_recentMenu->addAction("Clear Recent Files");
    connect(cl, &QAction::triggered, this, [this]() {
        QSettings("AkisVG","AkisVG").remove("recentFiles");
        updateRecentFilesMenu();
    });
}
