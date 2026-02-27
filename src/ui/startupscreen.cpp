#include "startupscreen.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QPushButton>
#include <QFrame>

// ──────────────────────────────────────────────────────────
static const QString kRecentFilesKey = "recentFiles";
static const int     kMaxRecent      = 10;

// Shared dark-panel stylesheet helpers
static QString groupBoxStyle()
{
    return
        "QGroupBox {"
        "  color: #ccc;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "  letter-spacing: 0.5px;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 6px;"
        "  margin-top: 10px;"
        "  padding-top: 6px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 0 8px;"
        "}";
}

static QString inputStyle()
{
    return
        "QSpinBox, QComboBox, QLineEdit {"
        "  background-color: #1e1e1e;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  border-radius: 4px;"
        "  padding: 5px 8px;"
        "  min-height: 24px;"
        "}"
        "QSpinBox:hover, QComboBox:hover, QLineEdit:hover {"
        "  border-color: #2a82da;"
        "}"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView {"
        "  background: #2d2d2d; color: white; selection-background-color: #2a82da;"
        "}";
}

static QPushButton* makeButton(const QString &text, const QString &accent = "#2a82da")
{
    QPushButton *btn = new QPushButton(text);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(40);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  padding: 0 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1a62ba;"
        "}"
    ).arg(accent, accent == "#2a82da" ? "#3a92ea" : "#555"));
    return btn;
}

// ──────────────────────────────────────────────────────────
StartupScreen::StartupScreen(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("AkisVG – Welcome");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(860, 580);
    setStyleSheet("background-color: #1e1e1e; color: white;");

    setupUI();
    loadRecentFiles();
}

// ──────────────────────────────────────────────────────────
void StartupScreen::setupUI()
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── LEFT BANNER ───────────────────────────────────────
    QWidget *banner = new QWidget();
    banner->setFixedWidth(260);
    banner->setStyleSheet("background-color: #141414;");

    QVBoxLayout *bannerLayout = new QVBoxLayout(banner);
    bannerLayout->setContentsMargins(24, 32, 24, 24);
    bannerLayout->setSpacing(8);

    // Logo / title area
    QLabel *logo = new QLabel("Akis\nVG");
    logo->setStyleSheet(
        "font-size: 52px;"
        "font-weight: 900;"
        "color: #2a82da;"
        "line-height: 1;"
        "letter-spacing: -2px;");
    logo->setAlignment(Qt::AlignLeft);
    bannerLayout->addWidget(logo);

    QLabel *subtitle = new QLabel("Vector Animation Studio");
    subtitle->setStyleSheet("color: #666; font-size: 11px; letter-spacing: 1px;");
    bannerLayout->addWidget(subtitle);

    bannerLayout->addSpacing(32);

    // Divider
    QFrame *div = new QFrame();
    div->setFrameShape(QFrame::HLine);
    div->setStyleSheet("color: #2a2a2a;");
    bannerLayout->addWidget(div);

    bannerLayout->addSpacing(16);

    // Version
    QLabel *ver = new QLabel(QString("Version %1").arg(QApplication::applicationVersion()));
    ver->setStyleSheet("color: #444; font-size: 10px;");
    bannerLayout->addWidget(ver);

    bannerLayout->addStretch();

    // Open existing
    m_openBtn = makeButton("Open Project…", "#3a3a3a");
    m_openBtn->setMinimumHeight(44);
    connect(m_openBtn, &QPushButton::clicked, this, &StartupScreen::onOpenClicked);
    bannerLayout->addWidget(m_openBtn);

    root->addWidget(banner);

    // ── RIGHT CONTENT ──────────────────────────────────────
    QWidget *content = new QWidget();
    content->setStyleSheet("background-color: #1e1e1e;");

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(28, 28, 28, 24);
    contentLayout->setSpacing(20);

    // ── NEW PROJECT GROUP ─────────────────────────────────
    QGroupBox *newGroup = new QGroupBox("New Project");
    newGroup->setStyleSheet(groupBoxStyle());

    QVBoxLayout *newGroupLayout = new QVBoxLayout(newGroup);
    newGroupLayout->setSpacing(10);

    // Project name
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setLabelAlignment(Qt::AlignRight);

    m_projectNameEdit = new QLineEdit("Untitled");
    m_projectNameEdit->setStyleSheet(inputStyle());
    formLayout->addRow("Name:", m_projectNameEdit);

    // Preset selector
    m_presetCombo = new QComboBox();
    m_presetCombo->setStyleSheet(inputStyle());
    m_presetCombo->addItem("Custom",         QSize(0,0));
    m_presetCombo->addItem("HD  (1280×720)", QSize(1280,720));
    m_presetCombo->addItem("Full HD  (1920×1080)", QSize(1920,1080));
    m_presetCombo->addItem("4K  (3840×2160)", QSize(3840,2160));
    m_presetCombo->addItem("Square  (1080×1080)", QSize(1080,1080));
    m_presetCombo->addItem("Portrait  (1080×1920)", QSize(1080,1920));
    m_presetCombo->setCurrentIndex(2); // default Full HD
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StartupScreen::onPresetSelected);
    formLayout->addRow("Preset:", m_presetCombo);

    // Width / height
    QWidget *resPair = new QWidget();
    QHBoxLayout *resLayout = new QHBoxLayout(resPair);
    resLayout->setContentsMargins(0,0,0,0);
    resLayout->setSpacing(8);

    m_widthSpin = new QSpinBox();
    m_widthSpin->setRange(64, 7680);
    m_widthSpin->setValue(1920);
    m_widthSpin->setSuffix(" px");
    m_widthSpin->setStyleSheet(inputStyle());

    QLabel *xLbl = new QLabel("×");
    xLbl->setStyleSheet("color: #666;");

    m_heightSpin = new QSpinBox();
    m_heightSpin->setRange(64, 4320);
    m_heightSpin->setValue(1080);
    m_heightSpin->setSuffix(" px");
    m_heightSpin->setStyleSheet(inputStyle());

    resLayout->addWidget(m_widthSpin);
    resLayout->addWidget(xLbl);
    resLayout->addWidget(m_heightSpin);
    resLayout->addStretch();
    formLayout->addRow("Size:", resPair);

    // FPS
    m_fpsCombо = new QComboBox();
    m_fpsCombо->setStyleSheet(inputStyle());
    m_fpsCombо->addItem("12 FPS", 12);
    m_fpsCombо->addItem("24 FPS", 24);
    m_fpsCombо->addItem("30 FPS", 30);
    m_fpsCombо->addItem("60 FPS", 60);
    m_fpsCombо->setCurrentIndex(1);
    formLayout->addRow("Frame Rate:", m_fpsCombо);

    newGroupLayout->addLayout(formLayout);

    m_createBtn = makeButton("✦  Create Project");
    m_createBtn->setMinimumHeight(44);
    connect(m_createBtn, &QPushButton::clicked, this, &StartupScreen::onCreateClicked);
    newGroupLayout->addWidget(m_createBtn);

    contentLayout->addWidget(newGroup);

    // ── RECENT FILES GROUP ────────────────────────────────
    QGroupBox *recentGroup = new QGroupBox("Recent Projects");
    recentGroup->setStyleSheet(groupBoxStyle());

    QVBoxLayout *recentLayout = new QVBoxLayout(recentGroup);
    recentLayout->setContentsMargins(8, 8, 8, 8);
    recentLayout->setSpacing(6);

    m_recentList = new QListWidget();
    m_recentList->setStyleSheet(
        "QListWidget {"
        "  background-color: #161616;"
        "  border: 1px solid #2a2a2a;"
        "  border-radius: 4px;"
        "  color: white;"
        "  font-size: 12px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-bottom: 1px solid #222;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: rgba(42,130,218,0.3);"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #2a2a2a;"
        "}"
    );
    m_recentList->setMaximumHeight(140);
    connect(m_recentList, &QListWidget::itemDoubleClicked,
            this, &StartupScreen::onRecentItemDoubleClicked);
    recentLayout->addWidget(m_recentList);

    m_openRecentBtn = makeButton("Open Selected", "#3a3a3a");
    m_openRecentBtn->setMinimumHeight(36);
    m_openRecentBtn->setEnabled(false);
    connect(m_openRecentBtn, &QPushButton::clicked, this, &StartupScreen::onOpenRecentClicked);
    connect(m_recentList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_openRecentBtn->setEnabled(!m_recentList->selectedItems().isEmpty());
    });
    recentLayout->addWidget(m_openRecentBtn);

    contentLayout->addWidget(recentGroup);
    contentLayout->addStretch();

    root->addWidget(content, 1);

    // Trigger preset to set default dims
    onPresetSelected(m_presetCombo->currentIndex());
}

// ──────────────────────────────────────────────────────────
void StartupScreen::loadRecentFiles()
{
    QSettings settings;
    QStringList files = settings.value(kRecentFilesKey).toStringList();

    m_recentList->clear();
    for (const QString &f : files) {
        QFileInfo fi(f);
        if (!fi.exists()) continue;

        QListWidgetItem *item = new QListWidgetItem();
        item->setText(fi.fileName());
        item->setToolTip(f);
        item->setData(Qt::UserRole, f);
        item->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
        m_recentList->addItem(item);
    }
}

void StartupScreen::saveRecentFiles()
{
    // The caller (main.cpp / MainWindow) is responsible for saving recent files.
    // This is a helper provided here for convenience.
}

// ──────────────────────────────────────────────────────────
void StartupScreen::onPresetSelected(int index)
{
    QSize sz = m_presetCombo->itemData(index).toSize();
    if (sz.width() > 0) {
        m_widthSpin->setValue(sz.width());
        m_heightSpin->setValue(sz.height());
    }
}

void StartupScreen::onCreateClicked()
{
    m_newWidth  = m_widthSpin->value();
    m_newHeight = m_heightSpin->value();
    m_newFps    = m_fpsCombо->currentData().toInt();
    m_newName   = m_projectNameEdit->text().trimmed();
    if (m_newName.isEmpty()) m_newName = "Untitled";

    m_result = Result::NewProject;
    accept();
}

void StartupScreen::onOpenClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Open Project",
        QDir::homePath(),
        "AkisVG Projects (*.akvg);;All Files (*)"
    );

    if (!path.isEmpty()) {
        m_chosenFilePath = path;
        m_result = Result::OpenProject;
        accept();
    }
}

void StartupScreen::onRecentItemDoubleClicked(QListWidgetItem *item)
{
    m_chosenFilePath = item->data(Qt::UserRole).toString();
    m_result = Result::OpenRecent;
    accept();
}

void StartupScreen::onOpenRecentClicked()
{
    auto sel = m_recentList->selectedItems();
    if (sel.isEmpty()) return;
    m_chosenFilePath = sel.first()->data(Qt::UserRole).toString();
    m_result = Result::OpenRecent;
    accept();
}
