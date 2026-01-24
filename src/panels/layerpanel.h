#include "layerpanel.h"
#include "core/project.h"
#include "core/layer.h"
#include "core/commands.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>

void LayerPanel::dragEnterEvent(QDragEnterEvent *event) {
    // Only accept if it's a file (URL)
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void LayerPanel::dropEvent(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QString filePath = mimeData->urls().at(0).toLocalFile();

        // Simple check for audio extensions
        if (filePath.endsWith(".mp3") || filePath.endsWith(".wav") || filePath.endsWith(".ogg")) {

            // Find which layer item we dropped it on
            QWidget* child = childAt(event->position().toPoint());
            // You'll need logic here to find the actual Layer* object associated with that widget

            qDebug() << "Audio file dropped:" << filePath;

            // TODO: Update the Layer's audio path and trigger waveform generation
        }
    }
}

LayerPanel::LayerPanel(Project *project, QUndoStack *undoStack, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_undoStack(undoStack)
{
    // SAFETY CHECK: Ensure undoStack is valid
    if (!m_undoStack) {
        qWarning("LayerPanel received null UndoStack! App will crash on action.");
    }

    setupUI();
    rebuildLayerList(); // Initial build

    // CRITICAL FIX: Split connections to avoid crashing
    // Only rebuild the list if layers are actually added/removed/reordered
    connect(m_project, &Project::layersChanged, this, &LayerPanel::rebuildLayerList, Qt::QueuedConnection);

    // If just the SELECTION changes, do NOT destroy the list items. Just update them.
    connect(m_project, &Project::currentLayerChanged, this, &LayerPanel::updateSelection);
}
void LayerPanel::setupUI()
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

    QLabel *title = new QLabel("LAYERS");
    title->setStyleSheet("color: white; font-weight: bold; font-size: 11px; letter-spacing: 1px;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    // Add button
    m_addButton = new QPushButton("+");
    m_addButton->setFixedSize(28, 28);
    m_addButton->setToolTip("Add Layer");
    m_addButton->setCursor(Qt::PointingHandCursor);
    m_addButton->setStyleSheet(
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
    connect(m_addButton, &QPushButton::clicked, this, &LayerPanel::onAddLayerClicked);
    headerLayout->addWidget(m_addButton);

    mainLayout->addWidget(header);

    // Layer list
    m_layerList = new QListWidget();
    m_layerList->setStyleSheet(
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
        );
    m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_layerList, &QListWidget::itemClicked, this, &LayerPanel::onLayerItemClicked);
    connect(m_layerList, &QListWidget::customContextMenuRequested, this, &LayerPanel::showContextMenu);

    mainLayout->addWidget(m_layerList);

    // Bottom toolbar
    QWidget *toolbar = new QWidget();
    toolbar->setStyleSheet("background-color: #3a3a3a; border-top: 1px solid #000;");
    toolbar->setFixedHeight(44);

    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 6, 8, 6);
    toolbarLayout->setSpacing(6);

    auto createToolButton = [](const QString &text, const QString &tooltip) {
        QPushButton *btn = new QPushButton(text);
        btn->setFixedSize(32, 32);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: #2d2d2d;"
            "   border: 1px solid #555;"
            "   border-radius: 4px;"
            "   color: #ccc;"
            "   font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #3a3a3a;"
            "   border-color: #2a82da;"
            "   color: white;"
            "}"
            "QPushButton:disabled {"
            "   color: #555;"
            "   border-color: #333;"
            "}"
            );
        return btn;
    };

    m_duplicateButton = createToolButton("⧉", "Duplicate Layer");
    connect(m_duplicateButton, &QPushButton::clicked, this, &LayerPanel::onDuplicateLayerClicked);
    toolbarLayout->addWidget(m_duplicateButton);

    m_deleteButton = createToolButton("🗑", "Delete Layer");
    connect(m_deleteButton, &QPushButton::clicked, this, &LayerPanel::onDeleteLayerClicked);
    toolbarLayout->addWidget(m_deleteButton);

    toolbarLayout->addSpacing(8);

    m_moveUpButton = createToolButton("▲", "Move Layer Up");
    connect(m_moveUpButton, &QPushButton::clicked, this, &LayerPanel::onMoveLayerUp);
    toolbarLayout->addWidget(m_moveUpButton);

    m_moveDownButton = createToolButton("▼", "Move Layer Down");
    connect(m_moveDownButton, &QPushButton::clicked, this, &LayerPanel::onMoveLayerDown);
    toolbarLayout->addWidget(m_moveDownButton);

    toolbarLayout->addStretch();

    mainLayout->addWidget(toolbar);
}

QWidget* LayerPanel::createLayerItem(Layer *layer, int index)
{
    QWidget *itemWidget = new QWidget();
    itemWidget->setFixedHeight(48);

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(8);

    // Color indicator
    QLabel *colorLabel = new QLabel();
    colorLabel->setFixedSize(6, 36);
    colorLabel->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(layer->color().name()));
    layout->addWidget(colorLabel);

    // Corrected logic for your switch statement
    QString layerColor;
    switch(layer->layerType()) { // Assuming type() returns your LayerType enum
    case LayerType::Audio:
        layerColor = "#9b59b6"; break; // Purple
    case LayerType::Art:
        layerColor = "#3498db"; break; // Blue
    case LayerType::Reference:
        layerColor = "#e74c3c"; break; // Red
    default:
        layerColor = "#2ecc71"; break; // Green
    }

    colorLabel->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(layerColor));

    colorLabel->setStyleSheet(QString("background-color: %1; border-radius: 3px;")
                                  .arg(layerColor));

    colorLabel->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(layerColor));

    // Layer name
    QLabel *nameLabel = new QLabel(layer->name());
    nameLabel->setStyleSheet("color: white; font-size: 12px; font-weight: 500;");
    layout->addWidget(nameLabel, 1);

    // Visibility toggle button
    QPushButton *visBtn = new QPushButton(layer->isVisible() ? "👁" : "👁‍🗨");
    visBtn->setFixedSize(28, 28);
    visBtn->setToolTip(layer->isVisible() ? "Hide Layer" : "Show Layer");
    visBtn->setCursor(Qt::PointingHandCursor);
    visBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: none;"
        "   font-size: 16px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a3a3a;"
        "   border-radius: 4px;"
        "}"
        );

    connect(visBtn, &QPushButton::clicked, this, [this, layer, index]() {
        layer->setVisible(!layer->isVisible());
        rebuildLayerList();
        // Force canvas refresh when visibility changes
        emit layer->visibilityChanged(layer->isVisible());
    });
    layout->addWidget(visBtn);

    // Lock toggle button
    QPushButton *lockBtn = new QPushButton(layer->isLocked() ? "🔒" : "🔓");
    lockBtn->setFixedSize(28, 28);
    lockBtn->setToolTip(layer->isLocked() ? "Unlock Layer" : "Lock Layer");
    lockBtn->setCursor(Qt::PointingHandCursor);
    lockBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: none;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a3a3a;"
        "   border-radius: 4px;"
        "}"
        );

    connect(lockBtn, &QPushButton::clicked, this, [this, layer, index]() {
        layer->setLocked(!layer->isLocked());
        rebuildLayerList();
    });
    layout->addWidget(lockBtn);

    // Style based on visibility
    if (!layer->isVisible()) {
        nameLabel->setStyleSheet("color: #666; font-size: 12px; font-weight: 500;");
    }

    return itemWidget;
}

// Rename 'updateLayerList' to 'rebuildLayerList'
void LayerPanel::rebuildLayerList()
{
    m_layerList->clear(); // Only safe to call when NOT triggered by a list item click

    auto layers = m_project->layers();
    for (int i = layers.size() - 1; i >= 0; --i) {
        Layer *layer = layers[i];

        QListWidgetItem *item = new QListWidgetItem(m_layerList);
        item->setSizeHint(QSize(0, 48));
        item->setData(Qt::UserRole, i);

        QWidget *itemWidget = createLayerItem(layer, i);
        m_layerList->setItemWidget(item, itemWidget);
    }

    updateSelection(); // Apply selection after rebuilding
    updateButtons();   // Update button states
}

// NEW FUNCTION: Updates visual state without deleting widgets
void LayerPanel::updateSelection()
{
    // Block signals to prevent infinite loops during selection updates
    const QSignalBlocker blocker(m_layerList);

    Layer* current = m_project->currentLayer();

    for(int i = 0; i < m_layerList->count(); ++i) {
        QListWidgetItem* item = m_layerList->item(i);
        int layerIndex = item->data(Qt::UserRole).toInt();
        Layer* layer = m_project->layerAt(layerIndex);

        if (layer == current) {
            item->setSelected(true);
            // Ensure the item is visible in scroll area
            m_layerList->scrollToItem(item);
        } else {
            item->setSelected(false);
        }
    }

    updateButtons();
}

void LayerPanel::updateButtons() {
    bool hasLayers = m_project->layerCount() > 0;
    bool canDelete = m_project->layerCount() > 1;

    m_duplicateButton->setEnabled(hasLayers);
    m_deleteButton->setEnabled(canDelete);
    m_moveUpButton->setEnabled(hasLayers);
    m_moveDownButton->setEnabled(hasLayers);
}


void LayerPanel::onLayerItemClicked(QListWidgetItem *item)
{
    int index = item->data(Qt::UserRole).toInt();
    m_project->setCurrentLayer(index);
}

void LayerPanel::onAddLayerClicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Layer", "Layer name:",
                                         QLineEdit::Normal,
                                         QString("Layer %1").arg(m_project->layerCount() + 1),
                                         &ok);

    if (ok && !name.isEmpty()) {
        m_undoStack->push(new AddLayerCommand(m_project, name));
    }
}

void LayerPanel::onDeleteLayerClicked()
{
    if (m_project->layerCount() <= 1) {
        QMessageBox::warning(this, "Cannot Delete", "Cannot delete the last layer!");
        return;
    }

    Layer *current = m_project->currentLayer();
    if (!current) return;

    int currentIndex = m_project->layers().indexOf(current);

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Layer",
                                                              QString("Delete layer '%1'?").arg(current->name()),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_undoStack->push(new RemoveLayerCommand(m_project, currentIndex));
    }
}

void LayerPanel::onDuplicateLayerClicked()
{
    Layer *current = m_project->currentLayer();
    if (!current) return;

    m_project->addLayer(current->name() + " Copy");
    // TODO: Copy layer content
}

void LayerPanel::onMoveLayerUp()
{
    Layer *current = m_project->currentLayer();
    if (!current) return;

    int currentIndex = m_project->layers().indexOf(current);
    if (currentIndex < m_project->layerCount() - 1) {
        m_project->moveLayer(currentIndex, currentIndex + 1);
    }
}

void LayerPanel::onMoveLayerDown()
{
    Layer *current = m_project->currentLayer();
    if (!current) return;

    int currentIndex = m_project->layers().indexOf(current);
    if (currentIndex > 0) {
        m_project->moveLayer(currentIndex, currentIndex - 1);
    }
}

void LayerPanel::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_layerList->itemAt(pos);
    if (!item) return;

    int index = item->data(Qt::UserRole).toInt();
    Layer *layer = m_project->layerAt(index);
    if (!layer) return;

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

    QAction *renameAct = menu.addAction("Rename Layer");
    QAction *duplicateAct = menu.addAction("Duplicate Layer");

    menu.addSeparator();

    // Layer type submenu
    QMenu *typeMenu = menu.addMenu("Change Layer Type");
    QAction *artAct = typeMenu->addAction("🎨 Art Layer");
    QAction *bgAct = typeMenu->addAction("🖼️ Background Layer");
    QAction *audioAct = typeMenu->addAction("🎵 Audio Layer");
    QAction *refAct = typeMenu->addAction("📐 Reference Layer");

    // Mark current type
    switch (layer->layerType()) {
    case LayerType::Art: artAct->setCheckable(true); artAct->setChecked(true); break;
    case LayerType::Background: bgAct->setCheckable(true); bgAct->setChecked(true); break;
    case LayerType::Audio: audioAct->setCheckable(true); audioAct->setChecked(true); break;
    case LayerType::Reference: refAct->setCheckable(true); refAct->setChecked(true); break;
    }

    menu.addSeparator();
    QAction *deleteAct = menu.addAction("Delete Layer");

    QAction *selected = menu.exec(m_layerList->mapToGlobal(pos));

    if (selected == renameAct) {
        bool ok;
        QString name = QInputDialog::getText(this, "Rename Layer", "Layer name:",
                                             QLineEdit::Normal, layer->name(), &ok);
        if (ok && !name.isEmpty()) {
            layer->setName(name);
            rebuildLayerList();
        }
    } else if (selected == duplicateAct) {
        onDuplicateLayerClicked();
    } else if (selected == deleteAct) {
        onDeleteLayerClicked();
    } else if (selected == artAct) {
        layer->setLayerType(LayerType::Art);
        rebuildLayerList();
    } else if (selected == bgAct) {
        layer->setLayerType(LayerType::Background);
        rebuildLayerList();
    } else if (selected == audioAct) {
        layer->setLayerType(LayerType::Audio);
        rebuildLayerList();
    } else if (selected == refAct) {
        layer->setLayerType(LayerType::Reference);
        rebuildLayerList();
    }
}
