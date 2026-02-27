#ifndef EYEDROPPERTOOL_H
#define EYEDROPPERTOOL_H

#include "tool.h"
#include <QColor>

class EyedropperTool : public Tool
{
    Q_OBJECT
public:
    explicit EyedropperTool(QObject *parent = nullptr);

    // We override these to sample colors
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

signals:
    // Signal to tell the UI to update its color buttons
    void colorPicked(const QColor &stroke, const QColor &fill);

private:
    QColor sampleAt(const QPointF &pos, VectorCanvas *canvas, bool getStroke);
};

#endif
