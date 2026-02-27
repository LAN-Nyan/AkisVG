#ifndef TOOLSETTINGSPANEL_H
#define TOOLSETTINGSPANEL_H

#include "tools/tool.h"

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

class ToolSettingsPanel : public QFrame {
    Q_OBJECT
public:
    explicit ToolSettingsPanel(QWidget *parent = nullptr);
    void updateForTool(ToolType type, Tool *tool = nullptr);

private:
    void clearContent();
    void buildPencilBrushControls(Tool *tool);
    void buildEraserControls(Tool *tool);
    void buildTextControls();
    void buildSelectControls();
    void buildShapeControls();
    void buildLineControls();
    void buildEmptyMessage(const QString &msg);

    QScrollArea  *m_scrollArea    = nullptr;
    QWidget      *m_contentWidget = nullptr;
    QVBoxLayout  *m_contentLayout = nullptr;
};

#endif
