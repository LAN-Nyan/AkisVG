#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QString>

// Forward declarations
class VectorObject;
class Layer;

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

#endif // COMMANDS_H
