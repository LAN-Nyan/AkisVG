#ifndef LAYER_H
#define LAYER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QMap>
#include <QList>

class Frame;
class VectorObject;

class Layer : public QObject
{
    Q_OBJECT

public:
    explicit Layer(const QString &name, QObject *parent = nullptr);
    ~Layer();
    
    // Properties
    QString name() const { return m_name; }
    void setName(const QString &name);
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);
    
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked);
    
    QColor color() const { return m_color; }
    void setColor(const QColor &color);
    
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);
    
    // Frame data management
    QList<VectorObject*> objectsAtFrame(int frameNumber) const;
    void addObjectToFrame(int frameNumber, VectorObject *obj);
    void removeObjectFromFrame(int frameNumber, VectorObject *obj);
    void clearFrame(int frameNumber);
    bool hasContentAtFrame(int frameNumber) const;

signals:
    void modified();
    void visibilityChanged(bool visible);
    void lockedChanged(bool locked);

private:
    QString m_name;
    bool m_visible;
    bool m_locked;
    QColor m_color;
    qreal m_opacity;
    
    // Frame data: frameNumber -> list of objects
    QMap<int, QList<VectorObject*>> m_frames;
};

#endif // LAYER_H
