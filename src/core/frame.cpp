#include "frame.h"

Frame::Frame(int frameNumber, QObject *parent)
    : QObject(parent)
    , m_frameNumber(frameNumber)
    , m_isKeyframe(false)
{
}

void Frame::setKeyframe(bool keyframe)
{
    m_isKeyframe = keyframe;
}
