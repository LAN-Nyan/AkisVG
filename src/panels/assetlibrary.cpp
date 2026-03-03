#include "assetlibrary.h"
#include "canvas/objects/objectgroup.h"
#include "utils/thememanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QPixmap>
#include <QMimeData>
#include <QDrag>
#include <QPainter>
#include <QDateTime>
#include <QApplication>
#include <QStackedLayout>

AssetLibrary::AssetLibrary(QWidget *parent) : QWidget(parent) { setupUI(); }

void AssetLibrary::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Set the base widget background to match "Panel background"
    this->setStyleSheet("background-color: #1e1e1e;");

    // --- HEADER ---
    // Updated from #3a3a3a to #1e1e1e with a #1a1a1a border
    m_header = new QWidget();
    m_header->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #1a1a1a;");
    m_header->setFixedHeight(40);

    QHBoxLayout *hl = new QHBoxLayout(m_header);
    hl->setContentsMargins(12, 0, 12, 0);

    m_titleLabel = new QLabel("ASSETS");
    // Matching the ToolBox title style: #c0392b (Active/accent)
    m_titleLabel->setStyleSheet("color: #c0392b; font-weight: bold; font-size: 10px; letter-spacing: 2px;");
    hl->addWidget(m_titleLabel);
    hl->addStretch();

    m_importButton = new QPushButton("+");
    m_importButton->setFixedSize(24, 24); // Slightly smaller for a tighter look
    m_importButton->setToolTip("Import Asset");
    m_importButton->setCursor(Qt::PointingHandCursor);
    m_importButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #c0392b;" // Active/accent
        "   border: none;"
        "   border-radius: 4px;"
        "   color: white;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e74c3c;" // Accent bright
        "}"
    );
    connect(m_importButton, &QPushButton::clicked, this, &AssetLibrary::onImportClicked);
    hl->addWidget(m_importButton);

    mainLayout->addWidget(m_header);

    // --- ASSET LIST ---
    m_assetList = new QListWidget();
    m_assetList->setStyleSheet(
        "QListWidget {"
        "   background-color: #1e1e1e;"  /* Panel background */
        "   border: none;"
        "   outline: none;"
        "   padding: 0px;"             /* No padding for the container */
        "   margin: 0px;"              /* No margin for the container */
        "}"
        "QListWidget::item {"
        "   background-color: #242424;"  /* Secondary bg */
        "   border-bottom: 1px solid #1a1a1a;" /* Separator line */
        "   margin: 0px;"               /* CRITICAL: Kills the 'sagging' offset */
        "   padding: 0px;"              /* Handle internal spacing via a layout, not CSS */
        "   color: #999;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #c0392b;"  /* Startup Active Red */
        "   color: white;"
        "   border-bottom: 1px solid #e74c3c;" /* Bright accent edge */
        "}"
        "QListWidget::item:selected:active {"
        "   background-color: #c0392b;"  /* Ensure it stays red when focused */
        "}"
    );

    m_assetList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assetList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_assetList->setDragEnabled(true);
    m_assetList->setDefaultDropAction(Qt::CopyAction);

    connect(m_assetList, &QListWidget::customContextMenuRequested, this, &AssetLibrary::showContextMenu);
    connect(m_assetList, &QListWidget::itemPressed, this, &AssetLibrary::startDrag);

    mainLayout->addWidget(m_assetList);

    // --- HINT LABEL ---
    // Cleaned up the colors to avoid the "grey block" look
    QLabel *hint = new QLabel(
        "<div style='text-align:center; padding:20px;'>"
        "<div style='font-size:24px; margin-bottom:8px; color:#444;'>📁</div>"
        "<div style='color:#666; font-size:10px; font-weight: bold; letter-spacing: 1px;'>EMPTY LIBRARY</div>"
        "<div style='color:#444; font-size:9px; margin-top:4px;'>Drop files or click + to import</div>"
        "</div>");
    hint->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(hint);

    setAcceptDrops(true);
}

// ─── Group auto-registration ──────────────────────────────────────────────────
void AssetLibrary::addObjectGroup(ObjectGroup *group)
{
    if (!group) return;
    Asset asset;
    asset.id        = QString("group_%1").arg(QDateTime::currentMSecsSinceEpoch());
    asset.name      = group->groupName().isEmpty() ? "Unnamed Group" : group->groupName();
    asset.path      = QString();
    asset.type      = Asset::Group;
    asset.group     = group;
    asset.thumbnail = group->thumbnail(64);
    m_assets.append(asset);
    updateAssetList();
    emit assetAdded(asset);
}

void AssetLibrary::onImportClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Import Assets", "",
        "All Supported (*.png *.jpg *.jpeg *.svg *.mp3 *.wav *.ogg);;"
        "Images (*.png *.jpg *.jpeg *.svg);;Audio (*.mp3 *.wav *.ogg)");
    for (const QString &f : files) addAsset(f);
}

void AssetLibrary::onDeleteClicked()
{
    QListWidgetItem *cur = m_assetList->currentItem();
    if (!cur) return;
    QString id = cur->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, "Delete Asset", "Remove from library?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_assets.removeIf([id](const Asset &a){ return a.id == id; });
        updateAssetList();
        emit assetRemoved(id);
    }
}

void AssetLibrary::addAsset(const QString &path)
{
    QFileInfo fi(path);
    Asset asset;
    asset.id   = QString::number(QDateTime::currentMSecsSinceEpoch());
    asset.name = fi.fileName();
    asset.path = path;
    QString sfx = fi.suffix().toLower();
    if (sfx=="png"||sfx=="jpg"||sfx=="jpeg"||sfx=="svg")      asset.type = Asset::Image;
    else if (sfx=="mp3"||sfx=="wav"||sfx=="ogg")               asset.type = Asset::Audio;
    else return;
    asset.thumbnail = generateThumbnail(path, asset.type);
    m_assets.append(asset);
    updateAssetList();
    emit assetAdded(asset);
}

QPixmap AssetLibrary::generateThumbnail(const QString &path, Asset::Type type)
{
    QPixmap thumb(64, 64);
    thumb.fill(Qt::transparent);
    if (type == Asset::Image) {
        QPixmap orig(path);
        if (!orig.isNull())
            thumb = orig.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        else {
            QPainter p(&thumb); p.fillRect(thumb.rect(), QColor(100,100,100));
            p.setPen(Qt::white); p.drawText(thumb.rect(), Qt::AlignCenter, "?");
        }
    } else {
        QPainter p(&thumb); p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(thumb.rect(), QColor(80,60,120));
        p.setPen(Qt::white);
        QFont f = p.font(); f.setPixelSize(32); p.setFont(f);
        p.drawText(thumb.rect(), Qt::AlignCenter, "♪");
    }
    return thumb;
}

void AssetLibrary::updateAssetList()
{
    m_assetList->clear();
    for (const Asset &asset : m_assets) {
        QListWidgetItem *item = new QListWidgetItem(m_assetList);
        item->setSizeHint(QSize(0, asset.type == Asset::Group ? 88 : 80));
        item->setData(Qt::UserRole, asset.id);
        m_assetList->setItemWidget(item, createAssetItem(asset));
    }
}

QWidget* AssetLibrary::createAssetItem(const Asset &asset)
{
    QWidget *w = new QWidget();
    w->setFixedHeight(asset.type == Asset::Group ? 88 : 80);
    w->setProperty("assetId", asset.id);

    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    QLabel *thumb = new QLabel();
    thumb->setPixmap(asset.thumbnail);
    thumb->setFixedSize(64, 64);
    thumb->setStyleSheet("border:1px solid #555; border-radius:4px; background-color:#1a1a1a;");
    thumb->setProperty("assetId", asset.id);
    thumb->setProperty("assetType", static_cast<int>(asset.type));
    thumb->setProperty("assetPath", asset.path);
    layout->addWidget(thumb);

    QVBoxLayout *info = new QVBoxLayout();
    QLabel *name = new QLabel(asset.name);
    name->setStyleSheet("color:white; font-size:12px; font-weight:500;");
    name->setWordWrap(true);
    info->addWidget(name);

    QString detail;
    if (asset.type == Asset::Image)      detail = QString("Image • %1 KB").arg(QFileInfo(asset.path).size()/1024);
    else if (asset.type == Asset::Audio) detail = QString("Audio • %1 KB").arg(QFileInfo(asset.path).size()/1024);
    else {
        int cnt = asset.group ? asset.group->childCount() : 0;
        detail  = QString("Group • %1 object%2").arg(cnt).arg(cnt==1?"":"s");
    }
    QLabel *det = new QLabel(detail);
    det->setStyleSheet("color:#888; font-size:10px;");
    info->addWidget(det);

    // Instance button for groups
    if (asset.type == Asset::Group && asset.group) {
        QPushButton *btn = new QPushButton("▶  Instance onto Canvas");
        btn->setFixedHeight(22);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background:#2a82da; color:white; border:none; border-radius:3px; font-size:10px; padding:2px 8px; }"
            "QPushButton:hover { background:#3a92ea; }");
        ObjectGroup *grp = asset.group;
        connect(btn, &QPushButton::clicked, this, [this, grp](){ emit groupInstanceRequested(grp); });
        info->addWidget(btn);
    }

    info->addStretch();
    layout->addLayout(info, 1);

    QString icon = (asset.type==Asset::Image) ? "🖼️" : (asset.type==Asset::Audio) ? "🎵" : "📦";
    QLabel *icn = new QLabel(icon);
    icn->setStyleSheet("font-size:24px;");
    layout->addWidget(icn);

    return w;
}

void AssetLibrary::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_assetList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.setStyleSheet("QMenu { background:#2d2d2d; color:white; border:1px solid #000; }"
                       "QMenu::item:selected { background:#2a82da; }");

    QString id = item->data(Qt::UserRole).toString();
    Asset *asset = assetById(id);

    QAction *instanceAct = nullptr;
    if (asset && asset->type == Asset::Group) {
        instanceAct = menu.addAction("Instance onto Canvas");
        menu.addSeparator();
    }
    QAction *deleteAct = menu.addAction("Delete Asset");
    QAction *sel = menu.exec(m_assetList->mapToGlobal(pos));

    if (sel == deleteAct) {
        m_assetList->setCurrentItem(item);
        onDeleteClicked();
    } else if (instanceAct && sel == instanceAct && asset && asset->group) {
        emit groupInstanceRequested(asset->group);
    }
}

Asset* AssetLibrary::assetById(const QString &id)
{
    for (Asset &a : m_assets)
        if (a.id == id) return &a;
    return nullptr;
}

void AssetLibrary::startDrag(QListWidgetItem *item)
{
    if (!item || !(QApplication::mouseButtons() & Qt::LeftButton)) return;
    Asset *asset = assetById(item->data(Qt::UserRole).toString());
    if (!asset) return;

    QMimeData *mime = new QMimeData();
    mime->setText(asset->id);
    mime->setData("application/x-lumina-asset", asset->id.toUtf8());
    mime->setData("application/x-lumina-asset-type", QByteArray::number(static_cast<int>(asset->type)));
    mime->setData("application/x-lumina-asset-path", asset->path.toUtf8());

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(asset->thumbnail.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    drag->setHotSpot(QPoint(24, 24));
    drag->exec(Qt::CopyAction);
}

void AssetLibrary::applyTheme()
{
    const ThemeColors &t = theme();

    this->setStyleSheet(QString("background-color: %1;").arg(t.bg0));
    m_header->setStyleSheet(
        QString("background-color: %1; border-bottom: 1px solid %2;").arg(t.bg0, t.bg1));
    m_titleLabel->setStyleSheet(
        QString("color: %1; font-weight: bold; font-size: 10px; letter-spacing: 2px;").arg(t.accent));

    m_importButton->setStyleSheet(
        QString("QPushButton { background-color: %1; border: none; border-radius: 4px;"
                " color: white; font-size: 18px; font-weight: bold; }"
                "QPushButton:hover { background-color: %2; }").arg(t.accent, t.accentHover));

    m_assetList->setStyleSheet(
        QString("QListWidget { background-color: %1; border: none; }"
                "QListWidget::item { border-bottom: 1px solid %2; }"
                "QListWidget::item:selected { background-color: %3; }"
                "QListWidget::item:selected:focus { background-color: %3; }"
                "QScrollBar:vertical { background: %1; width: 8px; border-radius: 4px; }"
                "QScrollBar::handle:vertical { background: %2; border-radius: 4px; }"
                "QScrollBar::handle:vertical:hover { background: %3; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
        .arg(t.bg0, t.bg1, t.accent));
}
