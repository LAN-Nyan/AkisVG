#ifndef IMAGEOBJECT_H
#define IMAGEOBJECT_H

#include "vectorobject.h"
#include <QPixmap>
#include <QString>
#include <QGraphicsSceneMouseEvent>

class ImageObject : public VectorObject
{
public:
    explicit ImageObject(QGraphicsItem *parent = nullptr);

    VectorObject* clone() const override;
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

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QPixmap m_pixmap;
    QString m_imagePath;
    QSizeF  m_size;
    bool    m_resizing     = false;
    int     m_resizeCorner = 0;   // 0=TL 1=TR 2=BL 3=BR
    QPointF m_pressScenePos;
    QSizeF  m_pressSize;
};

#endif // IMAGEOBJECT_H
