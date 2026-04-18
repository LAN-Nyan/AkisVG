#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QString>
#include <QColor>
#include <QPainterPath>


// Forward declarations
class VectorObject;
class Layer;
// Ensure Project and QSet are available
class Project;
#include <QSet>

class MoveFramesCommand : public QUndoCommand {
public:
    MoveFramesCommand(Project *project, const QSet<int> &frames, int delta, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;
private:
    Project *m_project;
    QSet<int> m_frames;
    int m_delta;
};

class AddFramesCommand : public QUndoCommand {
public:
    AddFramesCommand(Project *project, int count, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;
private:
    Project *m_project;
    int m_count;
};
/**
 * Command for adding a vector object to a layer
 */
class AddObjectCommand : public QUndoCommand
{
public:
    AddObjectCommand(VectorObject *object, Layer *layer, int frame,
                     QUndoCommand *parent = nullptr);
    ~AddObjectCommand() override;

    void undo() override;
    void redo() override;

private:
    VectorObject *m_object;
    Layer *m_layer;
    int m_frame;
    bool m_ownsObject;
};

/**
 * Command for removing a vector object from a layer
 */
class RemoveObjectCommand : public QUndoCommand
{
public:
    RemoveObjectCommand(VectorObject *object, Layer *layer, int frame,
                        QUndoCommand *parent = nullptr);
    ~RemoveObjectCommand() override;

    void undo() override;
    void redo() override;

private:
    VectorObject *m_object;
    Layer *m_layer;
    int m_frame;
    bool m_ownsObject;
};

/**
 * Command for adding a layer
 */
class AddLayerCommand : public QUndoCommand
{
public:
    AddLayerCommand(class Project *project, const QString &name,
                    QUndoCommand *parent = nullptr);
    ~AddLayerCommand() override;

    void undo() override;
    void redo() override;

private:
    class Project *m_project;
    Layer *m_layer;
    QString m_name;
    bool m_ownsLayer;
};

/**
 * Command for removing a layer
 */
class RemoveLayerCommand : public QUndoCommand
{
public:
    RemoveLayerCommand(class Project *project, int layerIndex,
                       QUndoCommand *parent = nullptr);
    ~RemoveLayerCommand() override;

    void undo() override;
    void redo() override;

private:
    class Project *m_project;
    Layer *m_layer;
    int m_layerIndex;
    bool m_ownsLayer;
};

/**
 * Command for changing the fill color of an object
 */
class FillColorCommand : public QUndoCommand
{
public:
    FillColorCommand(VectorObject *object, const QColor &newColor,
                     QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    VectorObject *m_object;
    QColor m_oldColor;
    QColor m_newColor;
};

class PathObject;

/** Undoable in-place path geometry change (lasso cut, path split, etc.). */
class PathObjectPathCommand : public QUndoCommand
{
public:
    PathObjectPathCommand(PathObject *path, const QPainterPath &oldPath,
                          const QPainterPath &newPath, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    PathObject *m_path;
    QPainterPath m_oldPath;
    QPainterPath m_newPath;
};

#endif // COMMANDS_H
