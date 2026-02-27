#ifndef TEXTOBJECT_H
#define TEXTOBJECT_H

#include "vectorobject.h"
#include <QString>
#include <QFont>

// =========================================================
//  TextObject
//  A VectorObject that renders a string using QPainter.
//  Supports: font family, size, bold, italic, underline,
//            text alignment, and selection outline.
//
//  RENDERING FIX:
//    – Uses QFontMetricsF (floating-point) for accurate
//      bounding-rect calculations, which prevents clipping.
//    – Passes the font to the painter BEFORE any metric
//      queries to ensure consistent results across DPI.
// =========================================================
class TextObject : public VectorObject
{
public:
    explicit TextObject(QGraphicsItem *parent = nullptr);
    virtual VectorObject* clone() const override;

    VectorObjectType objectType() const override { return VectorObjectType::Text; }

    // ── Text content ──────────────────────────────────────
    void    setText(const QString &text);
    QString text()  const { return m_text; }

    // ── Font properties ───────────────────────────────────
    void    setFontFamily(const QString &family);
    QString fontFamily()  const { return m_fontFamily; }

    void setFontSize(int size);
    int  fontSize()       const { return m_fontSize; }

    void setBold(bool b);
    bool bold()           const { return m_bold; }

    void setItalic(bool i);
    bool italic()         const { return m_italic; }

    void setUnderline(bool u);
    bool underline()      const { return m_underline; }

    void          setTextAlignment(Qt::Alignment align);
    Qt::Alignment textAlignment()   const { return m_alignment; }

    // ── QGraphicsItem interface ───────────────────────────
    QRectF boundingRect() const override;
    void   paint(QPainter *painter,
                 const QStyleOptionGraphicsItem *option,
                 QWidget *widget = nullptr) override;

private:
    QFont buildFont() const;

    QString       m_text       = "Text";
    QString       m_fontFamily = "Arial";
    int           m_fontSize   = 48;
    bool          m_bold       = false;
    bool          m_italic     = false;
    bool          m_underline  = false;
    Qt::Alignment m_alignment  = Qt::AlignLeft;
};

#endif // TEXTOBJECT_H
