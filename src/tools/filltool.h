#ifndef FILLTOOL_H
#define FILLTOOL_H

#include "tool.h" // Assuming your base class is tool.h
#include <QColor>
#include <QPoint>

class FillTool : public Tool
{
public:
    explicit FillTool(QObject *parent = nullptr);

    // This is the main trigger when the user clicks the canvas
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

private:
    // The standard flood-fill algorithm logic
    void floodFill(QImage &image, const QPoint &startPoint, const QColor &fillColor);
};

#endif // FILLTOOL_H
