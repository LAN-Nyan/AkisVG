#include "timelinewidget.h"
#include "core/project.h"
#include "core/layer.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimerEvent>
#include <QSplitter>
#include <QScrollArea>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QScrollBar>
#include <QAction>
#include <QInputDialog>

// --- Internal Helper: LayerListWidget ---
class LayerListWidget : public QWidget {
public:
    LayerListWidget(Project *project, QWidget *parent = nullptr)
        : QWidget(parent), m_project(project) {
        setMinimumWidth(200);
        setMaximumWidth(300);
        setMinimumHeight(32 + m_project->layerCount() * 36);

        connect(project, &Project::layersChanged, this, [this]() {
            setMinimumHeight(32 + m_project->layerCount() * 36);
            update();
        });
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), QColor(30, 30, 30));

        // Header
        painter.fillRect(0, 0, width(), 32, QColor(40, 40, 40));
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(rect().adjusted(12, 0, 0, -height() + 32), Qt::AlignLeft | Qt::AlignVCenter, "LAYERS");

        int y = 32;
        int rowHeight = 36;
        auto layers = m_project->layers();

        for (int i = layers.size() - 1; i >= 0; --i) {
            Layer *layer = layers[i];
            bool isCurrent = (m_project->currentLayer() == layer);

            QColor bgColor = isCurrent ? QColor(42, 130, 218, 40) : QColor(30, 30, 30);
            painter.fillRect(0, y, width(), rowHeight, bgColor);

            // Color strip
            painter.fillRect(8, y + 8, 4, rowHeight - 16, layer->color());

            painter.setPen(layer->isVisible() ? QColor(220, 220, 220) : QColor(100, 100, 100));
            painter.setFont(QFont("Arial", 10));
            painter.drawText(QRect(20, y, width() - 80, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, layer->name());

            QString visIcon = layer->isVisible() ? "👁" : "○";
            painter.setFont(QFont("Arial", 12));
            painter.drawText(QRect(width() - 65, y, 25, rowHeight), Qt::AlignCenter, visIcon);

            if (layer->isLocked()) {
                painter.drawText(QRect(width() - 35, y, 25, rowHeight), Qt::AlignCenter, "🔒");
            }

            painter.setPen(QColor(20, 20, 20));
            painter.drawLine(0, y + rowHeight - 1, width(), y + rowHeight - 1);
            y += rowHeight;
        }
    }

    void mousePressEvent(QMouseEvent *event) override {
        int y = event->pos().y();
        if (y < 32) return;

        int rowHeight = 36;
        int index = (y - 32) / rowHeight;
        int layerIndex = m_project->layers().size() - 1 - index;

        if (layerIndex >= 0 && layerIndex < m_project->layers().size()) {
            if (event->pos().x() > width() - 65 && event->pos().x() < width() - 40) {
                Layer *layer = m_project->layers()[layerIndex];
                layer->setVisible(!layer->isVisible());
                update();
            } else {
                m_project->setCurrentLayer(layerIndex);
                update();
            }
        }
    }

private:
    Project *m_project;
};

// --- FrameGridWidget Implementation ---

FrameGridWidget::FrameGridWidget(Project *project, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_onionFrames(2)
    , m_isDragging(false)
    , m_onionSkinEnabled(false)
{
    setMinimumHeight(200);
    connect(project, &Project::currentFrameChanged, this, QOverload<>::of(&QWidget::update));
    connect(project, &Project::layersChanged, this, QOverload<>::of(&QWidget::update));
}

QSize FrameGridWidget::sizeHint() const {
    return QSize(m_project->totalFrames() * 16, 200);
}

void FrameGridWidget::setOnionSkin(bool enabled, int frames) {
    m_onionSkinEnabled = enabled;
    m_onionFrames = frames;
    update();
}

void FrameGridWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(20, 20, 20));

    const int cellWidth = 16;
    const int rowHeight = 36;
    const int headerHeight = 32;

    // Header
    painter.fillRect(0, 0, width(), headerHeight, QColor(40, 40, 40));
    painter.setPen(QColor(120, 120, 120));
    painter.setFont(QFont("Arial", 8));

    for (int frame = 1; frame <= m_project->totalFrames() && frame * cellWidth < width(); ++frame) {
        int x = (frame - 1) * cellWidth;
        if (frame % 5 == 0) {
            painter.setPen(QColor(180, 180, 180));
            painter.drawText(QRect(x, 0, cellWidth * 5, headerHeight), Qt::AlignCenter, QString::number(frame));
            painter.setPen(QColor(120, 120, 120));
        }
        painter.setPen(QColor(30, 30, 30));
        painter.drawLine(x, 0, x, height());
    }

    // Grid Content
    auto layers = m_project->layers();
    int currentFrame = m_project->currentFrame();

    for (int i = layers.size() - 1; i >= 0; --i) {
        Layer *layer = layers[i];
        int y = headerHeight + (layers.size() - 1 - i) * rowHeight;

        painter.fillRect(0, y, width(), rowHeight, QColor(30, 30, 30));

        // Onion Skin - show previous AND next frames
        if (m_onionSkinEnabled && layer == m_project->currentLayer()) {
            // Previous frames (greenish)
            for (int offset = 1; offset <= m_onionFrames; ++offset) {
                int prevFrame = currentFrame - offset;
                if (prevFrame >= 1 && layer->hasContentAtFrame(prevFrame)) {
                    int x = (prevFrame - 1) * cellWidth;
                    int alpha = 120 - (offset * 30);
                    painter.fillRect(x + 2, y + 6, cellWidth - 4, rowHeight - 12,
                                     QColor(100, 200, 100, alpha));
                }
            }

            // Next frames (reddish)
            for (int offset = 1; offset <= m_onionFrames; ++offset) {
                int nextFrame = currentFrame + offset;
                if (nextFrame <= m_project->totalFrames() && layer->hasContentAtFrame(nextFrame)) {
                    int x = (nextFrame - 1) * cellWidth;
                    int alpha = 120 - (offset * 30);
                    painter.fillRect(x + 2, y + 6, cellWidth - 4, rowHeight - 12,
                                     QColor(200, 100, 100, alpha));
                }
            }
        }

        // FIXED: Frames - only paint extended frames in correct range
        for (int frame = 1; frame <= m_project->totalFrames(); ++frame) {
            int x = (frame - 1) * cellWidth;
            QRect cellRect(x, y, cellWidth, rowHeight);

            // Highlight current frame
            if (frame == currentFrame) {
                painter.fillRect(cellRect, QColor(42, 130, 218, 20));
            }

            // Check frame type
            bool isKeyFrame = layer->isKeyFrame(frame);
            bool isExtended = layer->isFrameExtended(frame);

            if (isKeyFrame) {
                // This frame has actual content - draw in layer color
                QColor col = layer->color();
                painter.fillRect(cellRect.adjusted(2, 8, -2, -8), col);
                painter.setPen(col.lighter(120));
                painter.drawRect(cellRect.adjusted(2, 8, -2, -8));

                // If this key frame is extended, draw yellow triangle marker
                int extendEnd = layer->getExtensionEnd(frame);
                if (extendEnd > frame) {
                    QPolygon triangle;
                    triangle << QPoint(x + cellWidth - 2, y + 8)
                             << QPoint(x + cellWidth - 2, y + 13)
                             << QPoint(x + cellWidth - 7, y + 8);
                    painter.setBrush(QColor(255, 193, 7));
                    painter.setPen(Qt::NoPen);
                    painter.drawPolygon(triangle);
                }
            } else if (isExtended) {
                // CRITICAL FIX: Only paint yellow if this frame is actually extended
                int keyFrame = layer->getKeyFrameFor(frame);
                if (keyFrame != -1 && keyFrame != frame) {
                    QColor yellowExtended(255, 193, 7, 180);
                    painter.fillRect(cellRect.adjusted(2, 8, -2, -8), yellowExtended);
                    painter.setPen(yellowExtended.darker(110));
                    painter.drawRect(cellRect.adjusted(2, 8, -2, -8));

                    // Diagonal hatching pattern
                    painter.setPen(QPen(QColor(255, 193, 7, 100), 1));
                    for (int dx = 0; dx < cellWidth; dx += 4) {
                        painter.drawLine(x + dx, y + 8, x + dx + 8, y + rowHeight - 8);
                    }
                }
            }

            painter.setPen(QColor(25, 25, 25));
            painter.drawRect(cellRect);
        }
        painter.setPen(QColor(20, 20, 20));
        painter.drawLine(0, y + rowHeight, width(), y + rowHeight);
    }

    // Playhead
    int playheadX = (currentFrame - 1) * cellWidth + cellWidth / 2;
    painter.setPen(QPen(QColor(255, 60, 60), 2));
    painter.drawLine(playheadX, headerHeight, playheadX, height());

    QPolygon triangle;
    triangle << QPoint(playheadX, headerHeight)
             << QPoint(playheadX - 6, headerHeight - 10)
             << QPoint(playheadX + 6, headerHeight - 10);
    painter.setBrush(QColor(255, 60, 60));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(triangle);
}

void FrameGridWidget::mousePressEvent(QMouseEvent *event) {
    const int cellWidth = 16;
    int frame = (event->pos().x() / cellWidth) + 1;
    if (frame >= 1 && frame <= m_project->totalFrames()) {
        m_project->setCurrentFrame(frame);
        m_isDragging = true;
    }
}

void FrameGridWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging) {
        const int cellWidth = 16;
        int frame = (event->pos().x() / cellWidth) + 1;
        frame = qBound(1, frame, m_project->totalFrames());

        if (frame != m_project->currentFrame()) {
            m_project->setCurrentFrame(frame);
            update();
        }
    }
}

void FrameGridWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);

    const int cellWidth = 16;
    const int headerHeight = 32;
    const int rowHeight = 36;

    int clickedFrame = (event->pos().x() / cellWidth) + 1;
    int y = event->pos().y();

    if (y < headerHeight) {
        // Header - frame count menu
        QAction *add10 = menu.addAction("Add 10 Frames");
        QAction *add24 = menu.addAction("Add 24 Frames");
        QAction *selected = menu.exec(event->globalPos());
        if (selected) {
            if (selected == add10) m_project->setTotalFrames(m_project->totalFrames() + 10);
            else if (selected == add24) m_project->setTotalFrames(m_project->totalFrames() + 24);
            updateGeometry();
            update();
        }
        return;
    }

    // Calculate which layer was clicked
    auto layers = m_project->layers();
    int layerRow = (y - headerHeight) / rowHeight;
    int layerIndex = layers.size() - 1 - layerRow;
    if (layerIndex < 0 || layerIndex >= layers.size()) return;

    Layer *layer = layers[layerIndex];
    bool hasContent = layer->isKeyFrame(clickedFrame);
    bool isExtended = layer->isFrameExtended(clickedFrame);
    int keyFrame = layer->getKeyFrameFor(clickedFrame);

    if (hasContent) {
        // Key frame - offer to extend
        QAction *extendAction = menu.addAction("🎬 Extend Frame...");
        int currentExtension = layer->getExtensionEnd(clickedFrame);
        if (currentExtension > clickedFrame) {
            QAction *clearAction = menu.addAction(QString("✂️ Clear Extension (to frame %1)").arg(currentExtension));
            QAction *selected = menu.exec(event->globalPos());
            if (selected == extendAction) {
                bool ok;
                int toFrame = QInputDialog::getInt(this, "Extend Frame",
                                                   QString("Extend frame %1 to frame:").arg(clickedFrame),
                                                   currentExtension, clickedFrame + 1, m_project->totalFrames(), 1, &ok);
                if (ok) {
                    layer->extendFrameTo(clickedFrame, toFrame);
                    update();
                }
            } else if (selected == clearAction) {
                layer->clearFrameExtension(clickedFrame);
                update();
            }
        } else {
            QAction *selected = menu.exec(event->globalPos());
            if (selected == extendAction) {
                bool ok;
                int toFrame = QInputDialog::getInt(this, "Extend Frame",
                                                   QString("Extend frame %1 to frame:").arg(clickedFrame),
                                                   clickedFrame + 10, clickedFrame + 1, m_project->totalFrames(), 1, &ok);
                if (ok) {
                    layer->extendFrameTo(clickedFrame, toFrame);
                    update();
                }
            }
        }
    } else if (isExtended) {
        // Extended frame - show info
        QAction *gotoAction = menu.addAction(QString("📍 Go to Key Frame %1").arg(keyFrame));
        QAction *clearAction = menu.addAction("✂️ Clear Extension");
        QAction *selected = menu.exec(event->globalPos());
        if (selected == gotoAction) {
            m_project->setCurrentFrame(keyFrame);
        } else if (selected == clearAction) {
            layer->clearFrameExtension(keyFrame);
            update();
        }
    } else {
        // Empty frame
        QAction *add10 = menu.addAction("Add 10 Frames");
        QAction *add24 = menu.addAction("Add 24 Frames");
        QAction *selected = menu.exec(event->globalPos());
        if (selected) {
            if (selected == add10) m_project->setTotalFrames(m_project->totalFrames() + 10);
            else if (selected == add24) m_project->setTotalFrames(m_project->totalFrames() + 24);
            updateGeometry();
            update();
        }
    }
}

void FrameGridWidget::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    m_isDragging = false;
}

// --- TimelineWidget Implementation ---

TimelineWidget::TimelineWidget(Project *project, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_isPlaying(false)
    , m_playbackTimerId(-1)
{
    setupUI();
    connect(m_project, &Project::currentFrameChanged, this, &TimelineWidget::updateFrameDisplay);
}

void TimelineWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Control Bar
    QWidget *controlBar = new QWidget();
    controlBar->setStyleSheet("background-color: #282828; border-bottom: 1px solid #000;");
    controlBar->setFixedHeight(48);

    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(12, 6, 12, 6);

    auto createPlayButton = [](const QString &text, const QString &tooltip) {
        QPushButton *btn = new QPushButton(text);
        btn->setFixedSize(34, 34);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { background-color: #1e1e1e; border: none; border-radius: 4px; color: white; font-size: 14px; } QPushButton:hover { background-color: #2a82da; } QPushButton:pressed { background-color: #1e60a0; }");
        return btn;
    };

    QPushButton *firstBtn = createPlayButton("⏮", "First Frame");
    connect(firstBtn, &QPushButton::clicked, [this]() { m_project->setCurrentFrame(1); });

    QPushButton *prevBtn = createPlayButton("◀", "Previous Frame");
    connect(prevBtn, &QPushButton::clicked, [this]() {
        int prev = m_project->currentFrame() - 1;
        if (prev >= 1) m_project->setCurrentFrame(prev);
    });

    m_playPauseBtn = createPlayButton("▶", "Play/Pause (Space)");
    connect(m_playPauseBtn, &QPushButton::clicked, this, &TimelineWidget::onPlayPauseClicked);

    QPushButton *nextBtn = createPlayButton("▶", "Next Frame");
    connect(nextBtn, &QPushButton::clicked, [this]() {
        int next = m_project->currentFrame() + 1;
        if (next <= m_project->totalFrames()) m_project->setCurrentFrame(next);
    });

    QPushButton *lastBtn = createPlayButton("⏭", "Last Frame");
    connect(lastBtn, &QPushButton::clicked, [this]() { m_project->setCurrentFrame(m_project->totalFrames()); });

    m_stopBtn = createPlayButton("⏹", "Stop");
    connect(m_stopBtn, &QPushButton::clicked, this, &TimelineWidget::onStopClicked);

    controlLayout->addWidget(firstBtn);
    controlLayout->addWidget(prevBtn);
    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addWidget(nextBtn);
    controlLayout->addWidget(lastBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addSpacing(16);

    // Frame Label
    m_frameLabel = new QLabel("1 / 100");
    m_frameLabel->setStyleSheet("color: #2a82da; font-family: 'Courier New', monospace; font-size: 13px; font-weight: bold; background-color: #1e1e1e; padding: 6px 12px; border-radius: 4px;");
    m_frameLabel->setMinimumWidth(90);
    m_frameLabel->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(m_frameLabel);

    controlLayout->addSpacing(12);
    QLabel *fpsLabel = new QLabel(QString("@ %1 FPS").arg(m_project->fps()));
    fpsLabel->setStyleSheet("color: #888; font-size: 11px;");
    controlLayout->addWidget(fpsLabel);
    controlLayout->addStretch();

    // Onion Skin
    QPushButton *onionBtn = new QPushButton("◉ Onion Skin");
    onionBtn->setCheckable(true);
    onionBtn->setStyleSheet("QPushButton { background-color: #1e1e1e; border: none; border-radius: 4px; color: #aaa; padding: 6px 12px; font-size: 11px; } QPushButton:checked { background-color: #2a82da; color: white; }");

    connect(onionBtn, &QPushButton::toggled, this, [this](bool checked) {
        FrameGridWidget *grid = findChild<FrameGridWidget*>();
        if (grid) grid->setOnionSkin(checked, 2);
    });
    controlLayout->addWidget(onionBtn);
    mainLayout->addWidget(controlBar);

    // Splitter Area
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle { background-color: #000; width: 1px; }");

    LayerListWidget *layerList = new LayerListWidget(m_project);
    splitter->addWidget(layerList);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(false);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setStyleSheet("QScrollArea { background-color: #1a1a1a; border: none; } QScrollBar:horizontal { background: #1e1e1e; height: 10px; } QScrollBar::handle:horizontal { background: #3a3a3a; border-radius: 5px; }");

    FrameGridWidget *frameGrid = new FrameGridWidget(m_project);
    scrollArea->setWidget(frameGrid);

    splitter->addWidget(scrollArea);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    updateFrameDisplay();
}

void TimelineWidget::onPlayPauseClicked() {
    if (m_isPlaying) stopPlayback();
    else startPlayback();
}

void TimelineWidget::onStopClicked() {
    stopPlayback();
    m_project->setCurrentFrame(1);
}

void TimelineWidget::onFrameChanged(int frame) {
    m_project->setCurrentFrame(frame);
}

void TimelineWidget::updateFrameDisplay() {
    int current = m_project->currentFrame();
    int total = m_project->totalFrames();
    m_frameLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

void TimelineWidget::startPlayback() {
    m_isPlaying = true;
    m_playPauseBtn->setText("⏸");
    int interval = 1000 / m_project->fps();
    m_playbackTimerId = startTimer(interval);
}

void TimelineWidget::stopPlayback() {
    m_isPlaying = false;
    m_playPauseBtn->setText("▶");
    if (m_playbackTimerId != -1) {
        killTimer(m_playbackTimerId);
        m_playbackTimerId = -1;
    }
}

void TimelineWidget::timerEvent(QTimerEvent *event) {
    if (event->timerId() == m_playbackTimerId) {
        int nextFrame = m_project->currentFrame() + 1;
        if (nextFrame > m_project->totalFrames()) {
            stopPlayback();
            m_project->setCurrentFrame(1);
        } else {
            m_project->setCurrentFrame(nextFrame);
        }
    }
}

void TimelineWidget::setOnionSkinEnabled(bool enabled) {
    m_onionSkinEnabled = enabled;
    FrameGridWidget *grid = findChild<FrameGridWidget*>();
    if (grid) {
        grid->setOnionSkin(enabled, 3);
    }
}

