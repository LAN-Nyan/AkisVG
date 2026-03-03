#ifndef ASSETLIBRARY_H
#define ASSETLIBRARY_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QLabel>

// Forward declaration
class ObjectGroup;

struct Asset {
    enum Type {
        Image,
        Audio,
        Group   // NEW: object groups from canvas
    };

    QString id;
    QString name;
    QString path;        // empty for Group assets
    Type type;
    QPixmap thumbnail;
    ObjectGroup *group = nullptr;  // non-null for Group assets
};

class AssetLibrary : public QWidget
{
    Q_OBJECT

public:
    explicit AssetLibrary(QWidget *parent = nullptr);

    QList<Asset> assets() const { return m_assets; }
    Asset* assetById(const QString &id);

    // Called by VectorCanvas when user groups objects
    void addObjectGroup(ObjectGroup *group);
    void applyTheme();

signals:
    void assetAdded(const Asset &asset);
    void assetRemoved(const QString &id);
    // Emitted when user wants to instance a group onto the canvas
    void groupInstanceRequested(ObjectGroup *group);

private slots:
    void onImportClicked();
    void onDeleteClicked();
    void showContextMenu(const QPoint &pos);
    void startDrag(QListWidgetItem *item);

private:
    void setupUI();
    void addAsset(const QString &path);
    void updateAssetList();
    QWidget* createAssetItem(const Asset &asset);
    QPixmap generateThumbnail(const QString &path, Asset::Type type);

    QListWidget *m_assetList;
    QPushButton *m_importButton;
    QPushButton *m_deleteButton;
    QWidget *m_header;
    QLabel *m_titleLabel;
    QList<Asset> m_assets;
};

#endif // ASSETLIBRARY_H
