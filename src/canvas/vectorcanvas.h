#ifndef VECTORCANVAS_H
#define VECTORCANVAS_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QUndoStack>

class Project;
class Tool;
class VectorObject;

class VectorCanvas : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit VectorCanvas(Project *project, QUndoStack *undoStack, QObject *parent = nullptr);
    ~VectorCanvas();

    Project* project() const { return m_project; }
    QUndoStack* undoStack() const { return m_undoStack; }

    // Tool management
    void setCurrentTool(Tool *tool);
    Tool* currentTool() const { return m_currentTool; }

    // Object management
    void addObject(VectorObject *obj);
    void removeObject(VectorObject *obj);
    void clearCurrentFrame();

    // Refresh display
    void refreshFrame();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private slots:
    void setupLayerConnections();
    void onFrameChanged(int frame);

private:
    Project *m_project;
    QUndoStack *m_undoStack;
    Tool *m_currentTool;
    QGraphicsRectItem *m_canvasBounds;
};

#endif // VECTORCANVAS_H
