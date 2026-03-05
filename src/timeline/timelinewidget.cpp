// TODO:
// 1. implement proper frame extender logic
// 2. Add MiDi synth

#include "timelinewidget.h"
#include "core/project.h"
#include "core/layer.h"
#include "utils/thememanager.h"

#include <QEventLoop> // Added because github hates me
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QElapsedTimer>
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
#include <QProcess>
#include <QStandardPaths>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSvgRenderer>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// --- Internal Helper: LayerListWidget ---
class LayerListWidget : public QWidget {
public:
    LayerListWidget(Project *project, QWidget *parent = nullptr)
        : QWidget(parent), m_project(project) {
        setMinimumWidth(200);
        setMaximumWidth(300);
        // Use sizeHint() for preferred size — do NOT call setMinimumHeight()
        // since that forces the parent dock widget to expand unboundedly.
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        connect(project, &Project::layersChanged, this, [this]() {
            updateGeometry();
            update();
        });
    }

    QSize sizeHint() const override {
        const int rowHeight = 36;
        const int headerHeight = 32;
        return QSize(220, headerHeight + m_project->layerCount() * rowHeight);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), theme().bg1Color());

        // Header
        painter.fillRect(0, 0, width(), 32, theme().bg2Color());
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(rect().adjusted(12, 0, 0, -height() + 32), Qt::AlignLeft | Qt::AlignVCenter, "LAYERS");

        int y = 32;
        int rowHeight = 36;
        auto layers = m_project->layers();

        for (int i = layers.size() - 1; i >= 0; --i) {
            Layer *layer = layers[i];
            bool isCurrent = (m_project->currentLayer() == layer);

            QColor accent = theme().accentColor();
            QColor bgColor = isCurrent ? QColor(accent.red(), accent.green(), accent.blue(), 35) : theme().bg1Color();
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

            // Render visibility icon using SVG assets (white-tinted)
            {
                QString visRes = layer->isVisible() ? ":/icons/unhide.svg" : ":/icons/hide.svg";
                QSvgRenderer visRenderer(visRes);
                if (visRenderer.isValid()) {
                    QPixmap visPx(16, 16);
                    visPx.fill(Qt::transparent);
                    QPainter svgP(&visPx);
                    visRenderer.render(&svgP);
                    svgP.setCompositionMode(QPainter::CompositionMode_SourceIn);
                    QColor iconCol = layer->isVisible() ? QColor(200,200,200) : QColor(80,80,80);
                    svgP.fillRect(visPx.rect(), iconCol);
                    svgP.end();
                    painter.drawPixmap(width() - 62, y + (rowHeight - 16) / 2, visPx);
                }
            }

            // Render lock icon using SVG assets
            {
                QString lockRes = layer->isLocked() ? ":/icons/lock.svg" : ":/icons/unlock.svg";
                QSvgRenderer lockRenderer(lockRes);
                if (lockRenderer.isValid()) {
                    QPixmap lockPx(14, 14);
                    lockPx.fill(Qt::transparent);
                    QPainter svgP(&lockPx);
                    lockRenderer.render(&svgP);
                    svgP.setCompositionMode(QPainter::CompositionMode_SourceIn);
                    QColor lockCol = layer->isLocked() ? QColor(220, 160, 60) : QColor(60,60,60);
                    svgP.fillRect(lockPx.rect(), lockCol);
                    svgP.end();
                    painter.drawPixmap(width() - 34, y + (rowHeight - 14) / 2, lockPx);
                }
            }

            painter.setPen(theme().bg0Color());
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
    connect(project, &Project::layersChanged,  this, QOverload<>::of(&QWidget::update));
    connect(project, &Project::modified,       this, [this]() { updateGeometry(); adjustSize(); update(); });

    // Connect to each layer's modified signal so that frame extensions,
    // interpolations, and any other layer-level changes immediately repaint
    // and resize the scroll area (Layer::modified ≠ Project::modified).
    auto connectLayers = [this]() {
        for (Layer *layer : m_project->layers()) {
            // disconnect first to avoid double-connections on layersChanged
            disconnect(layer, &Layer::modified, this, nullptr);
            connect(layer, &Layer::modified, this, [this]() {
                updateGeometry();   // scroll area picks up new totalFrames()
                adjustSize();       // actually resize the widget (needed when widgetResizable=false)
                update();           // repaint the grid
            });
        }
    };
    connectLayers();
    // Re-run whenever the layer list changes (add/remove/reorder)
    connect(project, &Project::layersChanged, this, connectLayers);
}

QSize FrameGridWidget::sizeHint() const {
    const int cellWidth    = 16;
    const int rowHeight    = 36;
    const int headerHeight = 32;
    int h = headerHeight + m_project->layerCount() * rowHeight;
    int w = m_project->totalFrames() * cellWidth;
    return QSize(qMax(w, 400), qMax(h, headerHeight + rowHeight));
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
    painter.fillRect(rect(), theme().bg0Color());

    const int cellWidth = 16;
    const int rowHeight = 36;
    const int headerHeight = 32;

    // Header / ruler
    painter.fillRect(0, 0, width(), headerHeight, theme().bg2Color());

    for (int frame = 1; frame <= m_project->totalFrames() && (frame - 1) * cellWidth < width(); ++frame) {
        int x = (frame - 1) * cellWidth;

        // Vertical grid line through entire widget
        painter.setPen(theme().bg1Color());
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

        painter.fillRect(0, y, width(), rowHeight, theme().bg1Color());

        // Highlight active layer row
        if (layer == m_project->currentLayer()) {
            QColor acc = theme().accentColor();
            painter.fillRect(0, y, width(), rowHeight, QColor(acc.red(), acc.green(), acc.blue(), 20));
        }

        // === AUDIO LAYER RENDERING — multi-clip ===
        if (layer->layerType() == LayerType::Audio && layer->hasAudio()) {
            const auto &clips = layer->audioClips();
            // Stagger clip rows so overlapping clips are visible
            int clipRowH = qMax(8, (rowHeight - 8) / qMax(1, clips.size()));

            for (int ci = 0; ci < clips.size(); ++ci) {
                const AudioData &audio = clips[ci];

                // For -1 duration show an "infinite" bar (until end of timeline)
                int effDuration = audio.durationFrames > 0
                    ? audio.durationFrames
                    : (m_project->totalFrames() - audio.startFrame + 1);

                int startX = (audio.startFrame - 1) * cellWidth;
                int widthPx = qMax(cellWidth, effDuration * cellWidth);
                int clipY = y + 4 + ci * (clipRowH + 2);

                QRect audioRect(startX, clipY, widthPx, clipRowH);

                QColor clipColor = audio.isMidi
                    ? QColor(138, 43, 226)          // purple for MIDI
                    : (audio.muted ? QColor(90,90,90) : QColor(39, 174, 96));

                painter.setBrush(clipColor);
                painter.setPen(clipColor.darker(150));
                painter.drawRoundedRect(audioRect, 3, 3);

                // Waveform / MIDI indicator
                int centerY = clipY + clipRowH / 2;
                if (audio.isMidi && !audio.midiNotes.isEmpty()) {
                    // Piano-roll: horizontal bars, pitch → Y, time → X
                    // Find pitch range for this clip
                    int minPitch = 127, maxPitch = 0;
                    for (const MidiNote &n : audio.midiNotes) {
                        minPitch = qMin(minPitch, n.pitch);
                        maxPitch = qMax(maxPitch, n.pitch);
                    }
                    int pitchRange = qMax(1, maxPitch - minPitch);
                    double totalBeats = audio.midiTotalBeats > 0 ? audio.midiTotalBeats : 1.0;
                    int drawH = clipRowH - 6;
                    int drawY = clipY + 3;

                    for (const MidiNote &n : audio.midiNotes) {
                        // X position: beat → pixel within clip rect
                        int nx = startX + (int)((n.startBeat  / totalBeats) * widthPx);
                        int nw = qMax(2, (int)((n.durationBeat / totalBeats) * widthPx));
                        // Y position: higher pitch = higher on screen
                        int ny = drawY + drawH - 1 -
                                 (int)(((double)(n.pitch - minPitch) / pitchRange) * (drawH - 2));
                        int nh = qMax(1, drawH / qMax(1, pitchRange + 1));

                        // Color by channel
                        static const QColor chanColors[] = {
                            {220,180,255},{180,220,255},{255,220,180},{180,255,220},
                            {255,180,220},{220,255,180},{200,200,255},{255,200,200}
                        };
                        QColor nc = chanColors[n.channel % 8];
                        nc.setAlpha(160 + n.velocity);
                        painter.fillRect(nx, ny, nw, nh, nc);
                    }
                } else if (audio.isMidi) {
                    // No parsed notes yet — simple placeholder bars
                    painter.setPen(QColor(220, 180, 255, 120));
                    for (int wx = startX+4; wx < startX+widthPx-4 && wx < width(); wx += 10)
                        painter.fillRect(wx, clipY+3, 4, clipRowH-6, QColor(220,180,255,100));
                } else if (audio.waveformData.size() > 0) {
                    painter.setPen(QColor(20, 60, 20, 200));
                    int samplesPerPixel = qMax(1, audio.waveformData.size() / qMax(1, widthPx));
                    for (int px = 0; px < widthPx && startX + px < width(); px++) {
                        int sampleIdx = px * samplesPerPixel;
                        if (sampleIdx < audio.waveformData.size()) {
                            float amplitude = audio.waveformData[sampleIdx];
                            int waveHeight = qAbs(amplitude * (clipRowH / 2 - 2));
                            painter.drawLine(startX+px, centerY-waveHeight, startX+px, centerY+waveHeight);
                        }
                    }
                } else {
                    painter.setPen(QColor(20, 60, 20, 150));
                    for (int wx = startX; wx < startX + widthPx && wx < width(); wx += 2) {
                        int wh = 2 + (int)(std::sin(wx * 0.1) * std::cos(wx * 0.05) * (clipRowH/4));
                        painter.drawLine(wx, centerY - wh, wx, centerY + wh);
                    }
                }

                // Clip label
                painter.setPen(Qt::white);
                QFont lf; lf.setPointSize(6); painter.setFont(lf);
                QString label = QFileInfo(audio.filePath).baseName();
                if (audio.isMidi) label = "🎹 " + label;
                painter.drawText(audioRect.adjusted(4, 0, -2, 0),
                                 Qt::AlignVCenter | Qt::AlignLeft,
                                 painter.fontMetrics().elidedText(label, Qt::ElideRight, widthPx - 8));
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
                QColor acc = theme().accentColor();
                painter.fillRect(cellRect, QColor(acc.red(), acc.green(), acc.blue(), 20));
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
                    painter.setBrush(QColor(230, 120, 20));
                    painter.drawEllipse(x + extendWidth - 10, y + rowHeight / 2 - 3, 6, 6);

                    frame = extendEnd;
                    continue;
                }

                // === STANDARD KEYFRAME (Red/accent) or MOTION PATH (Purple) ===
                if (layer->isMotionPathFrame(frame)) {
                    // Motion-path generated frame — render purple
                    QColor motionPurple(139, 92, 246);
                    painter.setBrush(motionPurple);
                    painter.setPen(motionPurple.lighter(130));
                    painter.drawRoundedRect(cellRect.adjusted(2, 8, -2, -8), 4, 4);

                    painter.setBrush(Qt::white);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(x + cellWidth / 2 - 2, y + rowHeight / 2 - 2, 4, 4);
                } else {
                    // Standard keyframe
                    QColor keyRed = theme().accentColor();
                    painter.setBrush(keyRed);
                    painter.setPen(keyRed.lighter(130));
                    painter.drawRoundedRect(cellRect.adjusted(2, 8, -2, -8), 4, 4);

                    painter.setBrush(Qt::white);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(x + cellWidth / 2 - 2, y + rowHeight / 2 - 2, 4, 4);
                }
            }
        }
    }

    // === PLAYHEAD ===
    int playheadX = (currentFrame - 1) * cellWidth + cellWidth / 2;
    painter.setPen(QPen(theme().accentColor(), 2));
    painter.drawLine(playheadX, headerHeight, playheadX, height());

    painter.setBrush(theme().accentColor());
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
            menu.addAction("Add Audio Clip...");
            menu.addAction("Add MIDI File...");
            menu.addSeparator();

            // Per-clip actions
            const auto &clips = layer->audioClips();
            QList<QAction*> muteActions, removeActions;
            for (int i = 0; i < clips.size(); ++i) {
                const AudioData &c = clips[i];
                QString label = QFileInfo(c.filePath).fileName();
                if (c.isMidi) label = "🎹 " + label;
                menu.addSection(label);
                muteActions.append(menu.addAction(c.muted ? "  Unmute" : "  Mute"));
                removeActions.append(menu.addAction("  Remove"));
            }

            QAction *clearAll = nullptr;
            if (!clips.isEmpty()) {
                menu.addSeparator();
                clearAll = menu.addAction("Clear All Clips");
            }

            QAction *selected = menu.exec(event->globalPos());
            if (!selected) return;

            QString actText = selected->text().trimmed();

            if (actText == "Add Audio Clip...") {
                QString audioPath = QFileDialog::getOpenFileName(
                    this, "Load Audio File", QString(),
                    "Audio Files (*.mp3 *.wav *.ogg *.flac *.m4a *.aac)");
                if (!audioPath.isEmpty()) {
                    AudioData audio = loadAudioFile(audioPath, clickedFrame);
                    layer->addAudioClip(audio);
                    update();
                    emit audioLoaded(layer, audioPath);
                }
            } else if (actText == "Add MIDI File...") {
                QString midiPath = QFileDialog::getOpenFileName(
                    this, "Load MIDI File", QString(),
                    "MIDI Files (*.mid *.midi)");
                if (!midiPath.isEmpty()) {
                    // Use loadAudioFile so the MIDI gets parsed + rendered via FluidSynth
                    AudioData audio = loadAudioFile(midiPath, clickedFrame);
                    layer->addAudioClip(audio);
                    update();
                    emit audioLoaded(layer, midiPath);
                }
            } else if (selected == clearAll) {
                layer->clearAudio();
                update();
            } else {
                for (int i = 0; i < muteActions.size(); ++i) {
                    if (selected == muteActions[i]) {
                        AudioData c = layer->audioClips()[i];
                        c.muted = !c.muted;
                        layer->setAudioClip(i, c);
                        update();
                        break;
                    }
                }
                for (int i = 0; i < removeActions.size(); ++i) {
                    if (selected == removeActions[i]) {
                        layer->removeAudioClip(i);
                        update();
                        break;
                    }
                }
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

        QAction *dupFrameAction = menu.addAction("Duplicate Frame");
        dupFrameAction->setEnabled(isKeyFrame);

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
        } else if (selected == dupFrameAction) {
            int dest = clickedFrame + 1;
            if (dest > m_project->totalFrames())
                m_project->setTotalFrames(dest);
            for (Layer *lyr : m_project->layers()) {
                if (lyr->layerType() != LayerType::Audio &&
                    lyr->layerType() != LayerType::Reference)
                    lyr->duplicateFrame(clickedFrame, dest);
            }
            m_project->setCurrentFrame(dest);
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



// ── Minimal MIDI file parser ──────────────────────────────────────────────
// Parses SMF type 0/1 .mid files to extract note events for piano-roll display.
// No external library needed — MIDI is a well-defined public binary format.

static quint32 readVarLen(const QByteArray &data, int &pos)
{
    quint32 val = 0;
    for (int i = 0; i < 4 && pos < data.size(); ++i) {
        quint8 b = (quint8)data[pos++];
        val = (val << 7) | (b & 0x7F);
        if (!(b & 0x80)) break;
    }
    return val;
}

static quint16 readU16BE(const QByteArray &d, int p)
{
    return ((quint8)d[p] << 8) | (quint8)d[p+1];
}
static quint32 readU32BE(const QByteArray &d, int p)
{
    return ((quint8)d[p]<<24)|((quint8)d[p+1]<<16)|((quint8)d[p+2]<<8)|(quint8)d[p+3];
}

struct MidiParseResult {
    QVector<MidiNote> notes;
    double            totalBeats = 0;
};

static MidiParseResult parseMidiFile(const QString &path)
{
    MidiParseResult result;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return result;
    QByteArray data = f.readAll();
    f.close();

    if (data.size() < 14 || data.mid(0,4) != "MThd") return result;

    int format   = readU16BE(data, 8);
    int numTracks= readU16BE(data, 10);
    int division = readU16BE(data, 12);  // ticks per quarter note (or SMPTE)

    if (division & 0x8000) return result; // SMPTE timecode - not supported here

    int ticksPerBeat = division;
    double usPerBeat = 500000.0; // default 120 BPM

    int pos = 14;

    // First pass for tempo (track 0 in type 1), then note events
    // We collect tempo changes per tick for proper beat calculation
    struct TempoChange { quint32 tick; double usPerBeat; };
    QVector<TempoChange> tempos;
    tempos.append({0, 500000.0});

    struct NoteOn { int channel; quint32 tick; int velocity; };
    QMap<int, QVector<NoteOn>> activeNotes; // key = pitch*16+channel

    quint32 maxTick = 0;

    for (int tr = 0; tr < numTracks; ++tr) {
        if (pos + 8 > data.size()) break;
        if (data.mid(pos, 4) != "MTrk") { pos += 4; pos += readU32BE(data,pos)+4; continue; }
        quint32 trackLen = readU32BE(data, pos + 4);
        pos += 8;
        int trackEnd = pos + (int)trackLen;
        if (trackEnd > data.size()) trackEnd = data.size();

        quint32 tick = 0;
        quint8  runningStatus = 0;

        while (pos < trackEnd) {
            quint32 delta = readVarLen(data, pos);
            tick += delta;
            if (tick > maxTick) maxTick = tick;

            if (pos >= trackEnd) break;
            quint8 event = (quint8)data[pos];

            if (event == 0xFF) { // meta event
                pos++;
                if (pos >= trackEnd) break;
                quint8 metaType = (quint8)data[pos++];
                quint32 metaLen = readVarLen(data, pos);
                if (metaType == 0x51 && metaLen == 3 && pos+3 <= trackEnd) {
                    // Set Tempo
                    quint32 us = ((quint8)data[pos]<<16)|((quint8)data[pos+1]<<8)|(quint8)data[pos+2];
                    usPerBeat = us;
                    tempos.append({tick, (double)us});
                }
                pos += metaLen;
            } else if (event == 0xF0 || event == 0xF7) { // sysex
                pos++;
                quint32 slen = readVarLen(data, pos);
                pos += slen;
            } else {
                // MIDI channel event
                if (event & 0x80) { runningStatus = event; pos++; }
                quint8 status  = runningStatus;
                quint8 type    = (status >> 4) & 0x0F;
                quint8 channel = status & 0x0F;

                if (type == 0x9 || type == 0x8) { // note on / note off
                    if (pos + 1 >= trackEnd) break;
                    quint8 pitch = (quint8)data[pos++];
                    quint8 vel   = (quint8)data[pos++];
                    bool   isOn  = (type == 0x9 && vel > 0);
                    int    key   = pitch * 16 + channel;

                    if (isOn) {
                        activeNotes[key].append({channel, tick, vel});
                    } else {
                        auto &ons = activeNotes[key];
                        if (!ons.isEmpty()) {
                            NoteOn on = ons.takeFirst();
                            // Convert ticks to beats
                            double startBeat = (double)on.tick / ticksPerBeat;
                            double endBeat   = (double)tick    / ticksPerBeat;
                            MidiNote note;
                            note.pitch        = pitch;
                            note.startBeat    = startBeat;
                            note.durationBeat = qMax(0.05, endBeat - startBeat);
                            note.channel      = channel;
                            note.velocity     = on.velocity;
                            result.notes.append(note);
                        }
                    }
                } else if (type == 0xA || type == 0xB || type == 0xE) {
                    pos += 2; // 2 data bytes
                } else if (type == 0xC || type == 0xD) {
                    pos += 1; // 1 data byte
                } else {
                    // Unknown, skip
                    break;
                }
            }
        }
        pos = trackEnd;
    }

    result.totalBeats = (double)maxTick / ticksPerBeat;
    return result;
}

// ── MIDI → PCM rendering ─────────────────────────────────────────────────
// Renders a .mid/.midi file to a temporary WAV using FluidSynth (CLI).
// The soundfont path is read first from QSettings (user-configured in the
// Settings panel → Audio → MIDI Synthesizer), then falls back to the
// standard system locations so it works out-of-the-box on most distros.
static QString renderMidiToWav(const QString &midiPath)
{
    // Find fluidsynth executable
    QString fluidsynth = QStandardPaths::findExecutable("fluidsynth");

#ifdef Q_OS_WIN
    // On Windows, also check common install locations
    if (fluidsynth.isEmpty()) {
        const QStringList winPaths = {
            "C:/Program Files/FluidSynth/bin/fluidsynth.exe",
            "C:/Program Files (x86)/FluidSynth/bin/fluidsynth.exe",
            QDir::homePath() + "/fluidsynth/bin/fluidsynth.exe",
        };
        for (const QString &p : winPaths) {
            if (QFile::exists(p)) { fluidsynth = p; break; }
        }
    }
#endif

    if (fluidsynth.isEmpty()) {
        qWarning() << "MIDI render: fluidsynth not found in PATH";
        return QString();
    }

    // ── Resolve soundfont ─────────────────────────────────────────────────
    // 1. User-configured path (stored by settingspanel SF2 picker)
    QString sf2;
    {
        QSettings s("AkisVG", "AkisVG");
        QString userSf2 = s.value("midi/soundfont").toString();
        if (!userSf2.isEmpty() && QFile::exists(userSf2))
            sf2 = userSf2;
    }

    // 2. Common system locations (Arch, Debian/Ubuntu, Fedora, Windows)
    if (sf2.isEmpty()) {
        QStringList sfPaths = {
            "/usr/share/soundfonts/default.sf2",
            "/usr/share/soundfonts/FluidR3_GM.sf2",
            "/usr/share/soundfonts/GeneralUser-GS.sf2",
            "/usr/share/sounds/sf2/FluidR3_GM.sf2",
            "/usr/share/sounds/sf2/default-GM.sf2",
            "/usr/share/sounds/sf2/TimGM6mb.sf2",
            "/usr/share/sounds/sf2/SGM-v2.01-NicePianosGuitarsBass-V1.3.sf2",
            // Arch: fluidsynth usually ships FluidR3_GM via extra/soundfont-fluid
            "/usr/share/soundfonts/fluid-soundfont-gm.sf2",
        };
#ifdef Q_OS_WIN
        // Windows: check next to fluidsynth binary, common install locations
        QDir fluidsynthDir = QFileInfo(fluidsynth).absoluteDir();
        sfPaths.prepend(fluidsynthDir.filePath("../share/soundfonts/default.sf2"));
        sfPaths.prepend(fluidsynthDir.filePath("../share/soundfonts/FluidR3_GM.sf2"));
        sfPaths.prepend("C:/Program Files/FluidSynth/share/soundfonts/FluidR3_GM.sf2");
        sfPaths.prepend("C:/soundfonts/FluidR3_GM.sf2");
        sfPaths.prepend("C:/soundfonts/default.sf2");
        // Also check user's Documents folder
        QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        sfPaths.prepend(docs + "/soundfonts/FluidR3_GM.sf2");
        sfPaths.prepend(docs + "/FluidR3_GM.sf2");
#endif
        for (const QString &p : sfPaths) {
            if (QFile::exists(p)) { sf2 = p; break; }
        }
    }

    if (sf2.isEmpty()) {
        qWarning() << "MIDI render: no soundfont found — install soundfont-fluid "
                      "(Arch) or fluid-soundfont-gm (Debian) or set a custom path "
                      "in Settings → Audio → MIDI Soundfont";
        return QString();
    }

    QString outWav = QDir::tempPath() + "/" +
                     QFileInfo(midiPath).completeBaseName() + "_rendered.wav";

    // fluidsynth -ni -g 1.0 -F output.wav soundfont.sf2 input.mid
    QStringList args = { "-ni", "-g", "1.0", "-F", outWav, sf2, midiPath };
    QProcess proc;
#ifdef Q_OS_WIN
    // On Windows, hide the console window that fluidsynth spawns
    proc.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    proc.start(fluidsynth, args);
    if (!proc.waitForFinished(30000)) {
        proc.kill();
        qWarning() << "MIDI render: fluidsynth timed out";
        return QString();
    }
    if (proc.exitCode() != 0 || !QFile::exists(outWav)) {
        qWarning() << "MIDI render failed:" << proc.readAllStandardError();
        return QString();
    }

    qDebug() << "MIDI rendered to" << outWav << "using" << sf2;
    return outWav;
}

AudioData FrameGridWidget::loadAudioFile(const QString &filePath, int startFrame) {
    AudioData audio;
    audio.filePath   = filePath;
    audio.startFrame = startFrame;
    audio.volume     = 1.0f;
    audio.muted      = false;
    audio.isMidi     = filePath.endsWith(".mid",  Qt::CaseInsensitive) ||
                       filePath.endsWith(".midi", Qt::CaseInsensitive);

    // ── MIDI: parse notes for visualization + render to PCM ─────────────
    QString actualFilePath = filePath;
    if (audio.isMidi) {
        // 1. Parse note events for piano-roll display
        MidiParseResult parsed = parseMidiFile(filePath);
        audio.midiNotes     = parsed.notes;
        audio.midiTotalBeats = parsed.totalBeats;

        // 2. Render to WAV via FluidSynth for correct tempo + tones
        QString rendered = renderMidiToWav(filePath);
        if (!rendered.isEmpty()) {
            audio.renderedPath = rendered;
            actualFilePath     = rendered;
        }
        if (audio.renderedPath.isEmpty()) {
            // FluidSynth render failed — warn the user instead of falling back
            // to Qt's native MIDI (which produces beep-like sounds).
            qWarning() << "MIDI playback: FluidSynth rendering failed for" << filePath
                       << "-- install fluidsynth + a soundfont for proper audio.";
            // Use a sentinel so the player knows not to play raw MIDI
            // (we keep filePath set so the clip is shown in the timeline).
            audio.durationFrames = 0; // Will be computed below if possible
        }
    }

    // ── Step 1: get accurate duration via QMediaPlayer ────────────────────
    // QAudioDecoder::duration() is unreliable (often -1 or 0 until fully
    // decoded). QMediaPlayer reports duration correctly after LoadedMedia.
    {
        QMediaPlayer  probe;
        QAudioOutput  probeOut;   // required by Qt6 even if we never play
        probe.setAudioOutput(&probeOut);
        probe.setSource(QUrl::fromLocalFile(actualFilePath));

        QEventLoop probeLoop;
        qint64     durationMs = -1;

        // mediaStatusChanged fires LoadedMedia once metadata is ready
        QObject::connect(&probe, &QMediaPlayer::mediaStatusChanged,
            [&](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::LoadedMedia ||
                    status == QMediaPlayer::BufferedMedia) {
                    durationMs = probe.duration();
                    probeLoop.quit();
                } else if (status == QMediaPlayer::InvalidMedia ||
                           status == QMediaPlayer::NoMedia) {
                    probeLoop.quit();
                }
            });
        // Also catch durationChanged which fires even before LoadedMedia on
        // some backends (GStreamer sends it as soon as demux finishes)
        QObject::connect(&probe, &QMediaPlayer::durationChanged,
            [&](qint64 dur) {
                if (dur > 0) {
                    durationMs = dur;
                    probeLoop.quit();
                }
            });

        QTimer::singleShot(8000, &probeLoop, &QEventLoop::quit); // 8s safety
        probeLoop.exec();
        probe.stop();

        int fps = m_project->fps();
        if (fps <= 0) fps = 24;

        if (durationMs > 0) {
            // Correct formula: ms * fps / 1000, with a small ceil to avoid
            // clipping the last partial frame
            audio.durationFrames = static_cast<int>(
                (durationMs * fps + 999) / 1000);   // ceiling division
        } else {
            // Could not determine duration — store -1 so the clip renders
            // and plays to its natural end without being cut off
            audio.durationFrames = -1;
        }
    }

    // ── Step 2: build waveform via QAudioDecoder (non-blocking best-effort)
    // Skip for MIDI — no PCM to decode, draw piano-roll style instead
    if (!audio.isMidi) {
        QAudioDecoder decoder;
        decoder.setSource(QUrl::fromLocalFile(actualFilePath));

        QEventLoop waveLoop;
        bool       waveFinished = false;

        QObject::connect(&decoder, &QAudioDecoder::bufferReady, [&]() {
            if (waveFinished) return;
            QAudioBuffer buffer = decoder.read();
            if (!buffer.isValid()) return;

            int channelCount = qMax(1, buffer.format().channelCount());
            QAudioFormat::SampleFormat fmt = buffer.format().sampleFormat();

            if (fmt == QAudioFormat::Float) {
                const float *data = buffer.constData<float>();
                int fc = buffer.frameCount();
                for (int i = 0; i < fc; i += 50)
                    audio.waveformData.append(
                        qMin(qAbs(data[i * channelCount]), 1.0f));
            } else if (fmt == QAudioFormat::Int16) {
                const qint16 *data = buffer.constData<qint16>();
                int fc = buffer.frameCount();
                for (int i = 0; i < fc; i += 50)
                    audio.waveformData.append(
                        qMin(qAbs(data[i * channelCount] / 32768.0f), 1.0f));
            }
        });

        QObject::connect(&decoder, &QAudioDecoder::finished,
            [&]() { waveFinished = true; waveLoop.quit(); });
        QObject::connect(&decoder,
            QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            [&](QAudioDecoder::Error) { waveFinished = true; waveLoop.quit(); });

        decoder.start();
        QTimer::singleShot(6000, &waveLoop, &QEventLoop::quit);
        waveLoop.exec();
        decoder.stop();

        // If we still don't have a duration (decoder sometimes fills it in
        // after processing), grab it now as a fallback
        if (audio.durationFrames <= 0 && decoder.duration() > 0) {
            int fps = m_project->fps() > 0 ? m_project->fps() : 24;
            audio.durationFrames = static_cast<int>(
                (decoder.duration() * fps + 999) / 1000);
        }
    }

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
{
    // Audio players are created on-demand per layer/clip

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
      // Add a top margin so it doesn't collide with the MenuBar
      mainLayout->setContentsMargins(0, 5, 0, 0);
      mainLayout->setSpacing(0);

    // Control Bar
    m_controlBar = new QWidget();
    m_controlBar->setStyleSheet(
        QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(theme().bg1, theme().bg0));
    m_controlBar->setFixedHeight(56);

    QHBoxLayout *controlLayout = new QHBoxLayout(m_controlBar);
        // INCREASE the top margin here specifically for the "Playback" label
        controlLayout->setContentsMargins(12, 10, 12, 8);
        controlLayout->setSpacing(10);

    // Helper: load a Qt resource SVG and return a white-tinted QIcon.
    // All SVG assets are drawn in black; we invert to white so they're visible
    // on the dark button background.
    auto loadWhiteIcon = [](const QString &resourcePath) -> QIcon {
        QPixmap px(24, 24);
        px.fill(Qt::transparent);
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        QSvgRenderer renderer(resourcePath);
        if (renderer.isValid()) {
            renderer.render(&p);
            // Invert: make black pixels white, keep transparency
            p.setCompositionMode(QPainter::CompositionMode_SourceIn);
            p.fillRect(px.rect(), Qt::white);
        }
        p.end();
        return QIcon(px);
    };

    auto createPlayButton = [this, &loadWhiteIcon](const QString &fallbackText, const QString &tooltip, const QString &resourcePath = QString()) {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(34, 34);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        if (!resourcePath.isEmpty()) {
            QIcon ico = loadWhiteIcon(resourcePath);
            if (!ico.isNull()) {
                btn->setIcon(ico);
                btn->setIconSize(QSize(20, 20));
                // Do NOT set text — icon covers it; no unicode doppelganger
            } else {
                btn->setText(fallbackText);
            }
        } else {
            btn->setText(fallbackText);
        }
        { const auto &t = theme();
        btn->setStyleSheet(QString("QPushButton { background-color: %1; border: none; border-radius: 4px; color: white; font-size: 14px; }"
            "QPushButton:hover { background-color: %2; } QPushButton:pressed { background-color: %3; }")
            .arg(t.bg4, t.accent, t.accentHover)); }
        m_playButtons.append(btn);
        return btn;
    };

    // firstBtn: gotofirstlast flipped 180° to point left (go-to-first)
    QPushButton *firstBtn = [&]() {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(34, 34);
        btn->setToolTip("First Frame");
        btn->setCursor(Qt::PointingHandCursor);
        QSvgRenderer renderer(QString(":/icons/gotofirstlast.svg"));
        if (renderer.isValid()) {
            QPixmap px(24, 24);
            px.fill(Qt::transparent);
            QPainter p(&px);
            p.setRenderHint(QPainter::Antialiasing);
            p.translate(12, 12); p.rotate(180); p.translate(-12, -12);
            renderer.render(&p);
            p.end();
            QPainter tp(&px);
            tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tp.fillRect(px.rect(), Qt::white);
            tp.end();
            btn->setIcon(QIcon(px));
            btn->setIconSize(QSize(20, 20));
        } else {
            btn->setText("⏮");
        }
        { const auto &t = theme();
        btn->setStyleSheet(QString("QPushButton { background-color: %1; border: none; border-radius: 4px; color: white; font-size: 14px; }"
            "QPushButton:hover { background-color: %2; } QPushButton:pressed { background-color: %3; }")
            .arg(t.bg4, t.accent, t.accentHover)); }
        m_playButtons.append(btn);
        return btn;
    }();
    connect(firstBtn, &QPushButton::clicked, [this]() { m_project->setCurrentFrame(1); });

    // prevBtn: load nextframe.svg but flip 180° so it points left
    QPushButton *prevBtn = [&]() {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(34, 34);
        btn->setToolTip("Previous Frame");
        btn->setCursor(Qt::PointingHandCursor);
        QSvgRenderer renderer(QString(":/icons/nextframe.svg"));
        if (renderer.isValid()) {
            QPixmap px(24, 24);
            px.fill(Qt::transparent);
            QPainter p(&px);
            p.setRenderHint(QPainter::Antialiasing);
            // Rotate 180° around center
            p.translate(12, 12);
            p.rotate(180);
            p.translate(-12, -12);
            renderer.render(&p);
            p.end();
            // Tint white
            QPainter tp(&px);
            tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tp.fillRect(px.rect(), Qt::white);
            tp.end();
            btn->setIcon(QIcon(px));
            btn->setIconSize(QSize(20, 20));
        } else {
            btn->setText("◀");
        }
        { const auto &t = theme();
        btn->setStyleSheet(QString("QPushButton { background-color: %1; border: none; border-radius: 4px; color: white; font-size: 14px; }"
            "QPushButton:hover { background-color: %2; } QPushButton:pressed { background-color: %3; }")
            .arg(t.bg4, t.accent, t.accentHover)); }
        m_playButtons.append(btn);
        return btn;
    }();
    connect(prevBtn, &QPushButton::clicked, [this]() {
        int prev = m_project->currentFrame() - 1;
        if (prev >= 1) m_project->setCurrentFrame(prev);
    });

    m_playPauseBtn = createPlayButton("▶", "Play/Pause (Space)", ":/icons/play.svg");
    connect(m_playPauseBtn, &QPushButton::clicked, this, &TimelineWidget::onPlayPauseClicked);

    QPushButton *nextBtn = createPlayButton("▶", "Next Frame", ":/icons/nextframe.svg");
    connect(nextBtn, &QPushButton::clicked, [this]() {
        m_project->setCurrentFrame(m_project->currentFrame() + 1);
    });

    QPushButton *lastBtn = createPlayButton("⏭", "Last Frame", ":/icons/gotofirstlast.svg");
    connect(lastBtn, &QPushButton::clicked, [this]() { m_project->setCurrentFrame(m_project->highestUsedFrame()); });

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
    m_frameLabel = new QLabel("F 1");
    m_frameLabel->setStyleSheet(
        QString("color: %1; font-family: 'Courier New', monospace; font-size: 13px; font-weight: bold; background-color: %2; padding: 6px 12px; border-radius: 4px;")
        .arg(theme().accent, theme().bg4));
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

    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setFixedWidth(80);
    m_volumeSlider->setToolTip("Master Volume");
    { const auto &t = theme();
    m_volumeSlider->setStyleSheet(
        QString("QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
                "QSlider::handle:horizontal { background: %2; width: 12px; margin: -4px 0; border-radius: 6px; }"
                "QSlider::handle:horizontal:hover { background: %3; }"
                "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }")
        .arg(t.bg4, t.accent, t.accentHover)); }
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        float vol = value / 100.0f;
        for (auto &players : m_audioPlayers)
            for (auto &cp : players)
                cp.output->setVolume(vol);
    });
    controlLayout->addWidget(m_volumeSlider);
    controlLayout->addSpacing(8);

    // NO GIF EXPORT BUTTONS HERE - MOVED TO MENU BAR

    mainLayout->addWidget(m_controlBar);

    // ── Scrollable timeline body ──────────────────────────────────────────────
    // Both the layer list and frame grid must scroll VERTICALLY together when
    // there are many layers, and HORIZONTALLY independently for wide timelines.
    // Layout:  outerScroll (vertical) -> splitterContainer -> splitter
    //                                                   ├─ layerList
    //                                                   └─ hScrollArea (horizontal) -> frameGrid

    QScrollArea *outerScroll = new QScrollArea();
    outerScroll->setWidgetResizable(true);
    outerScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outerScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    { const auto &t = theme();
    outerScroll->setStyleSheet(
        QString("QScrollArea { background-color: %1; border: none; }"
                "QScrollBar:vertical { background: %1; width: 8px; }"
                "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: %3; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
        .arg(t.bg0, t.bg2, t.accent)); }

    // SplitterContainer: override sizeHint so the outer scroll area knows
    // the minimum height needed (based on layer count) and can scroll vertically.
    // Width is freely expandable — hScrollArea inside handles horizontal overflow.
    struct SplitterContainer : public QWidget {
        Project *proj;
        SplitterContainer(Project *p, QWidget *par=nullptr): QWidget(par), proj(p) {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
        QSize sizeHint() const override {
            const int rowHeight = 36, headerHeight = 32;
            return QSize(400, headerHeight + proj->layerCount() * rowHeight);
        }
    };
    SplitterContainer *splitterContainer = new SplitterContainer(m_project);
    connect(m_project, &Project::layersChanged, splitterContainer,
            [splitterContainer](){ splitterContainer->updateGeometry(); });
    QHBoxLayout *scLayout = new QHBoxLayout(splitterContainer);
    scLayout->setContentsMargins(0, 0, 0, 0);
    scLayout->setSpacing(0);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle { background-color: #000; width: 1px; }");
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    LayerListWidget *layerList = new LayerListWidget(m_project);
    splitter->addWidget(layerList);

    QScrollArea *hScrollArea = new QScrollArea();
    hScrollArea->setWidgetResizable(false);
    hScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    hScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    { const auto &t = theme();
    hScrollArea->setStyleSheet(
        QString("QScrollArea { background-color: %1; border: none; }"
                "QScrollBar:horizontal { background: %1; height: 10px; }"
                "QScrollBar::handle:horizontal { background: %2; border-radius: 5px; }"
                "QScrollBar::handle:horizontal:hover { background: %3; }")
        .arg(t.bg0, t.bg2, t.accent)); }

    FrameGridWidget *frameGrid = new FrameGridWidget(m_project);

    // Connect audio loading signal
    connect(frameGrid, &FrameGridWidget::audioLoaded, this, &TimelineWidget::loadAudioTrack);
    connect(frameGrid, &FrameGridWidget::referenceImageImported, this, &TimelineWidget::handleReferenceImport);

    hScrollArea->setWidget(frameGrid);
    frameGrid->adjustSize();

    splitter->addWidget(hScrollArea);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    scLayout->addWidget(splitter);
    outerScroll->setWidget(splitterContainer);

    mainLayout->addWidget(outerScroll, 1);

    updateFrameDisplay();

    // Connect to settings panel onion skin changes
    connect(m_project, &Project::onionSkinSettingsChanged, this, [this, frameGrid]() {
        frameGrid->setOnionSkin(
            m_project->onionSkinEnabled(),
            m_project->onionSkinBefore() + m_project->onionSkinAfter()
        );
    });
}

void TimelineWidget::ensurePlayersForLayer(Layer *layer) {
    if (!layer) return;
    const auto &clips = layer->audioClips();
    auto &players = m_audioPlayers[layer];

    // Add players for new clips
    for (int i = players.size(); i < clips.size(); ++i) {
        AudioClipPlayer cp;
        cp.output = new QAudioOutput(this);
        cp.player = new QMediaPlayer(this);
        cp.player->setAudioOutput(cp.output);
        cp.clipIdx = i;
        players.append(cp);
    }
    // Remove players for removed clips
    while (players.size() > clips.size()) {
        auto &cp = players.last();
        cp.player->stop();
        cp.player->deleteLater();
        cp.output->deleteLater();
        players.removeLast();
    }
    // Update sources + volume
    for (int i = 0; i < clips.size() && i < players.size(); ++i) {
        const AudioData &clip = clips[i];
        auto &cp = players[i];
        // For MIDI: only play the rendered WAV; never feed raw MIDI to QMediaPlayer
        // (raw MIDI via Qt's backend produces beep-like sounds).
        // For regular audio: play the file directly.
        QString playPath;
        if (clip.isMidi) {
            if (!clip.renderedPath.isEmpty() && QFile::exists(clip.renderedPath))
                playPath = clip.renderedPath;
            // else: no rendered audio available — leave source empty (silent)
        } else {
            playPath = clip.filePath;
        }

        QUrl url = playPath.isEmpty() ? QUrl() : QUrl::fromLocalFile(playPath);
        if (cp.player->source() != url)
            cp.player->setSource(url);
        cp.output->setVolume(clip.muted ? 0.0f : clip.volume);
    }
}

void TimelineWidget::releasePlayersForLayer(Layer *layer) {
    if (!m_audioPlayers.contains(layer)) return;
    for (auto &cp : m_audioPlayers[layer]) {
        cp.player->stop();
        cp.player->deleteLater();
        cp.output->deleteLater();
    }
    m_audioPlayers.remove(layer);
}

void TimelineWidget::releaseAllPlayers() {
    for (Layer *layer : m_audioPlayers.keys())
        releasePlayersForLayer(layer);
}

void TimelineWidget::loadAudioTracks() {
    // Rebuild players for ALL audio layers
    for (Layer *layer : m_project->layers()) {
        if (layer->layerType() == LayerType::Audio && layer->hasAudio())
            ensurePlayersForLayer(layer);
    }
}

void TimelineWidget::loadAudioTrack(Layer *layer, const QString &/*audioPath*/) {
    if (layer) ensurePlayersForLayer(layer);
}

void TimelineWidget::syncAudioToFrame() {
    int currentFrame = m_project->currentFrame();
    int fps = m_project->fps();
    qint64 frameDiffMs = 1000 / qMax(1, fps);

    // Ensure we have players for all audio layers
    for (Layer *layer : m_project->layers()) {
        if (layer->layerType() != LayerType::Audio || !layer->hasAudio()) continue;
        ensurePlayersForLayer(layer);

        const auto &clips  = layer->audioClips();
        auto       &players = m_audioPlayers[layer];

        for (int i = 0; i < clips.size() && i < players.size(); ++i) {
            const AudioData &clip = clips[i];
            auto &cp = players[i];

            // Effective duration: -1 means "use full audio length"
            int effDuration = clip.durationFrames > 0
                ? clip.durationFrames
                : (cp.player->duration() > 0
                    ? (int)((cp.player->duration() * fps) / 1000) + 1
                    : 99999);

            bool inRange = (currentFrame >= clip.startFrame &&
                            currentFrame < clip.startFrame + effDuration);

            cp.output->setVolume(clip.muted ? 0.0f : clip.volume);

            if (inRange) {
                int frameOffset = currentFrame - clip.startFrame;
                qint64 posMs = (static_cast<qint64>(frameOffset) * 1000) / qMax(1, fps);

                bool alreadyPlaying = (cp.player->playbackState() == QMediaPlayer::PlayingState);

                // Only seek if:
                //  - Not currently playing (scrubbing), OR
                //  - Drift is very large (> 500ms) which means we jumped to a new position
                // Do NOT seek every frame while playing — this causes stuttering.
                if (!alreadyPlaying || !m_isPlaying) {
                    // Scrubbing: always seek to the correct position
                    qint64 curPosMs = cp.player->position();
                    if (qAbs(curPosMs - posMs) > frameDiffMs)
                        cp.player->setPosition(posMs);
                } else {
                    // Playing: only correct large drifts (> 500ms) to avoid stutter
                    qint64 curPosMs = cp.player->position();
                    if (qAbs(curPosMs - posMs) > 500)
                        cp.player->setPosition(posMs);
                }

                if (m_isPlaying && !clip.muted) {
                    if (cp.player->playbackState() != QMediaPlayer::PlayingState)
                        cp.player->play();
                }
            } else {
                if (cp.player->playbackState() == QMediaPlayer::PlayingState)
                    cp.player->pause();
            }
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
    m_frameLabel->setText(QString("F %1").arg(m_project->currentFrame()));
}

void TimelineWidget::startPlayback() {
    m_isPlaying = true;
    // Switch to pause icon
    {
        QSvgRenderer r(QString(":/icons/pause.svg"));
        if (r.isValid()) {
            QPixmap px(24,24); px.fill(Qt::transparent);
            QPainter p(&px); r.render(&p); p.end();
            QPainter tp(&px);
            tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tp.fillRect(px.rect(), Qt::white); tp.end();
            m_playPauseBtn->setIcon(QIcon(px));
            m_playPauseBtn->setText("");
        } else {
            m_playPauseBtn->setIcon(QIcon());
            m_playPauseBtn->setText("⏸");
        }
    }

    // Record the wall-clock time and the frame we're starting from so
    // timerEvent can compute which frame *should* be showing right now
    // instead of blindly incrementing. This prevents timer drift from
    // causing audio stutter.
    m_playStartFrame = m_project->currentFrame();
    m_playElapsed.restart();

    // Fire the timer slightly faster than frame-rate so we never skip a frame
    // (at 24 fps one frame = 41.67 ms; fire every 10 ms and let timerEvent
    // decide whether it's actually time to advance).
    m_playbackTimerId = startTimer(10, Qt::PreciseTimer);

    loadAudioTracks();
    syncAudioToFrame();
}

void TimelineWidget::stopPlayback() {
    m_isPlaying = false;
    // Switch back to play icon
    {
        QSvgRenderer r(QString(":/icons/play.svg"));
        if (r.isValid()) {
            QPixmap px(24,24); px.fill(Qt::transparent);
            QPainter p(&px); r.render(&p); p.end();
            QPainter tp(&px);
            tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tp.fillRect(px.rect(), Qt::white); tp.end();
            m_playPauseBtn->setIcon(QIcon(px));
            m_playPauseBtn->setText("");
        } else {
            m_playPauseBtn->setIcon(QIcon());
            m_playPauseBtn->setText("▶");
        }
    }
    if (m_playbackTimerId != -1) {
        killTimer(m_playbackTimerId);
        m_playbackTimerId = -1;
    }
    // Pause all active audio players
    for (auto &players : m_audioPlayers)
        for (auto &cp : players)
            if (cp.player->playbackState() == QMediaPlayer::PlayingState)
                cp.player->pause();
}

void TimelineWidget::timerEvent(QTimerEvent *event) {
    if (event->timerId() != m_playbackTimerId)
        return;

    int fps = qMax(1, m_project->fps());

    // Calculate which frame should be shown now based on wall-clock time.
    // This makes visual frame advance perfectly in sync with audio position
    // regardless of Qt timer jitter.
    qint64 elapsedMs  = m_playElapsed.elapsed();
    int    frameOffset = static_cast<int>((elapsedMs * fps) / 1000);
    int    targetFrame = m_playStartFrame + frameOffset;

    int stopAt = m_project->highestUsedFrame();
    if (targetFrame > stopAt) {
        stopPlayback();
        m_project->setCurrentFrame(1);
        return;
    }

    // Only call setCurrentFrame (which is expensive — it triggers a full
    // refreshFrame) when the frame actually changes.
    if (targetFrame != m_project->currentFrame())
        m_project->setCurrentFrame(targetFrame);
}

void TimelineWidget::setOnionSkinEnabled(bool enabled) {
    FrameGridWidget *grid = findChild<FrameGridWidget*>();
    if (grid) {
        int frames = m_project->onionSkinBefore() + m_project->onionSkinAfter();
        grid->setOnionSkin(enabled, frames);
    }
}

void TimelineWidget::rerenderMidiClips()
{
    // For every MIDI clip across all audio layers: delete the old rendered WAV,
    // re-render with the now-current soundfont, rebuild the QMediaPlayer source.
    bool any = false;
    for (Layer *layer : m_project->layers()) {
        if (layer->layerType() != LayerType::Audio) continue;
        for (int i = 0; i < layer->audioClips().size(); ++i) {
            AudioData clip = layer->audioClips()[i];
            if (!clip.isMidi) continue;

            // Remove stale rendered WAV
            if (!clip.renderedPath.isEmpty() && QFile::exists(clip.renderedPath))
                QFile::remove(clip.renderedPath);
            clip.renderedPath.clear();

            // Re-render with new soundfont
            QString newWav = renderMidiToWav(clip.filePath);
            clip.renderedPath = newWav;
            layer->setAudioClip(i, clip);
            any = true;
        }
    }

    if (any) {
        // Rebuild all players so they pick up the new WAV sources
        releaseAllPlayers();
        loadAudioTracks();
    }
}

void TimelineWidget::applyTheme()
{
    const ThemeColors &t = theme();

    m_controlBar->setStyleSheet(
        QString("background-color: %1; border-bottom: 1px solid %2;").arg(t.bg1, t.bg0));

    QString btnStyle = QString(
        "QPushButton { background-color: %1; border: none; border-radius: 4px;"
        " color: white; font-size: 14px; }"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton:pressed { background-color: %3; }")
        .arg(t.bg4, t.accent, t.accentHover);

    for (QPushButton *btn : m_playButtons)
        btn->setStyleSheet(btnStyle);

    m_frameLabel->setStyleSheet(
        QString("color: %1; font-family: 'Courier New', monospace; font-size: 13px;"
                " font-weight: bold; background-color: %2; padding: 6px 12px; border-radius: 4px;")
        .arg(t.accent, t.bg4));

    m_volumeSlider->setStyleSheet(
        QString("QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
                "QSlider::handle:horizontal { background: %2; width: 12px; margin: -4px 0; border-radius: 6px; }"
                "QSlider::handle:horizontal:hover { background: %3; }"
                "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }")
        .arg(t.bg4, t.accent, t.accentHover));

    // Repaint the painted sub-widgets so QPainter colors update
    update();
}
