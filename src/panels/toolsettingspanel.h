#ifndef TOOLSETTINGSPANEL_H
#define TOOLSETTINGSPANEL_H

#include "tools/tool.h"
#include "core/layer.h"

#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QComboBox>
#include <QFontComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QGroupBox>
#include <QFormLayout>
#include <QToolButton>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QColorDialog>

class ToolSettingsPanel : public QFrame {
    Q_OBJECT
public:
    explicit ToolSettingsPanel(QWidget *parent = nullptr);
    void updateForTool(ToolType type, Tool *tool = nullptr);
    void showAudioLayerControls(Layer *layer);
    void showInterpolationControls(int totalFrames = 24);
    void hideInterpolationControls();
    void updateInterpolationNodes(int nodeCount); // called as user places nodes
    void applyTheme();

signals:
    // Basic mode: totalFrames per node-to-node segment, advanced=false, keyframeTimes empty
    // Advanced mode: advanced=true, keyframeTimes[i] = absolute frame for node i
    void interpolationSettingsChanged(int totalFrames, bool advanced, QList<int> keyframeTimes);

private:
    void clearContent();
    void buildPencilBrushControls(Tool *tool);
    void buildEraserControls(Tool *tool);
    void buildTextControls(Tool *tool);
    void buildGradientControls(Tool *tool);
    void buildSelectControls();
    void buildShapeControls(Tool *tool = nullptr);
    void buildLineControls(Tool *tool = nullptr);
    void buildEmptyMessage(const QString &msg);
    void buildAudioControls(Layer *layer);

    void buildInterpolationControls(int totalFrames);
    void buildInterpolationAdvancedControls(int nodeCount);

    QScrollArea  *m_scrollArea    = nullptr;
    QWidget      *m_contentWidget = nullptr;
    QVBoxLayout  *m_contentLayout = nullptr;
    QWidget      *m_headerWidget  = nullptr;
    QLabel       *m_titleLabel    = nullptr;
    bool          m_interpolating = false;
    int           m_interpTotalFrames = 24;
    int           m_interpNodeCount   = 0;
    QComboBox    *m_interpModeCombo   = nullptr;
    QGroupBox    *m_interpAdvGroup    = nullptr;
    QSpinBox     *m_interpFramesSpin  = nullptr;
    QList<QSpinBox*> m_interpNodeSpins;
    Layer    *m_audioLayer    = nullptr;
    ToolType  m_lastToolType  = ToolType::None;
    Tool     *m_lastTool      = nullptr;
};

#endif
