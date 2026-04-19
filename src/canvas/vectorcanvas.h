#ifndef VECTORCANVAS_H
#define VECTORCANVAS_H

#include <QGraphicsScene>
#include <QUndoStack>
#include <QImage>
#include <QSet>
#include <QMap>
#include <QList>
#include <QUuid>
#include "tools/tool.h"
#include "core/layer.h"
#include "canvas/objects/objectgroup.h"  // Include the full definition

// Forward declarations
class Project;
class VectorObject;
class QPainter;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;
class SymbolMaster;
class SymbolInstance;

class VectorCanvas : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit VectorCanvas(Project *project, QUndoStack *undoStack, QObject *parent = nullptr);
    ~VectorCanvas();

    void setCurrentTool(Tool *tool);
    Tool* currentTool() const { return m_currentTool; }

    QUndoStack* undoStack() const { return m_undoStack; }

    void setOnionSkinEnabled(bool enabled);
    bool onionSkinEnabled() const { return m_onionSkinEnabled; }

    QImage currentImage();
    void updateCurrentImage(const QImage &image);

    void addObject(VectorObject *obj);
    void removeObject(VectorObject *obj);
    void clearCurrentFrame();
    void refreshFrame();
    void clearDisplay(); // call before loading a new project
    void cancelLiveDrawing(); // discard in-progress stroke before context menu

    // Batch-update guard: suppresses refreshFrame re-entrancy during multi-step
    // operations (grouping, interpolation). Signals still fire normally so that
    // undo, redo, and layer visibility continue to work correctly.
    void beginBatchUpdate() { m_batchUpdating = true; }
    void endBatchUpdate()   { m_batchUpdating = false; refreshFrame(); }

    ObjectGroup* groupSelectedObjects(const QString &name = QString());
    // Overload: group an explicit list of source objects (used by SelectTool bounding-box selection)
    ObjectGroup* groupObjects(const QList<VectorObject*> &sourceObjects, const QString &name = QString());
    void ungroupSelected();

    // Resolve a display clone (or live item) back to its layer-owned source.
    // Returns obj itself if it is already a source object.
    VectorObject* sourceObject(VectorObject *displayItem) const;

    // Reverse lookup: given a source object, find its current display clone.
    // Returns nullptr if the source has no clone (e.g. it's on a different frame).
    VectorObject* displayCloneFor(VectorObject *source) const;

    // Master Symbols functionality
    SymbolMaster* createMasterSymbol(const QList<VectorObject*> &objects, const QString &name = QString());
    SymbolInstance* createSymbolInstance(SymbolMaster *master);
    void updateSymbolInstances(SymbolMaster *master);
    QList<SymbolMaster*> allMasterSymbols() const;
    void removeMasterSymbol(SymbolMaster *master);

signals:
    // Emitted just before display items are destroyed during refreshFrame().
    // Connect to clear any raw pointers to display clones before they become dangling.
    void aboutToRefreshFrame();
    void referenceImageDropped(const QString &path, const QPointF &position);
    void audioDropped(const QString &path);
    void objectGroupCreated(ObjectGroup *group);
    void contextMenuRequestedAt(const QPoint &globalPos, const QPointF &scenePos);
    void interpolationModeChanged(bool active);

public slots:
    void onFrameChanged(int frame);
    void setupLayerConnections();
    void enterInterpolationMode();
    void exitInterpolationMode();

    // Selection highlight overlays — dashed bounding boxes drawn on top of selected
    // display clones. SelectTool calls showSelectionOverlays() after every selection
    // change. Overlays are cleared at the start of each refreshFrame.
    void showSelectionOverlays(const QList<VectorObject*> &sourceObjects);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QBrush getTextureBrush(ToolTexture texture, const QColor &color);
    void saveCurrentFrameStrokes();
    void connectLayerSignals(Layer *layer);

    Project *m_project;
    QUndoStack *m_undoStack;
    Tool *m_currentTool;
    bool m_onionSkinEnabled;
    bool m_isDrawing;
    bool m_isCancelingDrawing = false;  // Track if we're in the process of canceling a drawing
    QSet<Layer*> m_connectedLayers;
    bool m_isInterpolating = false;
    bool m_batchUpdating   = false;

    // Display clones owned by the canvas. Cleared and rebuilt each refreshFrame.
    QList<VectorObject*> m_displayItems;

    // Maps display clone → layer-owned source object so removeObject/grouping
    // can find the real object even when the scene holds clones.
    QMap<VectorObject*, VectorObject*> m_cloneToSource;

    // The currently-being-drawn object lives in the scene directly (not as a clone)
    // so the user sees strokes in real time. refreshFrame skips it.
    VectorObject* m_liveDrawingItem = nullptr;

    // Dashed bounding-box overlays showing the current bounding-box selection.
    QList<QGraphicsRectItem*> m_selectionOverlays;
};

/**
 * @class SymbolMaster
 * @brief Represents a master symbol that can be instanced across the project
 */
 class SymbolMaster : public ObjectGroup
 {
     Q_OBJECT
 
 public:
     explicit SymbolMaster(const QString &name, QGraphicsItem *parent = nullptr);
     virtual ~SymbolMaster();
 
     QUuid uuid() const { return m_uuid; }
     QList<SymbolInstance*> instances() const { return m_instances; }
 
     void addInstance(SymbolInstance *instance);
     void removeInstance(SymbolInstance *instance);
     void updateAllInstances();
 
     VectorObject* clone() const override;
     QPixmap thumbnail(int size) const;
     void setName(const QString &name);
 
 signals:
     void instanceAdded(SymbolInstance *instance);
     void instanceRemoved(SymbolInstance *instance);
     void masterModified();
 
 private:
     QUuid m_uuid;
     QList<SymbolInstance*> m_instances;
 };
 
 /**
  * @class SymbolInstance
  * @brief Represents an instance of a master symbol
  */
 class SymbolInstance : public ObjectGroup
 {
     Q_OBJECT
 
 public:
     explicit SymbolInstance(SymbolMaster *master, QGraphicsItem *parent = nullptr);
     virtual ~SymbolInstance();
 
     SymbolMaster* master() const { return m_master; }
     void setMaster(SymbolMaster *master);
 
     VectorObject* clone() const override;
     void updateFromMaster();
     QPixmap thumbnail(int size) const;
     void setName(const QString &name);
 
 private:
     SymbolMaster *m_master;
 };

#endif // VECTORCANVAS_H
