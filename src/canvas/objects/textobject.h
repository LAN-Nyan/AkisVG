#ifndef TEXTOBJECT_H
#define TEXTOBJECT_H

#include "vectorobject.h"
#include <QString>
#include <QFont>

class TextObject : public VectorObject
{
public:
    explicit TextObject(QGraphicsItem *parent = nullptr);

    VectorObjectType objectType() const override { return VectorObjectType::Text; }

    void setText(const QString &text);
    QString text() const { return m_text; }

    void setFontSize(int size);
    int fontSize() const { return m_fontSize; }

    void setFontFamily(const QString &family);
    QString fontFamily() const { return m_fontFamily; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    QString m_text;
    int m_fontSize;
    QString m_fontFamily;
};

#endif // TEXTOBJECT_H
