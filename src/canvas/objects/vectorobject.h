#ifndef VECTOROBJECT_H
#define VECTOROBJECT_H

#include <QObject>
#include <QGraphicsItem>
#include <QPainter>
#include <QColor>
#include <QPixmap>

enum class VectorObjectType {
    Path,
    Rectangle,
    Ellipse,
    Text,
    Image,
    Group
};

// Use Multiple Inheritance: QObject first, then QGraphicsItem
class VectorObject : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit VectorObject(QGraphicsItem *parent = nullptr);
    virtual ~VectorObject() override = default;

    virtual VectorObject* clone() const = 0;
    virtual VectorObjectType objectType() const = 0;

    virtual QPixmap toPixmap() {
        QRectF rect = boundingRect();
        if (rect.isEmpty()) return QPixmap();
        QPixmap pixmap(rect.size().toSize());
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-rect.topLeft());
        paint(&painter, nullptr, nullptr);
        return pixmap;
    }

    QRectF boundingRect() const override = 0;

    virtual void moveBy(qreal dx, qreal dy) {
        setPos(pos() + QPointF(dx, dy));
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override = 0;

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor &color) { m_strokeColor = color; }

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &color) { m_fillColor = color; }

    qreal strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(qreal width) { m_strokeWidth = width; }

    qreal objectOpacity() const { return m_objectOpacity; }
    void setObjectOpacity(qreal opacity) { m_objectOpacity = opacity; }

protected:
    QColor m_strokeColor = Qt::black;
    QColor m_fillColor = Qt::transparent;
    qreal m_strokeWidth = 1.0;
    qreal m_objectOpacity = 1.0;
};

#endif // VECTOROBJECT_H