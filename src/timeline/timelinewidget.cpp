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
#include <QFileDialog>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDecoder>
#include <QSlider>
#include <QTimer>
#include <cmath>
#include <QEventLoop>
#include <QAudioBuffer>

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

            // Show layer type indicator
            QString layerText = layer->name();
            if (layer->layerType() == LayerType::Audio) {
                layerText = " " + layerText;
            } else if (layer->layerType() == LayerType::Reference) {
                layerText = " " + layerText;
            } else if (layer->layerType() == LayerType::Background) {
                layerText = " " + layerText;
            }

            painter.drawText(QRect(20, y, width() - 80, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, layerText);

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
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(20, 20, 20));

    const int cellWidth = 16;
    const int rowHeight = 36;
    const int headerHeight = 32;

    // Header / ruler
    painter.fillRect(0, 0, width(), headerHeight, QColor(40, 40, 40));

    for (int frame = 1; frame <= m_project->totalFrames() && (frame - 1) * cellWidth < width(); ++frame) {
        int x = (frame - 1) * cellWidth;

        // Vertical grid line through entire widget
        painter.setPen(QColor(30, 30, 30));
        painter.drawLine(x, 0, x, height());

        // Ruler ticks and labels
        if (frame % 5 == 0) {
            // Major tick + number: draw label starting at x so it's left-aligned to the frame
            painter.setPen(QColor(180, 180, 180));
            painter.setFont(QFont("Arial", 8));
            painter.drawText(QRect(x + 2, 0, cellWidth * 5, headerHeight - 4),
                             Qt::AlignLeft | Qt::AlignBottom, QString::number(frame));
            // Major tick mark
            painter.setPen(QPen(QColor(130, 130, 130), 1));
            painter.drawLine(x, headerHeight - 8, x, headerHeight);
        } else {
            // Minor tick mark
            painter.setPen(QPen(QColor(70, 70, 70), 1));
            painter.drawLine(x, headerHeight - 4, x, headerHeight);
        }
    }

    // Grid Content
    auto layers = m_project->layers();
    int currentFrame = m_project->currentFrame();

    for (int i = layers.size() - 1; i >= 0; --i) {
        Layer *layer = layers[i];
        int y = headerHeight + (layers.size() - 1 - i) * rowHeight;

        painter.fillRect(0, y, width(), rowHeight, QColor(30, 30, 30));

        // Highlight active layer row
        if (layer == m_project->currentLayer()) {
            painter.fillRect(0, y, width(), rowHeight, QColor(42, 130, 218, 20));
        }

        // === AUDIO LAYER RENDERING WITH REAL WAVEFORM ===
        if (layer->layerType() == LayerType::Audio && layer->hasAudio()) {
            AudioData audio = layer->getAudioData();

            int startX = (audio.startFrame - 1) * cellWidth;
            int widthPx = audio.durationFrames * cellWidth;

            QRect audioRect(startX, y + 4, widthPx, rowHeight - 8);
            QColor audioGreen(83, 138, 63);

            if (audio.muted) {
                audioGreen = QColor(100, 100, 100);
            }

            painter.setBrush(audioGreen);
            painter.setPen(audioGreen.darker(150));
            painter.drawRoundedRect(audioRect, 4, 4);

            // Draw waveform from audio data
            if (audio.waveformData.size() > 0) {
                painter.setPen(QColor(20, 60, 20, 180));
                int centerY = y + rowHeight / 2;
                int samplesPerPixel = qMax(1, audio.waveformData.size() / widthPx);

                for (int px = 0; px < widthPx && px < width(); px++) {
                    int sampleIdx = px * samplesPerPixel;
                    if (sampleIdx < audio.waveformData.size()) {
                        float amplitude = audio.waveformData[sampleIdx];
                        int waveHeight = qAbs(amplitude * (rowHeight / 2 - 4));
                        int wx = startX + px;
                        painter.drawLine(wx, centerY - waveHeight, wx, centerY + waveHeight);
                    }
                }
            } else {
                // Fallback to simulated waveform if no data
                painter.setPen(QColor(20, 60, 20, 150));
                int centerY = y + rowHeight / 2;
                for (int wx = startX; wx < startX + widthPx && wx < width(); wx += 2) {
                    int waveHeight = 2 + (std::sin(wx * 0.1) * std::cos(wx * 0.05) * 10);
                    painter.drawLine(wx, centerY - waveHeight, wx, centerY + waveHeight);
                }
            }

            // Volume indicator
            if (audio.volume < 0.99f) {
                painter.setPen(Qt::white);
                painter.setFont(QFont("Arial", 7));
                QString volText = QString("%1%").arg(qRound(audio.volume * 100));
                painter.drawText(audioRect, Qt::AlignCenter, volText);
            }

            continue;
        }

        // === ONION SKIN ===
        if (m_onionSkinEnabled && layer == m_project->currentLayer()) {
            for (int offset = 1; offset <= m_onionFrames; ++offset) {
                int prevFrame = currentFrame - offset;
                if (prevFrame >= 1 && layer->hasContentAtFrame(prevFrame)) {
                    int x = (prevFrame - 1) * cellWidth;
                    int alpha = 120 - (offset * 30);
                    painter.fillRect(x + 2, y + 6, cellWidth - 4, rowHeight - 12,
                                     QColor(100, 200, 100, alpha));
                }
            }

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

        // === FRAME RENDERING WITH INTERPOLATION AS CONTINUOUS BAR ===
        for (int frame = 1; frame <= m_project->totalFrames(); ++frame) {
            int x = (frame - 1) * cellWidth;
            QRect cellRect(x, y, cellWidth, rowHeight);

            // Highlight current frame
            if (frame == currentFrame) {
                painter.fillRect(cellRect, QColor(42, 130, 218, 20));
            }

            bool isKeyFrame = layer->isKeyFrame(frame);
            // isExtended / isInterpolated / isInterpKeyframe are checked
            // implicitly through getExtensionEnd() / getInterpolationFor() below

            if (isKeyFrame) {
                QColor col = layer->color();

                // Check for interpolation first
                FrameInterpolation interp = layer->getInterpolationFor(frame);
                if (interp.startFrame == frame && interp.endFrame > frame) {
                    // === INTERPOLATION BAR (Purple continuous bar) ===
                    int interpWidth = (interp.endFrame - frame + 1) * cellWidth;
                    QRect interpRect(x, y + 8, interpWidth, rowHeight - 16);

                    QColor purple(138, 43, 226);
                    painter.setBrush(purple);
                    painter.setPen(QPen(purple.darker(130), 1));
                    painter.drawRoundedRect(interpRect, 6, 6);

                    // Start keyframe dot
                    painter.setBrush(Qt::white);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(x + 4, y + rowHeight / 2 - 3, 6, 6);

                    // End keyframe dot
                    painter.drawEllipse(x + interpWidth - 10, y + rowHeight / 2 - 3, 6, 6);

                    // Skip ahead
                    frame = interp.endFrame;
                    continue;
                }

                // Check if this keyframe has extension
                int extendEnd = layer->getExtensionEnd(frame);
                if (extendEnd > frame) {
                    // === EXTENDED FRAME (Orange bar) ===
                    int extendWidth = (extendEnd - frame + 1) * cellWidth;
                    QRect extendRect(x, y + 8, extendWidth, rowHeight - 16);

                    QColor orange(255, 165, 0);
                    painter.setBrush(orange);
                    painter.setPen(QPen(orange.darker(130), 1));
                    painter.drawRoundedRect(extendRect, 6, 6);

                    // Start dot
                    painter.setBrush(Qt::white);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(x + 4, y + rowHeight / 2 - 3, 6, 6);

                    // End dot
                    painter.setBrush(QColor(50, 50, 200));
                    painter.drawEllipse(x + extendWidth - 10, y + rowHeight / 2 - 3, 6, 6);

                    frame = extendEnd;
                    continue;
                }

                // === STANDARD KEYFRAME (Blue) ===
                painter.setBrush(col);
                painter.setPen(col.lighter(120));
                painter.drawRoundedRect(cellRect.adjusted(2, 8, -2, -8), 4, 4);

                painter.setBrush(Qt::white);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(x + cellWidth / 2 - 2, y + rowHeight / 2 - 2, 4, 4);
            }
        }
    }

    // === PLAYHEAD ===
    int playheadX = (currentFrame - 1) * cellWidth + cellWidth / 2;
    painter.setPen(QPen(QColor(255, 60, 60), 2));
    painter.drawLine(playheadX, headerHeight, playheadX, height());

    painter.setBrush(QColor(255, 60, 60));
    QPolygon triangle;
    triangle << QPoint(playheadX, headerHeight)
             << QPoint(playheadX - 5, headerHeight - 8)
             << QPoint(playheadX + 5, headerHeight - 8);
    painter.drawPolygon(triangle);
}

void FrameGridWidget::mousePressEvent(QMouseEvent *event) {
    const int cellWidth = 16;

    int clickedFrame = (event->pos().x() / cellWidth) + 1;

    if (clickedFrame > 0 && clickedFrame <= m_project->totalFrames()) {
        m_project->setCurrentFrame(clickedFrame);
    }
}

void FrameGridWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        const int cellWidth = 16;
        int frame = (event->pos().x() / cellWidth) + 1;
        if (frame > 0 && frame <= m_project->totalFrames() && frame != m_project->currentFrame()) {
            m_project->setCurrentFrame(frame);
        }
    }
}

void FrameGridWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);

    const int cellWidth = 16;
    const int rowHeight = 36;
    const int headerHeight = 32;
    int clickedFrame = (event->pos().x() / cellWidth) + 1;
    int y = event->pos().y();

    if (y < headerHeight) {
        QAction *add10 = menu.addAction("Add 10 Frames");
        QAction *add24 = menu.addAction("Add 24 Frames");
        QAction *selected = menu.exec(event->globalPos());
        if (selected == add10) m_project->setTotalFrames(m_project->totalFrames() + 10);
        else if (selected == add24) m_project->setTotalFrames(m_project->totalFrames() + 24);
        updateGeometry();
        update();
        return;
    }

    auto layers = m_project->layers();
    int layerRow = (y - headerHeight) / rowHeight;
    int layerIndex = layers.size() - 1 - layerRow;

    if (layerIndex >= 0 && layerIndex < layers.size()) {
        Layer *layer = layers[layerIndex];

        // === AUDIO LAYER CONTEXT MENU ===
        if (layer->layerType() == LayerType::Audio) {
            QAction *loadAudio = menu.addAction("Load Audio File...");
            QAction *removeAudio = nullptr;
            if (layer->hasAudio()) {
                removeAudio = menu.addAction("Remove Audio");
                menu.addSeparator();
                QAction *muteToggle = menu.addAction(layer->getAudioData().muted ? "Unmute" : "Mute");

                QAction *selected = menu.exec(event->globalPos());

                if (selected == muteToggle) {
                    AudioData audio = layer->getAudioData();
                    audio.muted = !audio.muted;
                    layer->setAudioData(audio);
                    update();
                    return;
                }
            }

            QAction *selected = menu.exec(event->globalPos());

            if (selected == loadAudio) {
                QString audioPath = QFileDialog::getOpenFileName(
                    this, "Load Audio File", QString(),
                    "Audio Files (*.mp3 *.wav *.ogg *.flac *.m4a)");

                if (!audioPath.isEmpty()) {
                    AudioData audio = loadAudioFile(audioPath, clickedFrame);
                    layer->setAudioData(audio);
                    update();

                    // Emit signal to start playback
                    emit audioLoaded(layer, audioPath);
                }
            } else if (selected == removeAudio && removeAudio) {
                layer->clearAudio();
                update();
            }
            return;
        }

        // === REFERENCE LAYER - IMAGE IMPORT ===
        if (layer->layerType() == LayerType::Reference) {
            QAction *importImage = menu.addAction("Import Reference Image...");
            QAction *clearRef = nullptr;
            if (layer->hasContentAtFrame(clickedFrame)) {
                clearRef = menu.addAction("Clear Reference");
            }

            QAction *selected = menu.exec(event->globalPos());

            if (selected == importImage) {
                QString imagePath = QFileDialog::getOpenFileName(
                    this, "Import Reference Image", QString(),
                    "Images (*.png *.jpg *.jpeg *.svg)");

                if (!imagePath.isEmpty()) {
                    // Signal to import image to layer
                    emit referenceImageImported(layer, imagePath, clickedFrame);
                    update();
                }
            } else if (selected == clearRef && clearRef) {
                layer->clearFrame(clickedFrame);
                update();
            }
            return;
        }

        // === ART LAYER CONTEXT MENU ===
        bool isKeyFrame = layer->isKeyFrame(clickedFrame);

        QAction *makeKeyAction = menu.addAction("Make Keyframe");
        makeKeyAction->setEnabled(!isKeyFrame); // Only enable if NOT a keyframe

        menu.addSeparator();
        QAction *extendAction = menu.addAction("Extend Frame (Hold)");
        extendAction->setEnabled(isKeyFrame);

        QAction *interpAction = menu.addAction("Start Interpolation (Tween)");
        interpAction->setEnabled(isKeyFrame);

        if (isKeyFrame) {
            menu.addSeparator();
            QAction *clearAction = menu.addAction("Clear Frame");
            menu.addAction(clearAction);
        }

        QAction *selected = menu.exec(event->globalPos());

        if (selected == makeKeyAction) {
            layer->makeKeyFrame(clickedFrame);
            update();
        } else if (selected == extendAction) {
            bool ok;
            int toFrame = QInputDialog::getInt(
                this, "Extend Frame",
                QString("Extend frame %1 to frame:").arg(clickedFrame),
                clickedFrame + 10, clickedFrame + 1, m_project->totalFrames(), 1, &ok);
            if (ok) {
                layer->extendFrameTo(clickedFrame, toFrame);
                update();
            }
        } else if (selected == interpAction) {
            bool ok;
            int endFrame = QInputDialog::getInt(
                this, "Create Interpolation",
                QString("Interpolate from frame %1 to:").arg(clickedFrame),
                clickedFrame + 10, clickedFrame + 1, m_project->totalFrames(), 1, &ok);

            if (ok) {
                if (!layer->isKeyFrame(endFrame)) {
                    QMessageBox::warning(this, "Cannot Interpolate",
                        "End frame must be a keyframe.\n\nCreate the end pose first by:\n"
                        "1. Going to that frame\n2. Drawing the end position\n3. Then set interpolation.");
                    return;
                }

                QStringList easingTypes = {"linear", "easeIn", "easeOut", "easeInOut"};
                QString easing = QInputDialog::getItem(
                    this, "Easing Type", "Select easing function:",
                    easingTypes, 0, false, &ok);

                if (ok) {
                    layer->setInterpolation(clickedFrame, endFrame, easing);
                    update();

                    QMessageBox::information(this, "Interpolation Set",
                        "Interpolation created!\n\nNow use the SELECT TOOL (V) to:\n"
                        "1. Select objects with bounding box\n"
                        "2. Move them to desired positions on in-between frames\n"
                        "3. The system will auto-generate smooth motion");
                }
            }
        }
    }
}

AudioData FrameGridWidget::loadAudioFile(const QString &filePath, int startFrame) {
    AudioData audio;
    audio.filePath = filePath;
    audio.startFrame = startFrame;
    audio.volume = 1.0f;
    audio.muted = false;

    // Use QAudioDecoder to build waveform visualization data.
    // NOTE: The FFmpeg Qt backend delivers float32 samples, not int16.
    // We detect the format at runtime to avoid type-punning crashes.
    QAudioDecoder decoder;
    decoder.setSource(QUrl::fromLocalFile(filePath));

    QEventLoop loop;
    bool finished = false;

    connect(&decoder, &QAudioDecoder::bufferReady, [&]() {
        if (finished) return;           // guard against post-finish callbacks
        QAudioBuffer buffer = decoder.read();
        if (!buffer.isValid()) return;

        int channelCount = buffer.format().channelCount();
        if (channelCount < 1) channelCount = 1;

        QAudioFormat::SampleFormat fmt = buffer.format().sampleFormat();

        if (fmt == QAudioFormat::Float) {
            const float *data = buffer.constData<float>();
            int frameCount = buffer.frameCount();
            // Downsample: one sample per ~100 frames, take first channel
            for (int i = 0; i < frameCount; i += 50) {
                float val = qAbs(data[i * channelCount]);   // first channel, absolute
                audio.waveformData.append(qMin(val, 1.0f));
            }
        } else if (fmt == QAudioFormat::Int16) {
            const qint16 *data = buffer.constData<qint16>();
            int frameCount = buffer.frameCount();
            for (int i = 0; i < frameCount; i += 50) {
                float val = qAbs(data[i * channelCount] / 32768.0f);
                audio.waveformData.append(qMin(val, 1.0f));
            }
        }
        // Other formats: skip — waveform just won't display
    });

    connect(&decoder, &QAudioDecoder::finished, [&]() {
        finished = true;
        loop.quit();
    });

    // Error handler — use explicit overload cast to resolve ambiguity
    connect(&decoder,
            QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            [&](QAudioDecoder::Error) {
        finished = true;
        loop.quit();
    });

    decoder.start();

    // Safety timeout: don't block the UI forever
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    decoder.stop();

    // Calculate duration in frames using the decoder's reported duration
    qint64 durationMs = decoder.duration();
    if (durationMs <= 0) {
        // Fallback: estimate from waveform sample count
        // (50 frames downsampled at whatever sample rate — rough but better than 0)
        durationMs = 1000; // default 1 second if unknown
    }

    int fps = m_project->fps();
    audio.durationFrames = qMax(1, static_cast<int>((durationMs * fps) / 1000));

    return audio;
}

void FrameGridWidget::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    m_isDragging = false;
}

// === TimelineWidget Implementation ===

TimelineWidget::TimelineWidget(Project *project, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_isPlaying(false)
    , m_playbackTimerId(-1)
    , m_audioPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
{
    m_audioPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f); // Set a sensible default volume

    setupUI();
    connect(m_project, &Project::currentFrameChanged, this, &TimelineWidget::updateFrameDisplay);
    connect(m_project, &Project::currentFrameChanged, this, &TimelineWidget::syncAudioToFrame);
    // Update the FPS label whenever project settings change
    connect(m_project, &Project::modified, this, [this]() {
        m_fpsLabel->setText(QString("@ %1 FPS").arg(m_project->fps()));
    });
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
    m_fpsLabel = new QLabel(QString("@ %1 FPS").arg(m_project->fps()));
    m_fpsLabel->setStyleSheet("color: #888; font-size: 11px;");
    controlLayout->addWidget(m_fpsLabel);

    controlLayout->addStretch();

    // Audio volume control
    QLabel *volIcon = new QLabel("🔊");
    volIcon->setStyleSheet("color: #888; font-size: 13px;");
    controlLayout->addWidget(volIcon);

    QSlider *volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(80);
    volumeSlider->setFixedWidth(80);
    volumeSlider->setToolTip("Master Volume");
    volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #1e1e1e; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #2a82da; width: 12px; margin: -4px 0; border-radius: 6px; }"
        "QSlider::handle:horizontal:hover { background: #3a92ea; }"
        "QSlider::sub-page:horizontal { background: #2a82da; border-radius: 2px; }"
    );
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_audioOutput->setVolume(value / 100.0f);
    });
    controlLayout->addWidget(volumeSlider);
    controlLayout->addSpacing(8);

    // NO GIF EXPORT BUTTONS HERE - MOVED TO MENU BAR

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

    // Connect audio loading signal
    connect(frameGrid, &FrameGridWidget::audioLoaded, this, &TimelineWidget::loadAudioTrack);
    connect(frameGrid, &FrameGridWidget::referenceImageImported, this, &TimelineWidget::handleReferenceImport);

    scrollArea->setWidget(frameGrid);

    splitter->addWidget(scrollArea);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    updateFrameDisplay();

    // Connect to settings panel onion skin changes
    connect(m_project, &Project::onionSkinSettingsChanged, this, [this, frameGrid]() {
        frameGrid->setOnionSkin(
            m_project->onionSkinEnabled(),
            m_project->onionSkinBefore() + m_project->onionSkinAfter()
        );
    });
}

void TimelineWidget::loadAudioTrack(Layer *layer, const QString &audioPath) {
    m_audioPlayer->setSource(QUrl::fromLocalFile(audioPath));
    m_currentAudioLayer = layer;

    // Apply the layer's volume immediately
    if (layer && layer->hasAudio()) {
        AudioData audio = layer->getAudioData();
        m_audioOutput->setVolume(audio.muted ? 0.0f : audio.volume);
    }
}

void TimelineWidget::syncAudioToFrame() {
    if (!m_currentAudioLayer || !m_currentAudioLayer->hasAudio()) {
        // Try to find an audio layer dynamically
        for (Layer *layer : m_project->layers()) {
            if (layer->layerType() == LayerType::Audio && layer->hasAudio()) {
                AudioData audio = layer->getAudioData();
                m_audioPlayer->setSource(QUrl::fromLocalFile(audio.filePath));
                m_currentAudioLayer = layer;
                break;
            }
        }
        if (!m_currentAudioLayer)
            return;
    }

    AudioData audio = m_currentAudioLayer->getAudioData();

    // Apply volume from audio data
    m_audioOutput->setVolume(audio.muted ? 0.0f : audio.volume);

    int currentFrame = m_project->currentFrame();

    if (currentFrame >= audio.startFrame &&
        currentFrame < audio.startFrame + audio.durationFrames)
    {
        int frameOffset = currentFrame - audio.startFrame;
        qint64 positionMs = (static_cast<qint64>(frameOffset) * 1000) / m_project->fps();

        // Only seek if we're more than one frame off to avoid stuttering
        qint64 currentPosMs = m_audioPlayer->position();
        qint64 expectedMs   = positionMs;
        qint64 frameDiffMs  = 1000 / m_project->fps();

        if (qAbs(currentPosMs - expectedMs) > frameDiffMs * 2) {
            m_audioPlayer->setPosition(positionMs);
        }

        if (m_isPlaying && !audio.muted) {
            if (m_audioPlayer->playbackState() != QMediaPlayer::PlayingState) {
                m_audioPlayer->play();
            }
        }
    } else {
        if (m_audioPlayer->playbackState() == QMediaPlayer::PlayingState) {
            m_audioPlayer->pause();
        }
    }
}

void TimelineWidget::handleReferenceImport(Layer *layer, const QString &imagePath, int frame) {
    // Signal to canvas or project to load reference image
    emit referenceImageRequested(layer, imagePath, frame);
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

    // Auto-detect first audio layer if none loaded yet
    if (!m_currentAudioLayer) {
        for (Layer *layer : m_project->layers()) {
            if (layer->layerType() == LayerType::Audio && layer->hasAudio()) {
                AudioData audio = layer->getAudioData();
                m_audioPlayer->setSource(QUrl::fromLocalFile(audio.filePath));
                m_currentAudioLayer = layer;
                break;
            }
        }
    }

    // Kick off audio at correct position
    syncAudioToFrame();
}

void TimelineWidget::stopPlayback() {
    m_isPlaying = false;
    m_playPauseBtn->setText("▶");
    if (m_playbackTimerId != -1) {
        killTimer(m_playbackTimerId);
        m_playbackTimerId = -1;
    }
    m_audioPlayer->pause();
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
    FrameGridWidget *grid = findChild<FrameGridWidget*>();
    if (grid) {
        int frames = m_project->onionSkinBefore() + m_project->onionSkinAfter();
        grid->setOnionSkin(enabled, frames);
    }
}
