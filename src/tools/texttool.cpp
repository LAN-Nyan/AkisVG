#include "texttool.h"
#include "canvas/vectorcanvas.h"
#include "canvas/objects/textobject.h"

#include <QInputDialog>
#include <QGraphicsSceneMouseEvent>

TextTool::TextTool(QObject *parent)
    : Tool(ToolType::Text, parent)
{
}

// ──────────────────────────────────────────────────────────
void TextTool::mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Tool::mousePressEvent(event, canvas);

    // Show a simple text input dialog
    bool ok = false;
    QString text = QInputDialog::getText(
        nullptr,
        "Insert Text",
        "Enter text:",
        QLineEdit::Normal,
        "Text",
        &ok
    );

    if (ok && !text.isEmpty()) {
        TextObject *obj = new TextObject();
        obj->setText(text);
        obj->setPos(event->scenePos());

        // Apply stored font settings
        obj->setFontFamily(m_fontFamily);
        obj->setFontSize(m_fontSize);
        obj->setBold(m_bold);
        obj->setItalic(m_italic);
        obj->setUnderline(m_underline);
        obj->setTextAlignment(m_alignment);

        // Colour comes from the tool's current stroke colour
        obj->setFillColor(m_strokeColor);
        obj->setStrokeColor(m_strokeColor);

        canvas->addObject(obj);
    }
}

void TextTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
}

void TextTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas)
{
    Q_UNUSED(event)
    Q_UNUSED(canvas)
    Tool::mouseReleaseEvent(event, canvas);
}

// ──────────────────────────────────────────────────── slots
void TextTool::setFontFamily(const QString &family)
{
    m_fontFamily = family;
    emit fontSettingsChanged();
}

void TextTool::setFontSize(int size)
{
    m_fontSize = qMax(4, size);
    emit fontSettingsChanged();
}

void TextTool::setBold(bool b)
{
    m_bold = b;
    emit fontSettingsChanged();
}

void TextTool::setItalic(bool i)
{
    m_italic = i;
    emit fontSettingsChanged();
}

void TextTool::setUnderline(bool u)
{
    m_underline = u;
    emit fontSettingsChanged();
}

void TextTool::setAlignment(Qt::Alignment align)
{
    m_alignment = align;
    emit fontSettingsChanged();
}
