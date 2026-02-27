#include "preferencesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QSettings>
#include <QDialogButtonBox>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , m_onionSkinColor(100, 200, 100)
{
    setWindowTitle("Preferences");
    setMinimumWidth(500);
    setupUI();
    loadSettings();
}

void PreferencesDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Onion Skin Settings Group
    QGroupBox *onionGroup = new QGroupBox("Onion Skin Settings");
    QVBoxLayout *onionLayout = new QVBoxLayout(onionGroup);
    
    // Enable/Disable
    m_onionSkinEnabled = new QCheckBox("Enable Onion Skin");
    onionLayout->addWidget(m_onionSkinEnabled);
    
    // Number of frames
    QHBoxLayout *framesLayout = new QHBoxLayout();
    framesLayout->addWidget(new QLabel("Number of frames:"));
    m_onionSkinFrames = new QSpinBox();
    m_onionSkinFrames->setRange(1, 10);
    m_onionSkinFrames->setValue(2);
    framesLayout->addWidget(m_onionSkinFrames);
    framesLayout->addStretch();
    onionLayout->addLayout(framesLayout);
    
    // Opacity
    QHBoxLayout *opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("Opacity:"));
    m_onionSkinOpacity = new QSpinBox();
    m_onionSkinOpacity->setRange(10, 100);
    m_onionSkinOpacity->setValue(70);
    m_onionSkinOpacity->setSuffix("%");
    opacityLayout->addWidget(m_onionSkinOpacity);
    opacityLayout->addStretch();
    onionLayout->addLayout(opacityLayout);
    
    // Color
    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("Color:"));
    m_onionSkinColorBtn = new QPushButton();
    m_onionSkinColorBtn->setFixedSize(50, 25);
    m_onionSkinColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #888;")
        .arg(m_onionSkinColor.name()));
    connect(m_onionSkinColorBtn, &QPushButton::clicked, [this]() {
        QColor newColor = QColorDialog::getColor(m_onionSkinColor, this, "Choose Onion Skin Color");
        if (newColor.isValid()) {
            m_onionSkinColor = newColor;
            m_onionSkinColorBtn->setStyleSheet(
                QString("background-color: %1; border: 1px solid #888;")
                .arg(m_onionSkinColor.name()));
        }
    });
    colorLayout->addWidget(m_onionSkinColorBtn);
    colorLayout->addStretch();
    onionLayout->addLayout(colorLayout);
    
    mainLayout->addWidget(onionGroup);
    mainLayout->addStretch();
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        emit settingsChanged();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void PreferencesDialog::loadSettings()
{
    QSettings settings("Lumina", "LuminaStudio");
    m_onionSkinEnabled->setChecked(settings.value("onionSkin/enabled", false).toBool());
    m_onionSkinFrames->setValue(settings.value("onionSkin/frames", 2).toInt());
    m_onionSkinOpacity->setValue(settings.value("onionSkin/opacity", 70).toInt());
    m_onionSkinColor = settings.value("onionSkin/color", QColor(100, 200, 100)).value<QColor>();
    m_onionSkinColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #888;")
        .arg(m_onionSkinColor.name()));
}

void PreferencesDialog::saveSettings()
{
    QSettings settings("Lumina", "LuminaStudio");
    settings.setValue("onionSkin/enabled", m_onionSkinEnabled->isChecked());
    settings.setValue("onionSkin/frames", m_onionSkinFrames->value());
    settings.setValue("onionSkin/opacity", m_onionSkinOpacity->value());
    settings.setValue("onionSkin/color", m_onionSkinColor);
}

bool PreferencesDialog::onionSkinEnabled() const
{
    return m_onionSkinEnabled->isChecked();
}

int PreferencesDialog::onionSkinFrames() const
{
    return m_onionSkinFrames->value();
}

QColor PreferencesDialog::onionSkinColor() const
{
    return m_onionSkinColor;
}

int PreferencesDialog::onionSkinOpacity() const
{
    return m_onionSkinOpacity->value();
}
