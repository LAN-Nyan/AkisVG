#include "commands.h"
#include "layer.h"
#include "project.h"
#include "canvas/objects/vectorobject.h"

// ddObjectCommand

AddObjectCommand::AddObjectCommand(VectorObject *object, Layer *layer, int frame,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_object(object)
    , m_layer(layer)
    , m_frame(frame)
    , m_ownsObject(false)
{
    setText("Add Object");
}

AddObjectCommand::~AddObjectCommand()
{
    if (m_ownsObject) {
        delete m_object;
    }
}

void AddObjectCommand::undo()
{
    m_layer->removeObjectFromFrame(m_frame, m_object);
    m_ownsObject = true;
}

void AddObjectCommand::redo()
{
    m_layer->addObjectToFrame(m_frame, m_object);
    m_ownsObject = false;
}

// RemoveObjectCommand

RemoveObjectCommand::RemoveObjectCommand(VectorObject *object, Layer *layer, int frame,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_object(object)
    , m_layer(layer)
    , m_frame(frame)
    , m_ownsObject(false)
{
    setText("Remove Object");
}

RemoveObjectCommand::~RemoveObjectCommand()
{
    if (m_ownsObject) {
        delete m_object;
    }
}

void RemoveObjectCommand::undo()
{
    m_layer->addObjectToFrame(m_frame, m_object);
    m_ownsObject = false;
}

void RemoveObjectCommand::redo()
{
    m_layer->removeObjectFromFrame(m_frame, m_object);
    m_ownsObject = true;
}

// ============= AddLayerCommand =============

AddLayerCommand::AddLayerCommand(Project *project, const QString &name,
                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_project(project)
    , m_layer(nullptr) // We store the pointer to identify it later
    , m_name(name)
{
    setText("Add Layer");
}

AddLayerCommand::~AddLayerCommand()
{
    // If the layer is NOT in the project (meaning it was undone),
    // we must delete it to prevent a memory leak.
    if (m_layer && !m_project->layers().contains(m_layer)) {
        delete m_layer;
    }
}

void AddLayerCommand::redo()
{
    if (!m_project) return; // Critical safety check

    if (!m_layer) {
        m_layer = new Layer(m_name, m_project);
    }

    m_project->addLayerSilent(m_layer);
    m_layer->setParent(m_project);

    // This is safer than calling emit directly inside the project
    QMetaObject::invokeMethod(m_project, "layersChanged", Qt::QueuedConnection);
}

void AddLayerCommand::undo()
{
    int index = m_project->layers().indexOf(m_layer);
    if (index >= 0) {
        m_project->removeLayerSilent(index);
        m_ownsLayer = true;

        // Tell the UI it's gone
        emit m_project->layersChanged();
    }
}

// ============= RemoveLayerCommand =============

RemoveLayerCommand::RemoveLayerCommand(Project *project, int layerIndex,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_project(project)
    , m_layer(nullptr)
    , m_layerIndex(layerIndex)
    , m_ownsLayer(false)
{
    setText("Remove Layer");
    m_layer = project->layerAt(layerIndex);
}

RemoveLayerCommand::~RemoveLayerCommand()
{
    if (m_ownsLayer) {
        delete m_layer;
    }
}

void RemoveLayerCommand::undo()
{
    m_project->insertLayerSilent(m_layerIndex, m_layer);
    m_ownsLayer = false;
}

void RemoveLayerCommand::redo()
{
    m_project->removeLayerSilent(m_layerIndex);
    m_ownsLayer = true;
}

// ============= FillColorCommand =============

FillColorCommand::FillColorCommand(VectorObject *object, const QColor &newColor,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_object(object)
    , m_oldColor(object->fillColor())
    , m_newColor(newColor)
{
    setText("Fill Color");
}

void FillColorCommand::undo()
{
    m_object->setFillColor(m_oldColor);
}

void FillColorCommand::redo()
{
    m_object->setFillColor(m_newColor);
}
