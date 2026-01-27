#ifndef TOOLSETTINGSPANEL_H
#define TOOLSETTINGSPANEL_H

#include "tools/tool.h"  // ADD THIS LINE - defines ToolType

#include <QFrame>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>

class ToolSettingsPanel : public QFrame {
    Q_OBJECT
public:
    explicit ToolSettingsPanel(QWidget *parent = nullptr);
    void updateForTool(ToolType type);

private:
    QVBoxLayout *m_layout;
    QComboBox *m_textureCombo;
    QLabel *m_previewLabel;
};

#endif
