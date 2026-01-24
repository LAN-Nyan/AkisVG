#ifndef FRAME_H
#define FRAME_H

#include <QObject>

// Frame class - currently minimal, will expand for keyframes/tweening
class Frame : public QObject
{
    Q_OBJECT

public:
    explicit Frame(int frameNumber, QObject *parent = nullptr);
    
    int frameNumber() const { return m_frameNumber; }
    bool isKeyframe() const { return m_isKeyframe; }
    void setKeyframe(bool keyframe);

private:
    int m_frameNumber;
    bool m_isKeyframe;
};

#endif // FRAME_H
