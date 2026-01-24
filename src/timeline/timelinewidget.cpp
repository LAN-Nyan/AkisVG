#include "timelinewidget.h"
#include "core/project.h"
#include "core/layer.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTimerEvent>
#include <QSplitter>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QMouseEvent>

// Custom widget for layer list
class LayerListWidget : public QWidget {
public:
    LayerListWidget(Project *project, QWidget *parent = nullptr)
        : QWidget(parent), m_project(project) {
        setMinimumWidth(200);
        setMaximumWidth(300);
        updateLayers();

        connect(project, &Project::layersChanged, this, &LayerListWidget::updateLayers);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), QColor(45, 45, 45));

        // Header
        painter.fillRect(0, 0, width(), 30, QColor(58, 58, 58));
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(rect().adjusted(10, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "LAYERS");

        // Draw layers
        int y = 30;
        int rowHeight = 40;

        auto layers = m_project->layers();
        for (int i = layers.size() - 1; i >= 0; --i) {
            Layer *layer = layers[i];
            bool isCurrent = (m_project->currentLayer() == layer);

            // Background
            QColor bgColor = isCurrent ? QColor(42, 130, 218, 50) : QColor(45, 45, 45);
            painter.fillRect(0, y, width(), rowHeight, bgColor);

            // Border
            painter.setPen(QColor(0, 0, 0));
            painter.drawLine(0, y + rowHeight, width(), y + rowHeight);

            // Layer color indicator
            painter.fillRect(5, y + 10, 6, rowHeight - 20, layer->color());

            // Layer name
            painter.setPen(layer->isVisible() ? Qt::white : QColor(100, 100, 100));
            painter.setFont(QFont("Arial", 11));
            painter.drawText(QRect(20, y, width() - 60, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, layer->name());

            // Visibility icon
            QString visIcon = layer->isVisible() ? "👁" : "👁‍🗨";
            painter.drawText(QRect(width() - 50, y, 20, rowHeight),
                             Qt::AlignCenter, visIcon);

            // Lock icon
            if (layer->isLocked()) {
                painter.drawText(QRect(width() - 25, y, 20, rowHeight),
                                 Qt::AlignCenter, "🔒");
            }

            y += rowHeight;
        }
    }

    void mousePressEvent(QMouseEvent *event) override {
        int y = event->pos().y();
        if (y < 30) return; // Header

        int rowHeight = 40;
        int index = (y - 30) / rowHeight;
        int layerIndex = m_project->layers().size() - 1 - index;

        if (layerIndex >= 0 && layerIndex < m_project->layers().size()) {
            // Check if clicked on visibility icon
            if (event->pos().x() > width() - 50 && event->pos().x() < width() - 30) {
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
    void updateLayers() {
        setMinimumHeight(30 + m_project->layerCount() * 40 + 50);
        update();
    }

    Project *m_project;
};

// Custom widget for frame grid
class FrameGridWidget : public QWidget {
public:
    FrameGridWidget(Project *project, QWidget *parent = nullptr)
        : QWidget(parent), m_project(project) {
        setMinimumHeight(200);

        connect(project, &Project::currentFrameChanged, this, QOverload<>::of(&QWidget::update));
        connect(project, &Project::layersChanged, this, QOverload<>::of(&QWidget::update));
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), QColor(26, 26, 26));

        const int cellWidth = 20;
        const int rowHeight = 40;
        const int headerHeight = 30;

        // Draw frame numbers
        painter.fillRect(0, 0, width(), headerHeight, QColor(58, 58, 58));
        painter.setPen(QColor(150, 150, 150));
        painter.setFont(QFont("Arial", 9));

        for (int frame = 1; frame <= m_project->totalFrames() && frame * cellWidth < width(); ++frame) {
            int x = (frame - 1) * cellWidth;

            if (frame % 5 == 0) {
                painter.setPen(QColor(200, 200, 200));
                painter.drawText(QRect(x, 0, cellWidth * 5, headerHeight),
                                 Qt::AlignCenter, QString::number(frame));
                painter.setPen(QColor(150, 150, 150));
            }

            // Frame divider
            painter.setPen(QColor(40, 40, 40));
            painter.drawLine(x, headerHeight, x, height());
        }

        // Draw layers
        auto layers = m_project->layers();
        for (int i = layers.size() - 1; i >= 0; --i) {
            Layer *layer = layers[i];
            int y = headerHeight + (layers.size() - 1 - i) * rowHeight;

            // Row background
            painter.fillRect(0, y, width(), rowHeight, QColor(45, 45, 45));

            // Draw cells with content
            for (int frame = 1; frame <= m_project->totalFrames(); ++frame) {
                int x = (frame - 1) * cellWidth;
                QRect cellRect(x, y, cellWidth, rowHeight);

                // Highlight current frame
                if (frame == m_project->currentFrame()) {
                    painter.fillRect(cellRect, QColor(42, 130, 218, 30));
                }

                // Draw content indicator
                if (layer->hasContentAtFrame(frame)) {
                    painter.fillRect(cellRect.adjusted(3, 8, -3, -8), layer->color());
                }

                // Cell border
                painter.setPen(QColor(40, 40, 40));
                painter.drawRect(cellRect);
            }

            // Row divider
            painter.setPen(QColor(0, 0, 0));
            painter.drawLine(0, y + rowHeight, width(), y + rowHeight);
        }

        // Draw playhead
        int playheadX = (m_project->currentFrame() - 1) * cellWidth + cellWidth / 2;
        painter.setPen(QPen(QColor(255, 0, 0), 2));
        painter.drawLine(playheadX, 0, playheadX, height());

        // Playhead triangle
        QPolygon triangle;
        triangle << QPoint(playheadX, 0)
                 << QPoint(playheadX - 6, 10)
                 << QPoint(playheadX + 6, 10);
        painter.setBrush(QColor(255, 0, 0));
        painter.drawPolygon(triangle);
    }

    void mousePressEvent(QMouseEvent *event) override {
        const int cellWidth = 20;
        const int headerHeight = 30;

        if (event->pos().y() < headerHeight) {
            // Clicked on header - seek to frame
            int frame = (event->pos().x() / cellWidth) + 1;
            if (frame >= 1 && frame <= m_project->totalFrames()) {
                m_project->setCurrentFrame(frame);
            }
        }
    }

    QSize sizeHint() const override {
        return QSize(m_project->totalFrames() * 20, 200);
    }

private:
    Project *m_project;
};

TimelineWidget::TimelineWidget(Project *project, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_isPlaying(false)
    , m_playbackTimerId(-1)
{
    setupUI();

    connect(m_project, &Project::currentFrameChanged,
            this, &TimelineWidget::updateFrameDisplay);
}

void TimelineWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Control bar
    QWidget *controlBar = new QWidget();
    controlBar->setStyleSheet("background-color: #3a3a3a; border-bottom: 1px solid #000;");
    controlBar->setFixedHeight(50);

    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(12, 8, 12, 8);

    // Playback buttons
    auto createPlayButton = [](const QString &text, const QString &tooltip) {
        QPushButton *btn = new QPushButton(text);
        btn->setFixedSize(36, 36);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: #2d2d2d;"
            "   border: 2px solid #555;"
            "   border-radius: 6px;"
            "   color: white;"
            "   font-size: 16px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: #3a3a3a;"
            "   border-color: #666;"
            "}"
            "QPushButton:pressed {"
            "   background-color: #2a82da;"
            "}"
            );
        return btn;
    };

    QPushButton *firstBtn = createPlayButton("⏮", "First Frame");
    connect(firstBtn, &QPushButton::clicked, this, [this]() {
        m_project->setCurrentFrame(1);
    });

    QPushButton *prevBtn = createPlayButton("◀", "Previous Frame");
    connect(prevBtn, &QPushButton::clicked, this, [this]() {
        int prev = m_project->currentFrame() - 1;
        if (prev >= 1) m_project->setCurrentFrame(prev);
    });

    m_playPauseBtn = createPlayButton("▶", "Play/Pause (Space)");
    connect(m_playPauseBtn, &QPushButton::clicked, this, &TimelineWidget::onPlayPauseClicked);

    QPushButton *nextBtn = createPlayButton("▶", "Next Frame");
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        int next = m_project->currentFrame() + 1;
        if (next <= m_project->totalFrames()) m_project->setCurrentFrame(next);
    });

    QPushButton *lastBtn = createPlayButton("⏭", "Last Frame");
    connect(lastBtn, &QPushButton::clicked, this, [this]() {
        m_project->setCurrentFrame(m_project->totalFrames());
    });

    m_stopBtn = createPlayButton("⏹", "Stop");
    connect(m_stopBtn, &QPushButton::clicked, this, &TimelineWidget::onStopClicked);

    controlLayout->addWidget(firstBtn);
    controlLayout->addWidget(prevBtn);
    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addWidget(nextBtn);
    controlLayout->addWidget(lastBtn);
    controlLayout->addWidget(m_stopBtn);

    controlLayout->addSpacing(16);

    // Frame counter
    m_frameLabel = new QLabel("1 / 100");
    m_frameLabel->setStyleSheet(
        "color: #2a82da; "
        "font-family: 'Courier New', monospace; "
        "font-size: 14px; "
        "font-weight: bold; "
        "background-color: #2d2d2d; "
        "padding: 6px 12px; "
        "border: 2px solid #555; "
        "border-radius: 6px;"
        );
    m_frameLabel->setMinimumWidth(100);
    m_frameLabel->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(m_frameLabel);

    controlLayout->addSpacing(16);

    // FPS label
    QLabel *fpsLabel = new QLabel(QString("@ %1 FPS").arg(m_project->fps()));
    fpsLabel->setStyleSheet("color: #888; font-size: 11px;");
    controlLayout->addWidget(fpsLabel);

    controlLayout->addStretch();

    // Add frames button
    QPushButton *addFramesBtn = new QPushButton("+ Add 50 Frames");
    addFramesBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #2a82da;"
        "   border: none;"
        "   border-radius: 6px;"
        "   color: white;"
        "   padding: 8px 16px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a92ea;"
        "}"
        );
    connect(addFramesBtn, &QPushButton::clicked, this, [this]() {
        m_project->setTotalFrames(m_project->totalFrames() + 50);
        updateFrameDisplay();
    });
    controlLayout->addWidget(addFramesBtn);

    mainLayout->addWidget(controlBar);

    // Timeline content
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle { background-color: #000; width: 2px; }");

    // Layer list
    LayerListWidget *layerList = new LayerListWidget(m_project);
    splitter->addWidget(layerList);

    // Frame grid (scrollable)
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(false);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(
        "QScrollArea { background-color: #1a1a1a; border: none; }"
        "QScrollBar:horizontal { background: #2d2d2d; height: 12px; }"
        "QScrollBar::handle:horizontal { background: #555; border-radius: 6px; min-width: 20px; }"
        "QScrollBar::handle:horizontal:hover { background: #666; }"
        );

    FrameGridWidget *frameGrid = new FrameGridWidget(m_project);
    scrollArea->setWidget(frameGrid);

    splitter->addWidget(scrollArea);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    updateFrameDisplay();
}

void TimelineWidget::onPlayPauseClicked()
{
    if (m_isPlaying) {
        stopPlayback();
    } else {
        startPlayback();
    }
}

void TimelineWidget::onStopClicked()
{
    stopPlayback();
    m_project->setCurrentFrame(1);
}

void TimelineWidget::onFrameChanged(int frame)
{
    m_project->setCurrentFrame(frame);
}

void TimelineWidget::updateFrameDisplay()
{
    int current = m_project->currentFrame();
    int total = m_project->totalFrames();

    m_frameLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

void TimelineWidget::startPlayback()
{
    m_isPlaying = true;
    m_playPauseBtn->setText("⏸");
    m_playPauseBtn->setStyleSheet(m_playPauseBtn->styleSheet() + "QPushButton { background-color: #2a82da; }");

    int interval = 1000 / m_project->fps();
    m_playbackTimerId = startTimer(interval);
}

void TimelineWidget::stopPlayback()
{
    m_isPlaying = false;
    m_playPauseBtn->setText("▶");
    m_playPauseBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #2d2d2d;"
        "   border: 2px solid #555;"
        "   border-radius: 6px;"
        "   color: white;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a3a3a;"
        "   border-color: #666;"
        "}"
        );

    if (m_playbackTimerId != -1) {
        killTimer(m_playbackTimerId);
        m_playbackTimerId = -1;
    }
}

void TimelineWidget::timerEvent(QTimerEvent *event)
{
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
