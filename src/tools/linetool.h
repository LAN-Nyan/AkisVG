#ifndef LINETOOL_H
#define LINETOOL_H

#include "tool.h"
#include <QPointF>
#include "canvas/objects/pathobject.h"

class LineTool : public Tool
{
    Q_OBJECT

public:
    explicit LineTool(QObject *parent = nullptr);

    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    // FIX #19: Expose dash style and arrow settings
    void setLineDashStyle(PathDashStyle style) { m_lineDashStyle = style; }
    PathDashStyle lineDashStyle() const { return m_lineDashStyle; }

    void setLineArrowAtEnd(bool arrow) { m_lineArrowAtEnd = arrow; }
    bool lineArrowAtEnd() const { return m_lineArrowAtEnd; }

private:
    QPointF m_startPoint;
    PathObject *m_currentLine;
    PathDashStyle m_lineDashStyle = PathDashStyle::Solid;
    bool m_lineArrowAtEnd = false;
};

#endif // LINETOOL_H
