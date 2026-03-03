#ifndef OBJECTGROUP_H
#define OBJECTGROUP_H

#include "vectorobject.h"
#include <QList>
#include <QString>

/**
 * ObjectGroup — a named collection of VectorObjects that behaves as a single
 * VectorObject. Every stroke or imported image can be added to a group, enabling:
 *   - unified transforms (move/rotate/scale the whole group)
 *   - instancing (copy the group to any frame or project)
 *   - automatic appearance in the Asset Panel
 */
class ObjectGroup : public VectorObject
{
public:
    explicit ObjectGroup(const QString &name = "Group", QGraphicsItem *parent = nullptr);
    ~ObjectGroup() override;

    VectorObjectType objectType() const override { return VectorObjectType::Group; }
    VectorObject* clone() const override;

    // Group identity
    QString groupName() const { return m_groupName; }
    void setGroupName(const QString &name) { m_groupName = name; }

    // Child management
    void addChild(VectorObject *obj);
    void removeChild(VectorObject *obj);
    QList<VectorObject*> children() const { return m_children; }
    bool isEmpty() const { return m_children.isEmpty(); }
    int childCount() const { return m_children.count(); }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    // Generate a thumbnail pixmap for the Asset Panel
    QPixmap thumbnail(int size = 64) const;

private:
    QString m_groupName;
    QList<VectorObject*> m_children;
};

#endif // OBJECTGROUP_H
