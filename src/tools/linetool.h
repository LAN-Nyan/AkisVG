#ifndef LINETOOL_H
#define LINETOOL_H

#include "tool.h"
#include <QPointF>

class PathObject;

class LineTool : public Tool
{
    Q_OBJECT

public:
    explicit LineTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    QPointF m_startPoint;
    PathObject *m_currentLine;
};

#endif // LINETOOL_H
