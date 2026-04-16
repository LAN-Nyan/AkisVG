#ifndef VECTORCANVAS_H
#define VECTORCANVAS_H

#include <QGraphicsScene>
#include <QUndoStack>
#include <QImage>
#include <QSet>
#include <QMap>
#include "tools/tool.h"

#include <QGraphicsScene>
#include <QUndoStack>
#include <QImage>
#include <QSet>
#include <QMap>
#include "tools/tool.h"
#include "core/layer.h"

class Project;
class VectorObject;
class ObjectGroup;
class QPainter;

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

    // Discard any in-progress stroke — must be called before opening a context
    // menu so the modal event loop cannot accumulate ghost pixels in the scene.

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

#endif // VECTORCANVAS_H
