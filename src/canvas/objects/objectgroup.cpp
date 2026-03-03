#include "objectgroup.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

ObjectGroup::ObjectGroup(const QString &name, QGraphicsItem *parent)
    : VectorObject(parent)
    , m_groupName(name)
{
    // Groups are selectable and movable as a unit
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

ObjectGroup::~ObjectGroup()
{
    // Children are owned by the group — delete them on destruction
    qDeleteAll(m_children);
    m_children.clear();
}

void ObjectGroup::addChild(VectorObject *obj)
{
    if (!obj || m_children.contains(obj)) return;
    m_children.append(obj);
    // Re-parent so the child renders relative to this group item
    obj->setParentItem(this);
    prepareGeometryChange();
    update();
}

void ObjectGroup::removeChild(VectorObject *obj)
{
    if (!obj) return;
    m_children.removeOne(obj);
    if (obj->parentItem() == this) {
        obj->setParentItem(nullptr);
    }
    prepareGeometryChange();
    update();
}

QRectF ObjectGroup::boundingRect() const
{
    if (m_children.isEmpty()) return QRectF(0, 0, 1, 1);

    QRectF united;
    for (VectorObject *child : m_children) {
        QRectF childRect = child->mapRectToParent(child->boundingRect());
        if (united.isNull()) united = childRect;
        else united = united.united(childRect);
    }
    return united.adjusted(-2, -2, 2, 2);
}

void ObjectGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    // Children paint themselves via QGraphicsItem parenting.
    // We just draw the selection outline when selected.
    if (option && (option->state & QStyle::State_Selected)) {
        painter->setPen(QPen(QColor(42, 130, 218), 1.5, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
    }
}

VectorObject* ObjectGroup::clone() const
{
    ObjectGroup *copy = new ObjectGroup(m_groupName);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setScale(scale());
    copy->setObjectOpacity(m_objectOpacity);

    for (VectorObject *child : m_children) {
        VectorObject *childCopy = child->clone();
        copy->addChild(childCopy);
    }
    return copy;
}

QPixmap ObjectGroup::thumbnail(int size) const
{
    QRectF rect = boundingRect();
    if (rect.isEmpty()) return QPixmap(size, size);

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    // Scale to fit
    qreal scaleX = size / rect.width();
    qreal scaleY = size / rect.height();
    qreal s = qMin(scaleX, scaleY) * 0.9;
    qreal ox = (size - rect.width() * s) / 2.0;
    qreal oy = (size - rect.height() * s) / 2.0;
    p.translate(ox - rect.left() * s, oy - rect.top() * s);
    p.scale(s, s);

    QStyleOptionGraphicsItem opt;
    for (VectorObject *child : m_children) {
        p.save();
        p.translate(child->pos());
        child->paint(&p, &opt, nullptr);
        p.restore();
    }
    return pixmap;
}
