#include "preferencesdialog.h"
#include "utils/hotkeymanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QSettings>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QScrollArea>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , m_onionSkinColor(100, 200, 100)
{
    setWindowTitle(tr("Preferences"));
    setMinimumWidth(520);
    setupUI();
    loadSettings();
}

void PreferencesDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget();

    // ── General / Onion skin ─────────────────────────────────────────────────
    auto *generalPage = new QWidget();
    auto *generalLayout = new QVBoxLayout(generalPage);
    auto *onionGroup = new QGroupBox(tr("Onion Skin"));
    auto *onionLayout = new QVBoxLayout(onionGroup);

    m_onionSkinEnabled = new QCheckBox(tr("Enable Onion Skin"));
    onionLayout->addWidget(m_onionSkinEnabled);

    auto *framesLayout = new QHBoxLayout();
    framesLayout->addWidget(new QLabel(tr("Frames:")));
    m_onionSkinFrames = new QSpinBox();
    m_onionSkinFrames->setRange(1, 10);
    m_onionSkinFrames->setValue(2);
    framesLayout->addWidget(m_onionSkinFrames);
    framesLayout->addStretch();
    onionLayout->addLayout(framesLayout);

    auto *opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel(tr("Opacity:")));
    m_onionSkinOpacity = new QSpinBox();
    m_onionSkinOpacity->setRange(10, 100);
    m_onionSkinOpacity->setValue(70);
    m_onionSkinOpacity->setSuffix("%");
    opacityLayout->addWidget(m_onionSkinOpacity);
    opacityLayout->addStretch();
    onionLayout->addLayout(opacityLayout);

    auto *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel(tr("Color:")));
    m_onionSkinColorBtn = new QPushButton();
    m_onionSkinColorBtn->setFixedSize(50, 25);
    connect(m_onionSkinColorBtn, &QPushButton::clicked, this, [this]() {
        const QColor c = QColorDialog::getColor(m_onionSkinColor, this, tr("Onion Skin Color"));
        if (c.isValid()) {
            m_onionSkinColor = c;
            m_onionSkinColorBtn->setStyleSheet(
                QString("background-color: %1; border: 1px solid #888;").arg(m_onionSkinColor.name()));
        }
    });
    colorLayout->addWidget(m_onionSkinColorBtn);
    colorLayout->addStretch();
    onionLayout->addLayout(colorLayout);

    generalLayout->addWidget(onionGroup);
    generalLayout->addStretch();
    tabs->addTab(generalPage, tr("General"));

    // ── Hotkeys ──────────────────────────────────────────────────────────────
    auto *hotkeyPage = new QWidget();
    auto *hotkeyOuter = new QVBoxLayout(hotkeyPage);
    auto *hotkeyScroll = new QScrollArea();
    hotkeyScroll->setWidgetResizable(true);
    auto *hotkeyContent = new QWidget();
    auto *hotkeyLayout = new QVBoxLayout(hotkeyContent);

    QLabel *hint = new QLabel(tr("Assign single-key shortcuts for tools (no Ctrl/Alt modifiers)."));
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #888;");
    hotkeyLayout->addWidget(hint);

    HotkeyManager &hk = HotkeyManager::instance();
    for (ToolType type : hk.configurableTools()) {
        auto *row = new QHBoxLayout();
        row->addWidget(new QLabel(hk.toolDisplayName(type)), 1);
        auto *edit = new QKeySequenceEdit();
        edit->setKeySequence(hk.shortcutFor(type));
        edit->setMaximumSequenceLength(1);
        m_hotkeyEdits.insert(type, edit);
        row->addWidget(edit);
        hotkeyLayout->addLayout(row);
    }

    auto *resetBtn = new QPushButton(tr("Reset Hotkeys to Defaults"));
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        HotkeyManager::instance().resetToDefaults();
        for (auto it = m_hotkeyEdits.cbegin(); it != m_hotkeyEdits.cend(); ++it)
            it.value()->setKeySequence(HotkeyManager::instance().shortcutFor(it.key()));
    });
    hotkeyLayout->addWidget(resetBtn);
    hotkeyLayout->addStretch();
    hotkeyScroll->setWidget(hotkeyContent);
    hotkeyOuter->addWidget(hotkeyScroll);
    tabs->addTab(hotkeyPage, tr("Hotkeys"));

    mainLayout->addWidget(tabs);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
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
    QSettings settings("AkisVG", "AkisVG");
    m_onionSkinEnabled->setChecked(settings.value("onionSkin/enabled", false).toBool());
    m_onionSkinFrames->setValue(settings.value("onionSkin/frames", 2).toInt());
    m_onionSkinOpacity->setValue(settings.value("onionSkin/opacity", 70).toInt());
    m_onionSkinColor = settings.value("onionSkin/color", QColor(100, 200, 100)).value<QColor>();
    m_onionSkinColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #888;").arg(m_onionSkinColor.name()));

    HotkeyManager::instance().load();
    for (auto it = m_hotkeyEdits.cbegin(); it != m_hotkeyEdits.cend(); ++it)
        it.value()->setKeySequence(HotkeyManager::instance().shortcutFor(it.key()));
}

void PreferencesDialog::saveSettings()
{
    QSettings settings("AkisVG", "AkisVG");
    settings.setValue("onionSkin/enabled", m_onionSkinEnabled->isChecked());
    settings.setValue("onionSkin/frames", m_onionSkinFrames->value());
    settings.setValue("onionSkin/opacity", m_onionSkinOpacity->value());
    settings.setValue("onionSkin/color", m_onionSkinColor);

    HotkeyManager &hk = HotkeyManager::instance();
    for (auto it = m_hotkeyEdits.cbegin(); it != m_hotkeyEdits.cend(); ++it)
        hk.setShortcut(it.key(), it.value()->keySequence());
}

bool PreferencesDialog::onionSkinEnabled() const { return m_onionSkinEnabled->isChecked(); }
int PreferencesDialog::onionSkinFrames() const { return m_onionSkinFrames->value(); }
QColor PreferencesDialog::onionSkinColor() const { return m_onionSkinColor; }
int PreferencesDialog::onionSkinOpacity() const { return m_onionSkinOpacity->value(); }
