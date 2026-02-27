#include "textobject.h"
#include <QPainter>
#include <QFontMetricsF>

// ──────────────────────────────────────────────────────────
QFont TextObject::buildFont() const
{
    QFont f(m_fontFamily, m_fontSize);
    f.setBold(m_bold);
    f.setItalic(m_italic);
    f.setUnderline(m_underline);
    return f;
}

// ──────────────────────────────────────────────────────────
TextObject::TextObject(QGraphicsItem *parent)
    : VectorObject(parent)
{
}

// ── clone ──────────────────────────────────────────────────
VectorObject* TextObject::clone() const
{
    TextObject *copy = new TextObject();

    // Text-specific
    copy->m_text       = m_text;
    copy->m_fontFamily = m_fontFamily;
    copy->m_fontSize   = m_fontSize;
    copy->m_bold       = m_bold;
    copy->m_italic     = m_italic;
    copy->m_underline  = m_underline;
    copy->m_alignment  = m_alignment;

    // Base VectorObject
    copy->setStrokeColor(strokeColor());
    copy->setFillColor(fillColor());
    copy->setStrokeWidth(strokeWidth());
    copy->setObjectOpacity(objectOpacity());

    // QGraphicsItem transform
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setScale(scale());
    copy->setZValue(zValue());

    return copy;
}

// ── setters ───────────────────────────────────────────────
void TextObject::setText(const QString &text)
{
    prepareGeometryChange();
    m_text = text;
    update();
}

void TextObject::setFontFamily(const QString &family)
{
    prepareGeometryChange();
    m_fontFamily = family;
    update();
}

void TextObject::setFontSize(int size)
{
    prepareGeometryChange();
    m_fontSize = qMax(4, size);
    update();
}

void TextObject::setBold(bool b)
{
    prepareGeometryChange();
    m_bold = b;
    update();
}

void TextObject::setItalic(bool i)
{
    prepareGeometryChange();
    m_italic = i;
    update();
}

void TextObject::setUnderline(bool u)
{
    prepareGeometryChange();
    m_underline = u;
    update();
}

void TextObject::setTextAlignment(Qt::Alignment align)
{
    m_alignment = align;
    update();
}

// ── boundingRect (FIX: use QFontMetricsF for accuracy) ────
QRectF TextObject::boundingRect() const
{
    QFont font = buildFont();
    QFontMetricsF fm(font);

    // Use boundingRect overload that takes a flags+rect to handle
    // multi-line and alignment correctly, with generous padding
    // so glyphs with descenders / ascenders are never clipped.
    QRectF textRect = fm.boundingRect(
        QRectF(0, 0, 4096, 4096),
        Qt::AlignLeft | Qt::TextWordWrap,
        m_text
    );

    return textRect.adjusted(-4, -4, 4, 4);
}

// ── paint (FIX: set font before metrics; render hints on) ──
void TextObject::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing,       true);
    painter->setRenderHint(QPainter::TextAntialiasing,   true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    QFont font = buildFont();
    painter->setFont(font);          // set BEFORE querying metrics

    QFontMetricsF fm(font);
    qreal baseline = fm.ascent();

    // Draw using fill colour (text colour)
    painter->setPen(m_fillColor);
    painter->setBrush(Qt::NoBrush);

    // Determine draw rect from bounding rect
    QRectF br = boundingRect().adjusted(4, 4, -4, -4);

    painter->drawText(
        br,
        static_cast<int>(m_alignment) | Qt::TextWordWrap,
        m_text
    );

    Q_UNUSED(baseline)  // kept to remind future devs why we query it

    // Selection outline
    if (isSelected()) {
        QPen selectPen(QColor(42, 130, 218), 1.5, Qt::DashLine);
        painter->setPen(selectPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}
