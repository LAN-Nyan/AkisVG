#include "pathobject.h"
#include <QPen>
#include <QLineF>
#include <QPolygonF>
#include <QPainter>
#include <QPainterPathStroker>
#include <QtMath>

PathObject::PathObject(QGraphicsItem *parent)
    : VectorObject(parent)
    , m_smoothPaths(true)
    , m_minPointDistance(3.0)
    , m_texture(PathTexture::Smooth)
{}

VectorObject* PathObject::clone() const
{
    PathObject *copy = new PathObject();
    copy->setPath(m_path);
    copy->m_rawPoints        = m_rawPoints;
    copy->m_pressurePoints   = m_pressurePoints;
    copy->m_smoothPaths      = m_smoothPaths;
    copy->m_minPointDistance = m_minPointDistance;
    copy->m_dashStyle        = m_dashStyle;
    copy->m_arrowAtEnd       = m_arrowAtEnd;
    copy->m_texture          = m_texture;
    copy->setStrokeColor(m_strokeColor);
    copy->setFillColor(m_fillColor);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setObjectOpacity(m_objectOpacity);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setScale(scale());
    copy->setVisible(isVisible());
    return copy;
}

// ─── Path mutation ─────────────────────────────────────────────────────────────

void PathObject::setPath(const QPainterPath &path)
{
    prepareGeometryChange();
    m_path = path;
    m_rawPoints.clear();
    update();
}

void PathObject::addPoint(const QPointF &point)
{
    prepareGeometryChange();
    if (m_path.elementCount() == 0) { m_path.moveTo(point); m_rawPoints.append(point); }
    else                             { m_path.lineTo(point); m_rawPoints.append(point); }
    update();
}

void PathObject::rebuildSmoothPath()
{
    if (m_rawPoints.size() < 2) return;
    QPainterPath p;
    p.moveTo(m_rawPoints.first());
    if (m_rawPoints.size() == 2) {
        p.lineTo(m_rawPoints.last());
    } else if (m_rawPoints.size() == 3) {
        p.quadTo(m_rawPoints[1], m_rawPoints[2]);
    } else {
        for (int i = 0; i < m_rawPoints.size() - 1; ++i) {
            QPointF p0 = (i > 0)                     ? m_rawPoints[i-1] : m_rawPoints[i];
            QPointF p1 = m_rawPoints[i];
            QPointF p2 = m_rawPoints[i+1];
            QPointF p3 = (i+2 < m_rawPoints.size()) ? m_rawPoints[i+2] : m_rawPoints[i+1];
            p.cubicTo(p1 + (p2-p0)/6.0, p2 - (p3-p1)/6.0, p2);
        }
    }
    m_path = p;
}

void PathObject::lineTo(const QPointF &point)
{
    prepareGeometryChange();
    if (m_smoothPaths && m_path.elementCount() > 0) {
        if (QLineF(m_path.currentPosition(), point).length() >= m_minPointDistance) {
            m_rawPoints.append(point);
            rebuildSmoothPath();
        }
    } else {
        m_path.lineTo(point);
        m_rawPoints.append(point);
    }
    update();
}

void PathObject::moveTo(const QPointF &p)
{
    m_rawPoints.clear();
    m_rawPoints.append(p);
    m_path = QPainterPath();
    m_path.moveTo(p);
    update();
}

void PathObject::quadTo(const QPointF &control, const QPointF &end)
{
    prepareGeometryChange();
    m_path.quadTo(control, end);
    update();
}

void PathObject::moveBy(qreal dx, qreal dy)
{
    prepareGeometryChange();
    m_path.translate(dx, dy);
    for (QPointF      &pt : m_rawPoints)       pt     += QPointF(dx, dy);
    for (PressurePoint &pp : m_pressurePoints) pp.pos += QPointF(dx, dy);
    update();
}

void PathObject::addPressurePoint(const QPointF &pos, qreal pressure)
{
    prepareGeometryChange();
    pressure = qBound(0.05, pressure, 1.0);
    m_pressurePoints.append({pos, pressure});
    m_rawPoints.append(pos);
    m_smoothedDirty = true;

    // Simple lineTo spine — used for bounding rect during live drawing.
    if (m_pressurePoints.size() == 1) {
        m_path = QPainterPath();
        m_path.moveTo(pos);
    } else {
        m_path.lineTo(pos);
    }
    update();
}

// ─── Catmull-Rom smoother ─────────────────────────────────────────────────────

static QVector<PressurePoint> buildSmoothedPressure(const QVector<PressurePoint> &pts)
{
    const int n = pts.size();
    if (n < 2) return pts;

    QVector<PressurePoint> out;
    out.reserve(n * 20); // Reserve more for high-speed strokes

    for (int i = 0; i < n - 1; ++i) {
        const QPointF p0 = pts[qMax(0, i-1)].pos;
        const QPointF p1 = pts[i].pos;
        const QPointF p2 = pts[i+1].pos;
        const QPointF p3 = pts[qMin(n-1, i+2)].pos;

        // --- THE DYNAMIC SAMPLING LOGIC ---
        // Calculate physical distance between these two raw points
        qreal segmentLength = QLineF(p1, p2).length();

        // We want 1 point roughly every 1.5 pixels.
        // Fast move (e.g. 300px) -> ~200 steps (t increment ~0.005)
        // Slow move (e.g. 5px)   -> ~3 steps   (t increment ~0.33)
        int steps = qMax(2, qCeil(segmentLength / 1.5));

        // Safety cap to prevent memory crashes on massive jumps
        steps = qMin(steps, 500);

        for (int s = 0; s < steps; ++s) {
            const qreal t  = (qreal)s / steps;
            const qreal t2 = t*t, t3 = t2*t;
            const qreal b0 = -0.5*t3 + 1.0*t2 - 0.5*t;
            const qreal b1 =  1.5*t3 - 2.5*t2 + 1.0;
            const qreal b2 = -1.5*t3 + 2.0*t2 + 0.5*t;
            const qreal b3 =  0.5*t3 - 0.5*t2;

            out.append({
                QPointF(b0*p0.x()+b1*p1.x()+b2*p2.x()+b3*p3.x(),
                        b0*p0.y()+b1*p1.y()+b2*p2.y()+b3*p3.y()),
                qBound(0.05, pts[i].pressure + (pts[i+1].pressure - pts[i].pressure)*t, 1.0)
            });
        }
    }
    out.append(pts.last());
    return out;
}

void PathObject::rebuildSmoothedPressure() const
{
    m_smoothedDirty    = false;
    m_smoothedPressure = buildSmoothedPressure(m_pressurePoints);

    // Rebuild spine for accurate bounds/hit-test after stroke is committed.
    if (m_smoothedPressure.size() >= 2) {
        QPainterPath spine;
        spine.moveTo(m_smoothedPressure[0].pos);
        const int ns = m_smoothedPressure.size();
        for (int i = 1; i + 1 < ns; i += 2) {
            QPointF mid = (m_smoothedPressure[i].pos + m_smoothedPressure[i+1].pos) / 2.0;
            spine.quadTo(m_smoothedPressure[i].pos, mid);
        }
        spine.lineTo(m_smoothedPressure.last().pos);
        const_cast<PathObject*>(this)->m_path = spine;
    }
}

// ─── Geometry ─────────────────────────────────────────────────────────────────

QRectF PathObject::boundingRect() const
{
    if (!m_path.isEmpty())
        return m_path.boundingRect().adjusted(-m_strokeWidth*2, -m_strokeWidth*2,
                                               m_strokeWidth*2,  m_strokeWidth*2);
    if (!m_pressurePoints.isEmpty()) {
        const QPointF p = m_pressurePoints.first().pos;
        const qreal   r = m_strokeWidth;
        return QRectF(p - QPointF(r,r), QSizeF(r*2, r*2));
    }
    return QRectF();
}

// ─── Arrow head ───────────────────────────────────────────────────────────────

void PathObject::drawArrowHead(QPainter *painter, const QPainterPath &path) const
{
    const int n = path.elementCount();
    if (n < 2) return;
    QPointF tip = path.elementAt(n-1), tail;
    bool found = false;
    for (int i = n-2; i >= 0; --i) {
        tail = path.elementAt(i);
        if (QLineF(tail,tip).length() > m_strokeWidth*1.5) { found=true; break; }
    }
    if (!found) tail = path.elementAt(0);
    QLineF dir(tail,tip);
    if (dir.length() < 0.5) return;
    const qreal al = qMax(12.0, m_strokeWidth*3.5);
    const qreal ah = qMax(5.0,  m_strokeWidth*1.5);
    dir.setLength(al);
    const QPointF base = tip - QPointF(dir.dx(), dir.dy());
    const QPointF perp(-dir.dy()/dir.length()*ah, dir.dx()/dir.length()*ah);
    QPolygonF head; head << tip << base+perp << base-perp;
    painter->save();
    painter->setOpacity(m_objectOpacity);
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_strokeColor);
    painter->drawPolygon(head);
    painter->restore();
}

// ─── Jitter utility ───────────────────────────────────────────────────────────

static inline qreal jit(quint32 seed, qreal scale)
{
    seed = seed * 1664525u + 1013904223u;
    return ((qreal)(seed & 0xFFFF) / 32767.5 - 1.0) * scale;
}

// ─── Core pressure stroke renderer ───────────────────────────────────────────
// Renders the pressure stroke as overlapping filled circles (stamp method).
//
// Why not drawLine with RoundCap:
//   When pen width >> segment length (which happens with thick brushes and
//   closely-spaced Catmull-Rom points), each drawLine renders as essentially
//   just its two round caps — a circle. Adjacent circles are visible as beads.
//
// The stamp method: draw filled ellipses at each smoothed point, spaced
// closely enough that they fully overlap into a solid shape.
// Spacing threshold = 0.3 * radius ensures ~70% overlap = completely solid.
// No tangent math, no polygon winding, no artifacts on any curve shape.
// Correction: i spent forever using Unicode to clean up the coments and section off my code!
// But i couldn't fix damn pressure sensitivity
// So, if anyone forks and reads this, DONT FUCK WITH THIS PIECE!!!!!!

void PathObject::paintPressureStroke(QPainter *painter, qreal baseWidth,
                                     qreal minFraction, qreal opacityMul) const
{
    if (m_smoothedDirty) rebuildSmoothedPressure();
    const QVector<PressurePoint> &pts = m_smoothedPressure;
    if (pts.size() < 2) return;

    painter->save();
    painter->setOpacity(m_objectOpacity * opacityMul);
    painter->setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < pts.size() - 1; ++i) {
        qreal p1 = qMax(minFraction, pts[i].pressure);
        qreal p2 = qMax(minFraction, pts[i+1].pressure);
        qreal width = baseWidth * ((p1 + p2) * 0.5);

        // RoundCap & RoundJoin create a seamless connection between segments
        painter->setPen(QPen(m_strokeColor, width,
                             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawLine(pts[i].pos, pts[i+1].pos);
    }
    painter->restore();
}
// ─── Build pen (non-pressure paths) ───────────────────────────────────────────

QPen PathObject::buildStrokePen(qreal width, QColor color) const
{
    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    switch (m_dashStyle) {
    case PathDashStyle::Dashed:
        pen.setStyle(Qt::DashLine);
        pen.setDashPattern({6.0, 4.0});
        break;
    case PathDashStyle::Dotted:
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern({1.0, 3.0});
        pen.setCapStyle(Qt::RoundCap);
        break;
    default: break;
    }
    return pen;
}

// ─── Texture: Smooth ──────────────────────────────────────────────────────────

void PathObject::paintSmooth(QPainter *painter) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (!m_pressurePoints.isEmpty()) {
        paintPressureStroke(painter, m_strokeWidth, 0.08, 1.0);
    } else {
        painter->setOpacity(m_objectOpacity);
        painter->setPen(buildStrokePen(m_strokeWidth, m_strokeColor));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(m_path);
    }

    painter->restore();
}

// ─── Texture: Grainy (dry brush / charcoal) ──────────────────────────────────

void PathObject::paintGrainy(QPainter *painter) const
{
    painter->save();
    painter->setOpacity(m_objectOpacity);

    if (!m_pressurePoints.isEmpty()) {
        // Semi-transparent base
        paintPressureStroke(painter, m_strokeWidth, 0.2, 0.65);

        if (m_smoothedDirty) rebuildSmoothedPressure();
        const QVector<PressurePoint> &pts = m_smoothedPressure;
        const int n  = pts.size();
        const int nb = qMax(5, qMin(12, (int)(m_strokeWidth / 2.5)));

        for (int b = 0; b < nb; ++b) {
            const qreal tFrac  = (nb <= 1) ? 0.0 : (qreal)b / (nb-1) - 0.5;
            const qreal bAlpha = 0.25 + 0.45 * ((qreal)(b%3) / 2.0);
            QColor bc = m_strokeColor;
            bc.setAlphaF(m_strokeColor.alphaF() * bAlpha);
            QPen pen(bc, 0.6 + 0.4*(b%2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            if (b % 3 == 0) {
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern({3.0+(b%4), 2.0+(b%3)});
            }
            painter->setPen(pen);
            painter->setOpacity(m_objectOpacity);

            QPainterPath bpath;
            bool started = false;
            for (int i = 0; i < n - 1; ++i) {
                const QPointF p0 = pts[i].pos, p1 = pts[i+1].pos;
                const qreal hw0  = m_strokeWidth * 0.5 * qMax(0.2, pts[i].pressure);
                const qreal hw1  = m_strokeWidth * 0.5 * qMax(0.2, pts[i+1].pressure);
                QPointF dir = p1 - p0;
                const qreal len = qSqrt(dir.x()*dir.x()+dir.y()*dir.y());
                if (len < 0.5) continue;
                dir /= len;
                const QPointF perp(-dir.y(), dir.x());
                const int si = i / 10;
                const QPointF bp0 = p0 + perp*(hw0*tFrac + jit((quint32)(b*997+si*13),     hw0*0.15));
                const QPointF bp1 = p1 + perp*(hw1*tFrac + jit((quint32)(b*997+(si+1)*13), hw1*0.15));
                if (b%5==0 && (i/10)%7==3) { started=false; continue; }
                if (!started) { bpath.moveTo(bp0); started=true; }
                bpath.lineTo(bp1);
            }
            if (!bpath.isEmpty()) painter->drawPath(bpath);
        }
    } else {
        QColor c = m_strokeColor;
        c.setAlphaF(c.alphaF() * 0.65);
        painter->setPen(buildStrokePen(m_strokeWidth * 0.7, c));
        painter->drawPath(m_path);
        c.setAlphaF(m_strokeColor.alphaF() * 0.35);
        painter->setPen(buildStrokePen(m_strokeWidth * 0.4, c));
        painter->save();
        painter->translate(0.8, 0.8);
        painter->drawPath(m_path);
        painter->restore();
    }
    painter->restore();
}

// ─── Texture: Chalk ───────────────────────────────────────────────────────────

void PathObject::paintChalk(QPainter *painter) const
{
    painter->save();
    painter->setOpacity(m_objectOpacity);

    if (!m_pressurePoints.isEmpty()) {
        paintPressureStroke(painter, m_strokeWidth * 1.15, 0.25, 0.48);
        paintPressureStroke(painter, m_strokeWidth * 0.60, 0.2,  0.72);

        if (m_smoothedDirty) rebuildSmoothedPressure();
        const QVector<PressurePoint> &spts = m_smoothedPressure;
        const int n = spts.size();
        QColor dust = m_strokeColor;
        dust.setAlphaF(m_strokeColor.alphaF() * 0.18);
        painter->setPen(QPen(dust, 1.0, Qt::SolidLine, Qt::RoundCap));
        painter->setOpacity(m_objectOpacity);

        for (int i = 1; i < n; i += 6) {
            const QPointF pos = spts[i].pos;
            const qreal   hw  = m_strokeWidth * 0.5 * qMax(0.25, spts[i].pressure);
            QPointF dir;
            if (i == n-1) dir = pos - spts[i-1].pos;
            else          dir = spts[i+1].pos - spts[i-1].pos;
            const qreal len = qSqrt(dir.x()*dir.x()+dir.y()*dir.y());
            if (len < 0.001) continue;
            dir /= len;
            const QPointF perp(-dir.y(), dir.x());
            const int ri = i / 10;
            for (int side : {-1, 1})
                for (int d = 0; d < 4; ++d) {
                    const qreal   spread = hw * (1.0 + jit((quint32)(ri*37+d*7+side*3), 0.4));
                    const QPointF dot    = pos + perp*(side*spread) + dir*jit((quint32)(ri*13+d*31), hw*0.5);
                    painter->drawPoint(dot);
                }
        }
    } else {
        QColor c = m_strokeColor;
        c.setAlphaF(c.alphaF()*0.50);
        painter->setPen(QPen(c, m_strokeWidth*1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(m_path);
        c.setAlphaF(m_strokeColor.alphaF()*0.72);
        painter->setPen(QPen(c, m_strokeWidth*0.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(m_path);
    }
    painter->restore();
}

// ─── Texture: Canvas ──────────────────────────────────────────────────────────

void PathObject::paintCanvas(QPainter *painter) const
{
    painter->save();
    painter->setOpacity(m_objectOpacity);

    if (!m_pressurePoints.isEmpty()) {
        paintPressureStroke(painter, m_strokeWidth, 0.3, 1.0);

        if (m_smoothedDirty) rebuildSmoothedPressure();
        const QVector<PressurePoint> &pts = m_smoothedPressure;
        const int n  = pts.size();
        const int nr = qMax(3, qMin(8, (int)(m_strokeWidth / 5.0)));

        for (int r = 0; r < nr; ++r) {
            const qreal tFrac = (nr<=1) ? 0.0 : (qreal)r/(nr-1) - 0.5;
            const qreal light = 1.0 + jit((quint32)(r*331), 0.18);
            QColor rc = m_strokeColor;
            rc.setRgbF(qBound(0.0,rc.redF()*light,  1.0),
                       qBound(0.0,rc.greenF()*light, 1.0),
                       qBound(0.0,rc.blueF()*light,  1.0),
                       rc.alphaF() * 0.65);
            QPainterPath rpath; bool rs = false;
            for (int i = 0; i < n-1; ++i) {
                const QPointF p0 = pts[i].pos, p1 = pts[i+1].pos;
                const qreal hw0  = m_strokeWidth*0.5*qMax(0.3,pts[i].pressure);
                const qreal hw1  = m_strokeWidth*0.5*qMax(0.3,pts[i+1].pressure);
                QPointF dir = p1-p0;
                const qreal len = qSqrt(dir.x()*dir.x()+dir.y()*dir.y());
                if (len < 0.5) continue;
                dir /= len;
                const QPointF perp(-dir.y(),dir.x());
                const int si = i/10;
                const QPointF rp0 = p0 + perp*(hw0*tFrac + jit((quint32)(r*991+si*17),     hw0*0.10));
                const QPointF rp1 = p1 + perp*(hw1*tFrac + jit((quint32)(r*991+(si+1)*17), hw1*0.10));
                if (!rs) { rpath.moveTo(rp0); rs=true; }
                rpath.lineTo(rp1);
            }
            painter->setPen(QPen(rc, 1.2+0.8*(r%2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setOpacity(m_objectOpacity);
            if (!rpath.isEmpty()) painter->drawPath(rpath);
        }

        QColor ec = m_strokeColor; ec.setAlphaF(m_strokeColor.alphaF()*0.30);
        painter->setPen(QPen(ec,1.0,Qt::SolidLine,Qt::RoundCap));
        painter->setOpacity(m_objectOpacity);
        for (int side : {-1,1}) {
            for (int i = 6; i < n; i += 18) {
                const QPointF pos = pts[i].pos;
                const qreal   hw  = m_strokeWidth*0.5*qMax(0.3,pts[i].pressure);
                QPointF dir;
                if (i>=n-1) dir = pos - pts[i-1].pos;
                else        dir = pts[i+1].pos - pts[i-1].pos;
                const qreal len = qSqrt(dir.x()*dir.x()+dir.y()*dir.y());
                if (len < 0.001) continue;
                dir /= len;
                const QPointF perp(-dir.y(),dir.x());
                const int    ri = i/10;
                const qreal  rr = jit((quint32)(side*1000+ri*41), hw*0.3);
                const QPointF e1 = pos + perp*(side*(hw+rr));
                const QPointF e2 = e1  + dir*jit((quint32)(ri*73),hw*0.25)
                                       + perp*(side*qAbs(jit((quint32)(ri*53),hw*0.25)));
                painter->drawLine(e1, e2);
            }
        }
    } else {
        painter->setPen(QPen(m_strokeColor, m_strokeWidth,
                             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(m_path);
    }
    painter->restore();
}

// ─── Main paint dispatch ──────────────────────────────────────────────────────

void PathObject::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option); Q_UNUSED(widget);
    if (m_path.isEmpty() && m_pressurePoints.isEmpty()) return;

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (m_fillColor != Qt::transparent) {
        painter->setBrush(m_fillColor);
        painter->setPen(Qt::NoPen);
        painter->drawPath(m_path);
    }

    switch (m_texture) {
    case PathTexture::Smooth: paintSmooth(painter); break;
    case PathTexture::Grainy: paintGrainy(painter); break;
    case PathTexture::Chalk:  paintChalk(painter);  break;
    case PathTexture::Canvas: paintCanvas(painter); break;
    }

    if (m_arrowAtEnd && m_path.elementCount() >= 2)
        drawArrowHead(painter, m_path);
}
