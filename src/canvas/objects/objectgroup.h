#ifndef OBJECTGROUP_H
#define OBJECTGROUP_H

#include "vectorobject.h"
#include <QList>
#include <QString>

class ObjectGroup : public VectorObject
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit ObjectGroup(const QString &name = "Group", QGraphicsItem *parent = nullptr);
    ~ObjectGroup() override;

    VectorObjectType objectType() const override { return VectorObjectType::Group; }
    VectorObject* clone() const override;

    QString groupName() const { return m_groupName; }
    void setGroupName(const QString &name) { m_groupName = name; }

    void addChild(VectorObject *obj);
    void removeChild(VectorObject *obj);
    QList<VectorObject*> children() const { return m_children; }
    bool isEmpty() const { return m_children.isEmpty(); }
    int childCount() const { return m_children.count(); }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    QPixmap thumbnail(int size = 64) const;

private:
    QString m_groupName;
    QList<VectorObject*> m_children;
};

#endif // OBJECTGROUP_H