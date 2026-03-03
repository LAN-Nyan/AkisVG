#include "layerpanel.h"
#include "core/project.h"
#include "core/layer.h"
#include "core/commands.h"
#include "utils/thememanager.h"

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

LayerPanel::LayerPanel(Project *project, QUndoStack *undoStack, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_undoStack(undoStack)
{
    if (!m_undoStack) {
        qWarning("LayerPanel received null UndoStack! App will crash on action.");
    }

    setAcceptDrops(true); // Enable drag and drop for audio files

    setupUI();
    rebuildLayerList();

    connect(m_project, &Project::layersChanged, this, &LayerPanel::rebuildLayerList, Qt::QueuedConnection);
    connect(m_project, &Project::currentLayerChanged, this, &LayerPanel::updateSelection);
}

void LayerPanel::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-lumina-asset")) {
        event->acceptProposedAction();
    } else if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void LayerPanel::dropEvent(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();

    // Handle drag from asset library
    if (mimeData->hasFormat("application/x-lumina-asset")) {
        QString typeStr = mimeData->data("application/x-lumina-asset-type");
        QString filePath = QString::fromUtf8(mimeData->data("application/x-lumina-asset-path"));

        int assetType = typeStr.toInt();
        if (assetType == 1) {  // Audio asset
            // TODO: Attach audio to layer
            qDebug() << "Audio dropped on layer panel:" << filePath;
            event->acceptProposedAction();
        }
    }
    // Handle direct file drop
    else if (mimeData->hasUrls()) {
        QString filePath = mimeData->urls().at(0).toLocalFile();

        if (filePath.endsWith(".mp3", Qt::CaseInsensitive) ||
            filePath.endsWith(".wav", Qt::CaseInsensitive) ||
            filePath.endsWith(".ogg", Qt::CaseInsensitive)) {

            // TODO: Attach audio to current layer
            qDebug() << "Audio file dropped:" << filePath;
            event->acceptProposedAction();
        }
    }
}

void LayerPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    m_header = new QWidget();
    m_header->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #000;");
    m_header->setFixedHeight(40);

    QHBoxLayout *headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel *title = new QLabel("LAYERS");
    title->setStyleSheet("color: white; font-weight: bold; font-size: 11px; letter-spacing: 1px;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_addButton = new QPushButton("+");
    m_addButton->setFixedSize(28, 28);
    m_addButton->setToolTip("Add Layer");
    m_addButton->setCursor(Qt::PointingHandCursor);
    m_addButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #c0392b;"
        "   border: none;"
        "   border-radius: 4px;"
        "   color: white;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e05241;"
        "}"
        );
    connect(m_addButton, &QPushButton::clicked, this, &LayerPanel::onAddLayerClicked);
    headerLayout->addWidget(m_addButton);

    mainLayout->addWidget(m_header);

    // Layer list
    m_layerList = new QListWidget();
    m_layerList->setStyleSheet(
        "QListWidget {"
        "   background-color: #1e1e1e;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   border-bottom: 1px solid #1a1a1a;"
        "   padding: 0;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(192, 57, 43, 0.25);"
        "}"
        "QScrollBar:vertical {"
        "   background: #1a1a1a; width: 8px; border-radius: 4px; margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #444; border-radius: 4px; min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #c0392b; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        );
    m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_layerList, &QListWidget::itemClicked, this, &LayerPanel::onLayerItemClicked);
    connect(m_layerList, &QListWidget::customContextMenuRequested, this, &LayerPanel::showContextMenu);

    mainLayout->addWidget(m_layerList);

    // Bottom toolbar
    m_toolbar = new QWidget();
    m_toolbar->setStyleSheet("background-color: #1e1e1e; border-top: 1px solid #000;");
    m_toolbar->setFixedHeight(44);

    QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(8, 6, 8, 6);
    toolbarLayout->setSpacing(6);

    auto createToolButton = [](const QString &text, const QString &tooltip) {
        QPushButton *btn = new QPushButton(text);
        btn->setFixedSize(32, 32);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: #1e1e1e;"
            "   border: 1px solid #555;"
            "   border-radius: 4px;"
            "   color: #ccc;"
            "   font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #1e1e1e;"
            "   border-color: #c0392b;"
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

    mainLayout->addWidget(m_toolbar);
}

QWidget* LayerPanel::createLayerItem(Layer *layer, int index)
{
    Q_UNUSED(index); // Clean up the compiler warning you saw earlier
    QWidget *itemWidget = new QWidget();
    itemWidget->setFixedHeight(54); // Increased slightly for two lines of text

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(10);

    // 1. Color Indicator (Keep your existing logic)
    QLabel *colorLabel = new QLabel();
    colorLabel->setFixedSize(4, 38);
    QString layerColor;
    QString typeLabelText;

    switch(layer->layerType()) {
    case LayerType::Audio:
        layerColor = "#27ae60";
        typeLabelText = "AUDIO";
        break;
    case LayerType::Art:
        layerColor = "#c0392b";
        typeLabelText = "ART";
        break;
    case LayerType::Reference:
        layerColor = "#e67e22";
        typeLabelText = "REFERENCE";
        break;
    case LayerType::Background:
        layerColor = "#555555";
        typeLabelText = "BACKGROUND";
        break;
    default:
        layerColor = "#c0392b";
        typeLabelText = "LAYER";
        break;
    }

    colorLabel->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(layerColor));
    layout->addWidget(colorLabel);

    // 2. TEXT CONTAINER (Stacking Name and Type)
    QWidget *textContainer = new QWidget();
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(layer->name());
    nameLabel->setStyleSheet("color: white; font-size: 12px; font-weight: bold;");

    QLabel *typeLabel = new QLabel(typeLabelText);
    typeLabel->setStyleSheet("color: #888; font-size: 9px; font-weight: bold; letter-spacing: 0.5px;");

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(typeLabel);
    layout->addWidget(textContainer, 1);

    // 3. Visibility Button (Keep your existing logic)
    QPushButton *visBtn = new QPushButton(layer->isVisible() ? "👁" : "◯");
    visBtn->setFixedSize(24, 24);
    visBtn->setStyleSheet("QPushButton { background: transparent; color: #aaa; border: none; font-size: 14px; } "
                          "QPushButton:hover { background: #444; border-radius: 4px; }");
    connect(visBtn, &QPushButton::clicked, this, [this, layer]() {
        layer->setVisible(!layer->isVisible());
        rebuildLayerList();
    });
    layout->addWidget(visBtn);

    // 4. Lock Button (Keep your existing logic)
    QPushButton *lockBtn = new QPushButton(layer->isLocked() ? "🔒" : "🔓");
    lockBtn->setFixedSize(24, 24);
    visBtn->setStyleSheet("QPushButton { background: transparent; color: #aaa; border: none; font-size: 12px; } "
                          "QPushButton:hover { background: #444; border-radius: 4px; }");
    connect(lockBtn, &QPushButton::clicked, this, [this, layer]() {
        layer->setLocked(!layer->isLocked());
        rebuildLayerList();
    });
    layout->addWidget(lockBtn);

    // Gray out text if hidden
    if (!layer->isVisible()) {
        nameLabel->setStyleSheet("color: #555; font-size: 12px;");
        typeLabel->setStyleSheet("color: #444; font-size: 9px;");
    }

    return itemWidget;
}

void LayerPanel::rebuildLayerList()
{
    m_layerList->clear();

    auto layers = m_project->layers();
    for (int i = layers.size() - 1; i >= 0; --i) {
        Layer *layer = layers[i];

        QListWidgetItem *item = new QListWidgetItem(m_layerList);
        item->setSizeHint(QSize(0, 48));
        item->setData(Qt::UserRole, i);

        QWidget *itemWidget = createLayerItem(layer, i);
        m_layerList->setItemWidget(item, itemWidget);
    }

    updateSelection();
    updateButtons();
}

void LayerPanel::updateSelection()
{
    const QSignalBlocker blocker(m_layerList);
    Layer* current = m_project->currentLayer();

    for(int i = 0; i < m_layerList->count(); ++i) {
        QListWidgetItem* item = m_layerList->item(i);
        int layerIndex = item->data(Qt::UserRole).toInt();
        Layer* layer = m_project->layerAt(layerIndex);

        if (layer == current) {
            item->setSelected(true);
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
        "   background-color: #c0392b;"
        "}"
        );

    QAction *renameAct = menu.addAction("Rename Layer");
    QAction *duplicateAct = menu.addAction("Duplicate Layer");

    menu.addSeparator();

    QMenu *typeMenu = menu.addMenu("Change Layer Type");
    QAction *artAct = typeMenu->addAction("🎨 Art Layer");
    QAction *bgAct = typeMenu->addAction("🖼️ Background Layer");
    QAction *audioAct = typeMenu->addAction("🎵 Audio Layer");
    QAction *refAct = typeMenu->addAction("📐 Reference Layer");

    switch (layer->layerType()) {
    case LayerType::Art: artAct->setCheckable(true); artAct->setChecked(true); break;
    case LayerType::Background: bgAct->setCheckable(true); bgAct->setChecked(true); break;
    case LayerType::Audio: audioAct->setCheckable(true); audioAct->setChecked(true); break;
    case LayerType::Reference: refAct->setCheckable(true); refAct->setChecked(true); break;
    case LayerType::Interpolation: break; // Interpolation layers use Art visually
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

void LayerPanel::applyTheme()
{
    const ThemeColors &t = theme();

    m_header->setStyleSheet(
        QString("background-color: %1; border-bottom: 1px solid #000;").arg(t.bg0));

    m_addButton->setStyleSheet(
        QString("QPushButton { background-color: %1; border: none; border-radius: 4px;"
                " color: white; font-size: 18px; font-weight: bold; }"
                "QPushButton:hover { background-color: %2; }").arg(t.accent, t.accentHover));

    m_layerList->setStyleSheet(
        QString("QListWidget { background-color: %1; border: none; outline: none; }"
                "QListWidget::item { border-bottom: 1px solid %2; padding: 0; }"
                "QListWidget::item:selected { background-color: rgba(%3,%4,%5,60); }"
                "QScrollBar:vertical { background: %2; width: 8px; border-radius: 4px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #444; border-radius: 4px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: %6; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
        .arg(t.bg0, t.bg1)
        .arg(t.accentColor().red()).arg(t.accentColor().green()).arg(t.accentColor().blue())
        .arg(t.accent));

    m_toolbar->setStyleSheet(
        QString("background-color: %1; border-top: 1px solid #000;").arg(t.bg0));

    QString toolBtnStyle = QString(
        "QPushButton { background-color: %1; border: 1px solid #555; border-radius: 4px;"
        " color: #ccc; font-size: 14px; }"
        "QPushButton:hover { background-color: %1; border-color: %2; color: white; }"
        "QPushButton:disabled { color: #555; border-color: #333; }")
        .arg(t.bg0, t.accent);

    m_deleteButton->setStyleSheet(toolBtnStyle);
    m_duplicateButton->setStyleSheet(toolBtnStyle);
    m_moveUpButton->setStyleSheet(toolBtnStyle);
    m_moveDownButton->setStyleSheet(toolBtnStyle);

    // Rebuild list so layer color indicators refresh too
    rebuildLayerList();
}
