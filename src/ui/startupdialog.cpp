#include "startupdialog.h"
#include "utils/thememanager.h"

#include <QApplication>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QListWidgetItem>
#include <QDateTime>
#include <QPushButton>
#include <QSpacerItem>
#include <QSvgRenderer>

// helpers
static QString inputStyle()
{
    return "QSpinBox, QComboBox, QLineEdit {"
           "  background:#141414; color:#eeeeee;" // Darker backgrounds
           "  border:1px solid #222; border-radius:5px; padding:5px 8px;"
           "  font-size:13px;"
           "}"
           "QSpinBox:hover, QComboBox:hover, QLineEdit:hover { border-color:#c0392b; }" // Red hover
           "QComboBox::drop-down { border:none; width:20px; }"
           "QComboBox QAbstractItemView { background:#141414; color:#eeeeee; selection-background-color:#c0392b; }";
}

static QString labelStyle() { return "color:#777; font-size:11px; font-weight:bold;"; }

// constructor
StartupDialog::StartupDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("AkisVG");
    setModal(true);
    setFixedSize(840, 560);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    setupUI();
    loadRecents();
}

// setupUI
void StartupDialog::setupUI()
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // LEFT PANEL
    QWidget *left = new QWidget();
    left->setFixedWidth(320);
    left->setStyleSheet("background:#0a0a0a; border-right:1px solid #1a1a1a;");
    QVBoxLayout *ll = new QVBoxLayout(left);
    ll->setContentsMargins(28, 100, 28, 24); // More top margin to clear the painted logo

    QLabel *appName = new QLabel("AkisVG");
    appName->setStyleSheet("color:white; font-size:30px; font-weight:bold;");
    ll->addWidget(appName);

    QLabel *tagline = new QLabel("Vector Animation Studio");
    tagline->setStyleSheet("color:#444; font-size:12px; margin-bottom:28px;");
    ll->addWidget(tagline);

    ll->addSpacing(20);
    QLabel *recentLabel = new QLabel("RECENT PROJECTS");
    recentLabel->setStyleSheet("color:#c0392b; font-size:10px; font-weight:bold; letter-spacing:1px;");
    ll->addWidget(recentLabel);

    m_recentList = new QListWidget();
    m_recentList->setStyleSheet(
        "QListWidget { background:transparent; border:none; color:#777; }"
        "QListWidget::item:selected { background:#c0392b; color:white; }"
    );
    ll->addWidget(m_recentList, 1);

    // Theme selector
    ll->addSpacing(12);
    QLabel *themeLabel = new QLabel("THEME");
    themeLabel->setStyleSheet("color:#c0392b; font-size:10px; font-weight:bold; letter-spacing:1px;");
    ll->addWidget(themeLabel);

    m_themeCombo = new QComboBox();
    m_themeCombo->addItem("🎬  Studio Grey",  0);
    m_themeCombo->addItem("🔴  Red & Black",  1);
    m_themeCombo->addItem("🔵  Blue & Black", 2);
    m_themeCombo->addItem("🩶  Grey & Blue",  3);
    m_themeCombo->addItem("🩷  Grey & Red",   4);
    // Restore previously saved theme choice
    {
        QSettings s("AkisVG", "AkisVG");
        int savedTheme = s.value("theme", 0).toInt();
        m_themeCombo->setCurrentIndex(qBound(0, savedTheme, 4));
        ThemeManager::instance().setTheme(savedTheme);
    }
    m_themeCombo->setStyleSheet(
        "QComboBox { background:#141414; color:#eee; border:1px solid #222; border-radius:4px; padding:5px 8px; font-size:12px; }"
        "QComboBox:hover { border-color:#c0392b; }"
        "QComboBox::drop-down { border:none; width:18px; }"
        "QComboBox QAbstractItemView { background:#141414; color:#eee; selection-background-color:#c0392b; }");
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        ThemeManager::instance().setTheme(idx);
        QSettings s("AkisVG", "AkisVG");
        s.setValue("theme", idx);
    });
    ll->addWidget(m_themeCombo);
    ll->addSpacing(8);

    // UI Scale
    QLabel *scaleLabel = new QLabel("UI SCALE");
    scaleLabel->setStyleSheet("color:#c0392b; font-size:10px; font-weight:bold; letter-spacing:1px;");
    ll->addWidget(scaleLabel);

    QWidget *scaleRow = new QWidget();
    QHBoxLayout *scaleRowLayout = new QHBoxLayout(scaleRow);
    scaleRowLayout->setContentsMargins(0, 0, 0, 0);
    scaleRowLayout->setSpacing(8);

    QSlider *scaleSlider = new QSlider(Qt::Horizontal);
    scaleSlider->setRange(50, 200);  // 50% to 200%
    scaleSlider->setTickInterval(25);  // 50,75,100,125,150,175,200
    {
        QSettings s("AkisVG", "AkisVG");
        scaleSlider->setValue(s.value("ui/scale", 100).toInt());
    }
    scaleSlider->setStyleSheet(
        "QSlider::groove:horizontal { background:#222; height:4px; border-radius:2px; }"
        "QSlider::handle:horizontal { background:#c0392b; width:12px; margin:-4px 0; border-radius:6px; }"
        "QSlider::sub-page:horizontal { background:#c0392b; border-radius:2px; }");

    QLabel *scaleValueLabel = new QLabel(QString("%1%").arg(scaleSlider->value()));
    scaleValueLabel->setStyleSheet("color:#eee; font-size:11px; min-width:36px;");
    scaleValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    connect(scaleSlider, &QSlider::valueChanged, this, [scaleValueLabel](int v) {
        scaleValueLabel->setText(QString("%1%").arg(v));
        QSettings s("AkisVG", "AkisVG");
        s.setValue("ui/scale", v);
        double factor = v / 100.0;
        QFont f = QApplication::font();
        f.setPointSizeF(9.0 * factor);
        QApplication::setFont(f);
        int btnH    = qRound(28  * factor);
        int inputH  = qRound(24  * factor);
        int scrollW = qRound(8   * factor);
        int menuPad = qRound(4   * factor);
        int pad     = qRound(4   * factor);
        int radius  = qRound(4   * factor);
        if (v != 100) {
            qApp->setStyleSheet(QString(
                "QPushButton  { min-height: %1px; padding: 0 %2px; border-radius: %8px; }"
                "QToolButton  { min-height: %1px; min-width:  %1px; }"
                "QLineEdit    { min-height: %3px; padding: %2px; border-radius: %8px; }"
                "QSpinBox     { min-height: %3px; padding: %2px; }"
                "QDoubleSpinBox { min-height: %3px; padding: %2px; }"
                "QComboBox    { min-height: %3px; padding: 0 %2px; }"
                "QScrollBar:vertical   { width:  %4px; }"
                "QScrollBar:horizontal { height: %4px; }"
                "QMenu::item  { padding: %5px %6px; }"
                "QMenuBar::item{ padding: %5px %6px; }"
                "QGroupBox    { padding-top: %7px; margin-top: %7px; }"
                "QDockWidget::title { padding: %5px; }"
                "QTabBar::tab { min-height: %3px; padding: %5px %6px; }"
            )
            .arg(btnH).arg(pad).arg(inputH).arg(scrollW)
            .arg(menuPad).arg(menuPad*4).arg(menuPad*2)
            .arg(radius));
        } else {
            qApp->setStyleSheet(QString());
        }
    });

    scaleRowLayout->addWidget(scaleSlider, 1);
    scaleRowLayout->addWidget(scaleValueLabel);
    ll->addWidget(scaleRow);
    ll->addSpacing(8);

    m_openBtn = new QPushButton("  Open Project…");
    m_openBtn->setFixedHeight(36);
    m_openBtn->setStyleSheet(
        "QPushButton { background:#1a1a1a; color:#888; border:1px solid #222; border-radius:5px; font-size:13px; }"
        "QPushButton:hover { border-color:#c0392b; color:#ccc; }");
    connect(m_openBtn, &QPushButton::clicked, this, &StartupDialog::onOpenClicked);
    ll->addWidget(m_openBtn);

    // Wire up double-click on recent list
    connect(m_recentList, &QListWidget::itemDoubleClicked,
            this, &StartupDialog::onRecentDoubleClicked);

    root->addWidget(left);

    // RIGHT PANEL
    QWidget *right = new QWidget();
    right->setStyleSheet("background:#111111;");
    QVBoxLayout *rl = new QVBoxLayout(right);
    rl->setContentsMargins(40, 40, 40, 40);
    rl->setSpacing(8);

    QLabel *newTitle = new QLabel("New Project");
    newTitle->setStyleSheet("color:white; font-size:22px; font-weight:bold; margin-bottom:8px;");
    rl->addWidget(newTitle);
    rl->addSpacing(20);

    // GRID FORM
    QGridLayout *form = new QGridLayout();
    form->setSpacing(10);

    auto addRow = [&](int row, const QString &lbl, QWidget *w) {
        QLabel *l = new QLabel(lbl);
        l->setStyleSheet("color:#555; font-size:11px; font-weight:bold;");
        form->addWidget(l, row, 0, Qt::AlignRight | Qt::AlignVCenter);
        w->setStyleSheet("background:#161616; color:#eee; border:1px solid #222; padding:5px; border-radius:4px;");
        form->addWidget(w, row, 1);
    };

    m_nameEdit = new QLineEdit("Untitled");
    addRow(0, "Name:", m_nameEdit);

    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("HD 1920x1080", QSize(1920, 1080));
    m_presetCombo->addItem("Custom…", QSize(-1, -1));
    addRow(1, "Preset:", m_presetCombo);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StartupDialog::onPresetChanged);

    m_widthSpin = new QSpinBox(); m_widthSpin->setRange(64, 7680); m_widthSpin->setValue(1920);
    addRow(2, "Width:", m_widthSpin);

    m_heightSpin = new QSpinBox(); m_heightSpin->setRange(64, 7680); m_heightSpin->setValue(1080);
    addRow(3, "Height:", m_heightSpin);

    m_fpsComboo = new QComboBox();
    m_fpsComboo->addItem("24 fps", 24);
    m_fpsComboo->addItem("30 fps", 30);
    m_fpsComboo->addItem("60 fps", 60);
    addRow(4, "FPS:", m_fpsComboo);

    rl->addLayout(form);
    rl->addStretch(1);

    m_createBtn = new QPushButton("Create Project  →");
    m_createBtn->setFixedHeight(44);
    m_createBtn->setStyleSheet("background:#c0392b; color:white; font-weight:bold; border-radius:6px;");
    connect(m_createBtn, &QPushButton::clicked, this, &StartupDialog::onCreateClicked);
    rl->addWidget(m_createBtn);

    root->addWidget(right, 1);
}

// Recent projects
void StartupDialog::loadRecents()
{
    QSettings s("AkisVG", "AkisVG");
    QStringList recents = s.value("recentFiles").toStringList();
    QStringList legacy  = s.value("recentProjects").toStringList();
    for (const QString &p : legacy)
        if (!recents.contains(p)) recents.append(p);

    m_recentList->clear();
    for (const QString &path : recents) {
        if (!QFileInfo::exists(path)) continue;
        QFileInfo fi(path);
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(fi.baseName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        item->setIcon(QIcon::fromTheme("text-x-generic"));
        m_recentList->addItem(item);
    }

    if (m_recentList->count() == 0) {
        QListWidgetItem *placeholder = new QListWidgetItem("No recent projects");
        placeholder->setForeground(QColor("#3a4a6a"));
        placeholder->setFlags(Qt::NoItemFlags);
        m_recentList->addItem(placeholder);
    }
}

void StartupDialog::saveRecent(const QString &path)
{
    QSettings s("AkisVG", "AkisVG");
    QStringList recents = s.value("recentFiles").toStringList();
    recents.removeAll(path);
    recents.prepend(path);
    while (recents.size() > 10) recents.removeLast();
    s.setValue("recentFiles", recents);
}

void StartupDialog::onRecentDoubleClicked(QListWidgetItem *item)
{
    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty() || !QFileInfo::exists(path)) return;
    m_openPath = path;
    m_action   = Action::OpenProject;
    accept();
}

// Slots
void StartupDialog::onCreateClicked()
{
    m_action = Action::NewProject;
    accept();
}

void StartupDialog::onOpenClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project or Import Video", QString(),
        "AkisVG Projects (*.akisvg *.json);;"
        "Video Files (*.mp4 *.mkv *.mov *.avi);;"
        "All Files (*)");
    if (path.isEmpty()) return;

    m_openPath = path;

    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "mp4" || ext == "mkv" || ext == "mov" || ext == "avi") {
        m_action = Action::ImportVideo;
    } else {
        m_action = Action::OpenProject;
        saveRecent(path);
    }

    accept();
}

void StartupDialog::onPresetChanged(int index)
{
    QSize sz = m_presetCombo->itemData(index).toSize();
    if (sz.width() > 0 && sz.height() > 0) {
        m_widthSpin->blockSignals(true);
        m_heightSpin->blockSignals(true);
        m_widthSpin->setValue(sz.width());
        m_heightSpin->setValue(sz.height());
        m_widthSpin->blockSignals(false);
        m_heightSpin->blockSignals(false);
    }
    bool isCustom = (index == m_presetCombo->count() - 1);
    m_widthSpin->setReadOnly(!isCustom && sz.width() > 0);
    m_heightSpin->setReadOnly(!isCustom && sz.height() > 0);
}

// Getters
QString StartupDialog::projectName()  const { return m_nameEdit->text().trimmed(); }
int     StartupDialog::canvasWidth()  const { return m_widthSpin->value(); }
int     StartupDialog::canvasHeight() const { return m_heightSpin->value(); }
int     StartupDialog::fps()          const { return m_fpsComboo->currentData().toInt(); }

void StartupDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.translate(48, 56);

    int size = 52;
    float half = size / 2.0f;

    QPainterPath path;
    path.moveTo(half, 0);
    path.lineTo(size, half);
    path.lineTo(half, size);
    path.lineTo(0, half);
    path.closeSubpath();

    path.moveTo(half, 5);
    path.lineTo(half + 10, half);
    path.lineTo(half, size - 5);
    path.lineTo(half - 10, half);
    path.closeSubpath();

    QLinearGradient grad(0, 0, size, size);
    grad.setColorAt(0, QColor("#e74c3c"));
    grad.setColorAt(1, QColor("#8e1c12"));

    painter.fillPath(path, grad);
    painter.setPen(QPen(QColor(255, 255, 255, 40), 1.5));
    painter.drawPath(path);
}
