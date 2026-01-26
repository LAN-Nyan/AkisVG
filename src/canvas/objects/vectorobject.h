#ifndef VECTOROBJECT_H
#define VECTOROBJECT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QColor>
#include <QPixmap>

enum class VectorObjectType {
    Path,
    Rectangle,
    Ellipse,
    Text,
    Image
};

class VectorObject : public QGraphicsItem
{
public:
    explicit VectorObject(QGraphicsItem *parent = nullptr);
    virtual ~VectorObject() override = default;

    // --- Cloning Mechanism ---
    // Every subclass (Path, Rectangle, etc.) must implement this
    virtual VectorObject* clone() const = 0;

    // --- Type Identification ---
    virtual VectorObjectType objectType() const = 0;

    // --- Rendering Helpers ---
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

    // --- QGraphicsItem Interface ---
    QRectF boundingRect() const override = 0;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override = 0;

    // --- Common Properties ---
    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor &color);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &color);

    qreal strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(qreal width);

    qreal objectOpacity() const { return m_objectOpacity; }
    void setObjectOpacity(qreal opacity);

protected:
    QColor m_strokeColor = Qt::black;
    QColor m_fillColor = Qt::transparent;
    qreal m_strokeWidth = 1.0;
    qreal m_objectOpacity = 1.0;
};

#endif // VECTOROBJECT_H
