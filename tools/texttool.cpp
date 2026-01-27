#include "texttool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/textobject.h"

#include <QInputDialog>

TextTool::TextTool(QObject *parent)
    : Tool(ToolType::Text, parent)
{
}

void TextTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);

    // Show text input dialog
    bool ok;
    QString text = QInputDialog::getText(nullptr, "Insert Text", "Enter text:",
                                         QLineEdit::Normal, "Text", &ok);

    if (ok && !text.isEmpty()) {
        TextObject *textObj = new TextObject();
        textObj->setText(text);
        textObj->setPos(event->scenePos());
        textObj->setStrokeColor(m_strokeColor);
        textObj->setFillColor(m_strokeColor); // Text uses stroke color as fill
        textObj->setFontSize(48); // Default size

        canvas->addObject(textObj);
    }
}

void TextTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    // Text tool doesn't need move handling
}

void TextTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    Tool::mouseReleaseEvent(event, canvas);
}
