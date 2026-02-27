#include "assetlibrary.h"

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

AssetLibrary::AssetLibrary(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void AssetLibrary::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    QWidget *header = new QWidget();
    header->setStyleSheet("background-color: #3a3a3a; border-bottom: 1px solid #000;");
    header->setFixedHeight(40);

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel *title = new QLabel("ASSETS");
    title->setStyleSheet("color: white; font-weight: bold; font-size: 11px; letter-spacing: 1px;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    // Import button
    m_importButton = new QPushButton("+");
    m_importButton->setFixedSize(28, 28);
    m_importButton->setToolTip("Import Asset");
    m_importButton->setCursor(Qt::PointingHandCursor);
    m_importButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2a82da;"
        "   border: none;"
        "   border-radius: 4px;"
        "   color: white;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a92ea;"
        "}"
        );
    connect(m_importButton, &QPushButton::clicked, this, &AssetLibrary::onImportClicked);
    headerLayout->addWidget(m_importButton);

    mainLayout->addWidget(header);

    // Asset list
    m_assetList = new QListWidget();
    m_assetList->setStyleSheet(
        "QListWidget {"
        "   background-color: #2d2d2d;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   border-bottom: 1px solid #1a1a1a;"
        "   padding: 0;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(42, 130, 218, 0.3);"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #3a3a3a;"
        "}"
        "QScrollBar:vertical {"
        "   background: #1a1a1a; width: 8px; border-radius: 4px; margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #444; border-radius: 4px; min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #2a82da; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        );
    m_assetList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assetList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_assetList->setDragEnabled(true);
    m_assetList->setDefaultDropAction(Qt::CopyAction);

    connect(m_assetList, &QListWidget::customContextMenuRequested, this, &AssetLibrary::showContextMenu);
    connect(m_assetList, &QListWidget::itemPressed, this, &AssetLibrary::startDrag);

    mainLayout->addWidget(m_assetList);

    // Drop zone hint
    QLabel *dropHint = new QLabel(
        "<div style='text-align: center; padding: 20px;'>"
        "<div style='font-size: 32px; margin-bottom: 8px;'>📁</div>"
        "<div style='color: #888; font-size: 11px;'>Drop files here or click + to import</div>"
        "<div style='color: #666; font-size: 10px; margin-top: 4px;'>Supported: PNG, JPG, SVG, MP3, WAV</div>"
        "</div>"
        );
    dropHint->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(dropHint);

    // Enable drag and drop
    setAcceptDrops(true);
}

void AssetLibrary::onImportClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Import Assets", "",
                                                      "All Supported (*.png *.jpg *.jpeg *.svg *.mp3 *.wav *.ogg);;"
                                                      "Images (*.png *.jpg *.jpeg *.svg);;"
                                                      "Audio (*.mp3 *.wav *.ogg)");

    for (const QString &file : files) {
        addAsset(file);
    }
}

void AssetLibrary::onDeleteClicked()
{
    QListWidgetItem *current = m_assetList->currentItem();
    if (!current) return;

    QString assetId = current->data(Qt::UserRole).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Asset",
                                                              "Remove this asset from the library?",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_assets.removeIf([assetId](const Asset &a) { return a.id == assetId; });
        updateAssetList();
        emit assetRemoved(assetId);
    }
}

void AssetLibrary::addAsset(const QString &path)
{
    QFileInfo fileInfo(path);

    Asset asset;
    asset.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    asset.name = fileInfo.fileName();
    asset.path = path;

    QString suffix = fileInfo.suffix().toLower();
    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "svg") {
        asset.type = Asset::Image;
    } else if (suffix == "mp3" || suffix == "wav" || suffix == "ogg") {
        asset.type = Asset::Audio;
    } else {
        return; // Unsupported type
    }

    asset.thumbnail = generateThumbnail(path, asset.type);

    m_assets.append(asset);
    updateAssetList();
    emit assetAdded(asset);
}

QPixmap AssetLibrary::generateThumbnail(const QString &path, Asset::Type type)
{
    QPixmap thumbnail(64, 64);
    thumbnail.fill(Qt::transparent);

    if (type == Asset::Image) {
        QPixmap original(path);
        if (!original.isNull()) {
            thumbnail = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            QPainter painter(&thumbnail);
            painter.fillRect(thumbnail.rect(), QColor(100, 100, 100));
            painter.setPen(Qt::white);
            painter.drawText(thumbnail.rect(), Qt::AlignCenter, "?");
        }
    } else if (type == Asset::Audio) {
        QPainter painter(&thumbnail);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(thumbnail.rect(), QColor(80, 60, 120));
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPixelSize(32);
        painter.setFont(font);
        painter.drawText(thumbnail.rect(), Qt::AlignCenter, "♪");
    }

    return thumbnail;
}

void AssetLibrary::updateAssetList()
{
    m_assetList->clear();

    for (const Asset &asset : m_assets) {
        QListWidgetItem *item = new QListWidgetItem(m_assetList);
        item->setSizeHint(QSize(0, 80));
        item->setData(Qt::UserRole, asset.id);

        QWidget *itemWidget = createAssetItem(asset);
        m_assetList->setItemWidget(item, itemWidget);
    }
}

QWidget* AssetLibrary::createAssetItem(const Asset &asset)
{
    QWidget *itemWidget = new QWidget();
    itemWidget->setFixedHeight(80);
    itemWidget->setProperty("assetId", asset.id);  // Store ID for drag

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    // Thumbnail (make it draggable)
    QLabel *thumbLabel = new QLabel();
    thumbLabel->setPixmap(asset.thumbnail);
    thumbLabel->setFixedSize(64, 64);
    thumbLabel->setScaledContents(false);
    thumbLabel->setStyleSheet("border: 1px solid #555; border-radius: 4px; background-color: #1a1a1a;");
    thumbLabel->setProperty("assetId", asset.id);
    thumbLabel->setProperty("assetType", static_cast<int>(asset.type));
    thumbLabel->setProperty("assetPath", asset.path);
    layout->addWidget(thumbLabel);

    // Info
    QVBoxLayout *infoLayout = new QVBoxLayout();

    QLabel *nameLabel = new QLabel(asset.name);
    nameLabel->setStyleSheet("color: white; font-size: 12px; font-weight: 500;");
    nameLabel->setWordWrap(true);
    infoLayout->addWidget(nameLabel);

    QString typeStr = (asset.type == Asset::Image) ? "Image" : "Audio";
    QFileInfo fileInfo(asset.path);
    QString sizeStr = QString::number(fileInfo.size() / 1024) + " KB";

    QLabel *detailLabel = new QLabel(QString("%1 • %2").arg(typeStr, sizeStr));
    detailLabel->setStyleSheet("color: #888; font-size: 10px;");
    infoLayout->addWidget(detailLabel);

    infoLayout->addStretch();
    layout->addLayout(infoLayout, 1);

    // Type icon
    QString icon = (asset.type == Asset::Image) ? "🖼️" : "🎵";
    QLabel *iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet("font-size: 24px;");
    layout->addWidget(iconLabel);

    return itemWidget;
}

void AssetLibrary::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_assetList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu {"
        "   background-color: #2d2d2d;"
        "   color: white;"
        "   border: 1px solid #000;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #2a82da;"
        "}"
        );

    QAction *deleteAct = menu.addAction("Delete Asset");

    QAction *selected = menu.exec(m_assetList->mapToGlobal(pos));

    if (selected == deleteAct) {
        m_assetList->setCurrentItem(item);
        onDeleteClicked();
    }
}

Asset* AssetLibrary::assetById(const QString &id)
{
    for (Asset &asset : m_assets) {
        if (asset.id == id) {
            return &asset;
        }
    }
    return nullptr;
}

void AssetLibrary::startDrag(QListWidgetItem *item)
{
    if (!item || !(QApplication::mouseButtons() & Qt::LeftButton)) return;

    QString assetId = item->data(Qt::UserRole).toString();
    Asset *asset = assetById(assetId);
    if (!asset) return;

    // Create MIME data
    QMimeData *mimeData = new QMimeData();
    mimeData->setText(asset->id);
    mimeData->setData("application/x-lumina-asset", asset->id.toUtf8());
    mimeData->setData("application/x-lumina-asset-type",
                      QByteArray::number(static_cast<int>(asset->type)));
    mimeData->setData("application/x-lumina-asset-path", asset->path.toUtf8());

    // Create drag with thumbnail
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(asset->thumbnail.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    drag->setHotSpot(QPoint(24, 24));

    drag->exec(Qt::CopyAction);
}
