#ifndef LAYERPANEL_H
#define LAYERPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QMenu>
#include <QUndoStack>

class Project;
class Layer;

class LayerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LayerPanel(Project *project, QUndoStack *undoStack, QWidget *parent = nullptr);

private slots:
    // Structural updates (Safe to call when layers are added/removed)
    void rebuildLayerList();

    // State updates (Safe to call during mouse clicks/selection changes)
    void updateSelection();
    void updateButtons();
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    // Interaction handlers
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
    QUndoStack *m_undoStack;
    QListWidget *m_layerList;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_duplicateButton;
    QPushButton *m_moveUpButton;
    QPushButton *m_moveDownButton;
};

#endif // LAYERPANEL_H

