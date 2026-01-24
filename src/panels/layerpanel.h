#ifndef LAYERPANEL_H
#define LAYERPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QMenu>

class Project;
class Layer;

class LayerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LayerPanel(Project *project, QWidget *parent = nullptr);

private slots:
    void updateLayerList();
    void onLayerItemClicked(QListWidgetItem *item);
    void onAddLayerClicked();
    void onDeleteLayerClicked();
    void onDuplicateLayerClicked();
    void onMoveLayerUp();
    void onMoveLayerDown();
    void showContextMenu(const QPoint &pos);

private:
    void setupUI();
    QWidget* createLayerItem(Layer *layer, int index);

    Project *m_project;
    QListWidget *m_layerList;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_duplicateButton;
    QPushButton *m_moveUpButton;
    QPushButton *m_moveDownButton;
};

#endif // LAYERPANEL_H
