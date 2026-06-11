#ifndef SYMBOLEDITORDIALOG_H
#define SYMBOLEDITORDIALOG_H

#include <QDialog>

class Project;
class VectorCanvas;
class CanvasView;
class ToolBox;
class SymbolMaster;
class QUndoStack;

/**
 * Modal editor for a SymbolMaster: isolated canvas + tools; changes propagate to all instances on accept.
 */
class SymbolEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolEditorDialog(SymbolMaster *master, Project *dimensionSource, QWidget *parent = nullptr);

protected:
    void accept() override;
    void reject() override;

private:
    void discardUnsaved();

    SymbolMaster *m_master = nullptr;
    Project *m_proj = nullptr;
    VectorCanvas *m_canvas = nullptr;
    CanvasView *m_view = nullptr;
    ToolBox *m_tools = nullptr;
    QUndoStack *m_undo = nullptr;
};

#endif
