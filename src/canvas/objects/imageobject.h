#ifndef IMAGEOBJECT_H
#define IMAGEOBJECT_H

#include "vectorobject.h"
#include <QPixmap>
#include <QString>

class ImageObject : public VectorObject
{
public:
    explicit ImageObject(QGraphicsItem *parent = nullptr);

    VectorObjectType objectType() const override { return VectorObjectType::Image; }

    void setImage(const QPixmap &pixmap);
    void setImagePath(const QString &path);
    QPixmap image() const { return m_pixmap; }

    void setSize(const QSizeF &size);
    QSizeF size() const { return m_size; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    QPixmap m_pixmap;
    QString m_imagePath;
    QSizeF m_size;
};

#endif // IMAGEOBJECT_H
