#include "textobject.h"
#include <QPainter>
#include <QFontMetrics>

TextObject::TextObject(QGraphicsItem *parent)
    : VectorObject(parent)
    , m_text("Text")
    , m_fontSize(48)
    , m_fontFamily("Arial")
{
}

void TextObject::setText(const QString &text)
{
    prepareGeometryChange();
    m_text = text;
    update();
}

void TextObject::setFontSize(int size)
{
    prepareGeometryChange();
    m_fontSize = size;
    update();
}

void TextObject::setFontFamily(const QString &family)
{
    prepareGeometryChange();
    m_fontFamily = family;
    update();
}

QRectF TextObject::boundingRect() const
{
    QFont font(m_fontFamily, m_fontSize);
    QFontMetrics metrics(font);
    QRectF rect = metrics.boundingRect(m_text);
    return rect.adjusted(-5, -5, 5, 5);
}

void TextObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QFont font(m_fontFamily, m_fontSize);
    painter->setFont(font);

    // Use fill color for text
    painter->setPen(m_fillColor);
    painter->setBrush(Qt::NoBrush);

    painter->drawText(QPointF(0, QFontMetrics(font).ascent()), m_text);

    // Draw selection outline if selected
    if (isSelected()) {
        QPen selectPen(Qt::blue, 1, Qt::DashLine);
        painter->setPen(selectPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}
