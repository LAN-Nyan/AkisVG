#ifndef ASSETLIBRARY_H
#define ASSETLIBRARY_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QString>

struct Asset {
    enum Type {
        Image,
        Audio
    };

    QString id;
    QString name;
    QString path;
    Type type;
    QPixmap thumbnail;
};

class AssetLibrary : public QWidget
{
    Q_OBJECT

public:
    explicit AssetLibrary(QWidget *parent = nullptr);

    QList<Asset> assets() const { return m_assets; }
    Asset* assetById(const QString &id);

signals:
    void assetAdded(const Asset &asset);
    void assetRemoved(const QString &id);

private slots:
    void onImportClicked();
    void onDeleteClicked();
    void showContextMenu(const QPoint &pos);

private:
    void setupUI();
    void addAsset(const QString &path);
    void updateAssetList();
    QWidget* createAssetItem(const Asset &asset);
    QPixmap generateThumbnail(const QString &path, Asset::Type type);

    QListWidget *m_assetList;
    QPushButton *m_importButton;
    QPushButton *m_deleteButton;
    QList<Asset> m_assets;
};

#endif // ASSETLIBRARY_H
