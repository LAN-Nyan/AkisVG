#include "vectorobject.h"

VectorObject::VectorObject(QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_strokeColor(Qt::black)
    , m_fillColor(Qt::transparent)
    , m_strokeWidth(2.0)
    , m_objectOpacity(1.0)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    // PERFORMANCE: Enable caching for static objects
    // This caches the rendered item in device coordinates
    // Comment out if objects change frequently or if it causes issues
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}
