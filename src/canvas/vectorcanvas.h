#ifndef VECTORCANVAS_H
#define VECTORCANVAS_H

#include <QGraphicsScene>
#include <QUndoStack>
#include <QImage>
#include <QSet>
#include "tools/tool.h"
#include "core/layer.h"

// Forward declarations
class Project;
class VectorObject;
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

    // Interpolation mode support
    void setInterpolationMode(bool enabled);
    bool isInInterpolationMode() const { return m_interpolationMode; }

signals:
    void referenceImageDropped(const QString &path, const QPointF &position);
    void audioDropped(const QString &path);

public slots:
    void onFrameChanged(int frame);
    void setupLayerConnections();

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
    bool m_interpolationMode;
    QSet<Layer*> m_connectedLayers;  // Track which layers are already connected
};

#endif // VECTORCANVAS_H
