#ifndef FRAME_H
#define FRAME_H

#include <QObject>
#include <QList>
#include <QPainterPath>
#include <QColor>
#include "tools/tool.h"

// Forward declaration
class VectorObject;

struct VectorStroke {
    QPainterPath path;
    QColor color;
    qreal width;
    ToolTexture texture;
};

class Frame : public QObject
{
    Q_OBJECT

public:
    explicit Frame(int frameNumber, QObject *parent = nullptr);

    int frameNumber() const { return m_frameNumber; }
    bool isKeyframe() const { return m_isKeyframe; }
    void setKeyframe(bool keyframe);

    // Hold duration (for frame holds in animation)
    int holdDuration() const { return m_holdDuration; }
    void setHoldDuration(int duration) { m_holdDuration = duration; }

    // Frame content
    QList<VectorObject*> objects;
    QList<VectorStroke> strokes;

private:
    int m_frameNumber;
    bool m_isKeyframe;
    int m_holdDuration;  // How many frames to hold this frame
};

#endif // FRAME_H
