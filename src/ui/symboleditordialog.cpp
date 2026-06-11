#include "symboleditordialog.h"
#include "core/project.h"
#include "core/layer.h"
#include "canvas/vectorcanvas.h"
#include "canvas/canvasview.h"
#include "canvas/objects/vectorobject.h"
#include "panels/toolbox.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QKeySequence>

SymbolEditorDialog::SymbolEditorDialog(SymbolMaster *master, Project *dimensionSource, QWidget *parent)
    : QDialog(parent)
    , m_master(master)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(tr("Master Symbol — %1").arg(master ? master->groupName() : QString()));
    resize(960, 640);
    setModal(true);

    if (!m_master || !dimensionSource) {
        return;
    }

    m_undo = new QUndoStack(this);
    m_proj = new Project(this);
    m_proj->createNew(dimensionSource->width(), dimensionSource->height(), dimensionSource->fps());
    m_proj->setCurrentFrame(1);

    m_canvas = new VectorCanvas(m_proj, m_undo, this);
    m_view = new CanvasView(m_canvas, this);
    m_view->setAcceptDrops(true);
    m_tools = new ToolBox(this);

    connect(m_tools, &ToolBox::toolChanged, m_canvas, &VectorCanvas::setCurrentTool);
    m_canvas->setCurrentTool(m_tools->currentTool());

    Layer *layer = m_proj->currentLayer();
    for (VectorObject *c : m_master->children()) {
        if (!c) continue;
        VectorObject *copy = c->clone();
        layer->addObjectToFrame(1, copy);
    }
    m_canvas->refreshFrame();
    m_view->fitCanvas();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *lay = new QVBoxLayout(this);
    auto *top = new QHBoxLayout();
    top->addWidget(m_tools, 0);
    top->addWidget(m_view, 1);
    lay->addLayout(top, 1);
    lay->addWidget(buttons);

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(reject()));
}

void SymbolEditorDialog::discardUnsaved()
{
    if (!m_proj || !m_canvas) return;
    Layer *layer = m_proj->currentLayer();
    if (!layer) return;
    layer->clearFrame(1);
}

void SymbolEditorDialog::reject()
{
    discardUnsaved();
    QDialog::reject();
}

void SymbolEditorDialog::accept()
{
    if (!m_master || !m_proj) {
        QDialog::accept();
        return;
    }

    Layer *layer = m_proj->currentLayer();
    if (!layer) {
        QDialog::accept();
        return;
    }

    QList<VectorObject *> objs = layer->objectsAtFrame(1);

    QRectF united;
    for (VectorObject *o : objs) {
        if (!o) continue;
        QRectF r = o->mapRectToScene(o->boundingRect());
        united = united.isNull() ? r : united.united(r);
    }
    if (united.isNull()) {
        m_master->setArtworkChildren({});
        discardUnsaved();
        QDialog::accept();
        return;
    }

    QList<VectorObject *> forMaster;
    for (VectorObject *o : objs) {
        if (!o) continue;
        layer->removeObjectFromFrame(1, o);
        o->setPos(o->scenePos() - united.topLeft());
        forMaster.append(o);
    }
    m_master->setArtworkChildren(forMaster);

    discardUnsaved();
    QDialog::accept();
}
