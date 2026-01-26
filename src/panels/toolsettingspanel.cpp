#include "toolsettingspanel.h"
#include <QPainter>  // ADD THIS
#include <QPen>      // ADD THIS

ToolSettingsPanel::ToolSettingsPanel(QWidget *parent) : QFrame(parent) {
    // Fixed class name from ToolSettingsPanel to match header
    m_layout = new QVBoxLayout(this);
    m_textureCombo = new QComboBox(this);
    m_previewLabel = new QLabel(this);

    m_layout->addWidget(new QLabel("Texture:", this));
    m_layout->addWidget(m_textureCombo);
    m_layout->addWidget(m_previewLabel);

    setStyleSheet("background-color: #2d2d2d; color: white;");
}

void ToolSettingsPanel::updateForTool(ToolType type) {
    // Show/hide texture options based on tool
    bool showTexture = (type == ToolType::Pencil ||
                        type == ToolType::Brush);
    m_textureCombo->setVisible(showTexture);
}
