#include "pathobject.h"
#include <QPen>
#include <QLineF>

PathObject::PathObject(QGraphicsItem *parent)
    : VectorObject(parent)
    , m_smoothPaths(true)  // Enable smooth paths by default
    , m_minPointDistance(3.0)  // Minimum distance between points for smoothing
    , m_texture(PathTexture::Smooth)  // Default to smooth texture
{
    // Path is stored in scene coordinates, item position stays at (0,0)
    // This prevents coordinate confusion
}

// The missing piece: Implementation of the clone method
VectorObject* PathObject::clone() const
{
    PathObject* copy = new PathObject();

    // Copy Path-specific data
    copy->setPath(this->m_path);
    copy->m_rawPoints = this->m_rawPoints;
    copy->m_smoothPaths = this->m_smoothPaths;
    copy->m_minPointDistance = this->m_minPointDistance;

    // Copy base VectorObject properties
    copy->setStrokeColor(this->m_strokeColor);
    copy->setFillColor(this->m_fillColor);
    copy->setStrokeWidth(this->m_strokeWidth);
    copy->setObjectOpacity(this->m_objectOpacity);

    // Copy QGraphicsItem transformation properties
    copy->setPos(this->pos());
    copy->setRotation(this->rotation());
    copy->setScale(this->scale());
    copy->setZValue(this->zValue());
    copy->setVisible(this->isVisible());

    return copy;
}

void PathObject::setPath(const QPainterPath &path)
{
    prepareGeometryChange();
    m_path = path;
    // When setting path externally, clear raw points as they're no longer valid
    m_rawPoints.clear();
    update();
}

void PathObject::addPoint(const QPointF &point)
{
    prepareGeometryChange();
    if (m_path.elementCount() == 0) {
        m_path.moveTo(point);
        m_rawPoints.append(point);
    } else {
        m_path.lineTo(point);
        m_rawPoints.append(point);
    }
    update();
}

void PathObject::rebuildSmoothPath()
{
    if (m_rawPoints.size() < 2) {
        return;
    }

    // Rebuild the path from scratch using Catmull-Rom spline
    QPainterPath newPath;
    newPath.moveTo(m_rawPoints.first());

    if (m_rawPoints.size() == 2) {
        // Just two points - draw a straight line
        newPath.lineTo(m_rawPoints.last());
    } else if (m_rawPoints.size() == 3) {
        // Three points - use quadratic curve
        QPointF p1 = m_rawPoints[0];
        QPointF p2 = m_rawPoints[1];
        QPointF p3 = m_rawPoints[2];
        newPath.quadTo(p2, p3);
    } else {
        // Four or more points - use Catmull-Rom spline
        for (int i = 0; i < m_rawPoints.size() - 1; ++i) {
            QPointF p0 = (i > 0) ? m_rawPoints[i - 1] : m_rawPoints[i];
            QPointF p1 = m_rawPoints[i];
            QPointF p2 = m_rawPoints[i + 1];
            QPointF p3 = (i + 2 < m_rawPoints.size()) ? m_rawPoints[i + 2] : m_rawPoints[i + 1];

            // Calculate Catmull-Rom control points for cubic Bezier
            // These formulas convert Catmull-Rom to Bezier curve
            QPointF cp1 = p1 + (p2 - p0) / 6.0;
            QPointF cp2 = p2 - (p3 - p1) / 6.0;

            // Draw cubic curve from p1 to p2
            newPath.cubicTo(cp1, cp2, p2);
        }
    }

    m_path = newPath;
}

void PathObject::lineTo(const QPointF &point)
{
    prepareGeometryChange();

    if (m_smoothPaths && m_path.elementCount() > 0) {
        // Get the last point
        QPointF lastPoint = m_path.currentPosition();
        QLineF line(lastPoint, point);

        // Only add point if it's far enough from the last one
        if (line.length() >= m_minPointDistance) {
            // Store the new point in our raw points list
            m_rawPoints.append(point);

            // Rebuild the entire smooth path from raw points
            rebuildSmoothPath();
        }
    } else {
        // Non-smooth mode - use direct line
        m_path.lineTo(point);
        m_rawPoints.append(point); // Keep track of raw points even in non-smooth mode
    }

    update();
}

QRectF PathObject::boundingRect() const
{
    if (m_path.isEmpty()) {
        return QRectF();
    }

    qreal penWidth = m_strokeWidth / 2.0;
    return m_path.boundingRect().adjusted(-penWidth, -penWidth, penWidth, penWidth);
}

void PathObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_path.isEmpty()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(m_objectOpacity);

    // Handle fill
    if (m_fillColor != Qt::transparent) {
        painter->setBrush(m_fillColor);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    switch (m_texture) {
    case PathTexture::Smooth: {
        // Standard solid stroke
        QPen pen(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(pen);
        painter->drawPath(m_path);
        break;
    }
    case PathTexture::Grainy: {
        // Grainy: draw the path multiple times with slight random offsets and partial opacity
        painter->setPen(QPen(m_strokeColor, m_strokeWidth * 0.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setOpacity(m_objectOpacity * 0.85);
        painter->drawPath(m_path);

        // Second pass — shifted, thinner
        painter->save();
        painter->setOpacity(m_objectOpacity * 0.45);
        QColor grainColor = m_strokeColor;
        grainColor.setAlphaF(grainColor.alphaF() * 0.6);
        painter->setPen(QPen(grainColor, m_strokeWidth * 0.4, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
        painter->translate(0.8, 0.8);
        painter->drawPath(m_path);
        painter->translate(-1.6, 0.5);
        painter->drawPath(m_path);
        painter->restore();
        break;
    }
    case PathTexture::Chalk: {
        // Chalk: soft, semi-transparent, slightly rough edges with multiple thin passes
        QColor chalkColor = m_strokeColor;

        // Base wide soft pass
        chalkColor.setAlphaF(qMin(1.0, m_strokeColor.alphaF()) * 0.55);
        painter->setPen(QPen(chalkColor, m_strokeWidth * 1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setOpacity(m_objectOpacity);
        painter->drawPath(m_path);

        // Middle solid pass
        chalkColor.setAlphaF(qMin(1.0, m_strokeColor.alphaF()) * 0.75);
        painter->setPen(QPen(chalkColor, m_strokeWidth * 0.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(m_path);

        // Edge scatter passes
        painter->save();
        chalkColor.setAlphaF(0.25);
        painter->setPen(QPen(chalkColor, m_strokeWidth * 0.25, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        painter->translate(1.0, -0.5);
        painter->drawPath(m_path);
        painter->translate(-2.0, 1.0);
        painter->drawPath(m_path);
        painter->restore();
        break;
    }
    case PathTexture::Canvas: {
        // Canvas: cross-hatched look — two passes at slight angles
        QPen pen1(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(pen1);
        painter->setOpacity(m_objectOpacity * 0.8);
        painter->drawPath(m_path);

        // Second pass with slightly different width and offset to simulate weave
        painter->save();
        QColor canvasColor = m_strokeColor;
        canvasColor.setAlphaF(canvasColor.alphaF() * 0.45);
        painter->setPen(QPen(canvasColor, m_strokeWidth * 0.35, Qt::DashDotLine, Qt::FlatCap, Qt::MiterJoin));
        painter->setOpacity(m_objectOpacity * 0.6);
        painter->translate(0.5, 0.5);
        painter->drawPath(m_path);
        painter->translate(0, -1.0);
        painter->drawPath(m_path);
        painter->restore();
        break;
    }
    }
}

void PathObject::moveTo(const QPointF &p) {
    m_path.moveTo(p); // Assuming m_path is your QPainterPath
    update(); // Trigger a redraw
}

void PathObject::quadTo(const QPointF &control, const QPointF &end) {
    m_path.quadTo(control, end);
    update();
}
