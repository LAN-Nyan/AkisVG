#include "magicwandtool.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QStack>
#include <QApplication>
#include <cmath>

MagicWandTool::MagicWandTool(QObject *parent)
    : Tool(ToolType::MagicWand, parent)
{
    setName("Magic Wand");
    setCursor(Qt::ArrowCursor);
}

void MagicWandTool::setCanvasSnapshot(const QImage &img, QPointF sceneOrigin)
{
    m_snapshot    = img.convertToFormat(QImage::Format_ARGB32);
    m_sceneOrigin = sceneOrigin;
}

void MagicWandTool::mousePressEvent(QMouseEvent *event, QPointF scenePos)
{
    if (event->button() != Qt::LeftButton || m_snapshot.isNull()) return;

    QPoint px(qRound((scenePos - m_sceneOrigin).x()),
              qRound((scenePos - m_sceneOrigin).y()));
    if (!m_snapshot.rect().contains(px)) return;

    cancel();
    m_selectionMask  = m_contiguous ? floodFill(px) : globalSelect(m_snapshot.pixel(px));
    m_outlinePolygon = buildOutline(m_selectionMask);

    if (!m_outlinePolygon.isEmpty())
        showActionMenu();   // Qt6: no widget param
}

void MagicWandTool::mouseMoveEvent   (QMouseEvent*, QPointF) {}
void MagicWandTool::mouseReleaseEvent(QMouseEvent*, QPointF) {}

void MagicWandTool::draw(QPainter *painter)
{
    if (m_outlinePolygon.isEmpty()) return;
    painter->save();

    static int animOffset = 0;
    animOffset = (animOffset + 1) % 8;

    QPen p1(Qt::white, 1.5, Qt::CustomDashLine);
    p1.setDashPattern({4, 4}); p1.setDashOffset(animOffset);
    painter->setPen(p1);
    painter->drawPolygon(m_outlinePolygon);

    QPen p2(Qt::black, 1.5, Qt::CustomDashLine);
    p2.setDashPattern({4, 4}); p2.setDashOffset(animOffset + 4);
    painter->setPen(p2);
    painter->drawPolygon(m_outlinePolygon);

    painter->restore();
}

bool MagicWandTool::colorInRange(QRgb candidate, QRgb seed) const
{
    return (qAbs(qRed(candidate)   - qRed(seed))   +
            qAbs(qGreen(candidate) - qGreen(seed)) +
            qAbs(qBlue(candidate)  - qBlue(seed))  +
            qAbs(qAlpha(candidate) - qAlpha(seed))) / 4 <= m_tolerance;
}

QImage MagicWandTool::floodFill(QPoint seedPt) const
{
    QImage mask(m_snapshot.size(), QImage::Format_Grayscale8);
    mask.fill(0);
    QRgb seedColor = m_snapshot.pixel(seedPt);
    QStack<QPoint> stack;
    stack.push(seedPt);

    while (!stack.isEmpty()) {
        QPoint p = stack.pop();
        if (!m_snapshot.rect().contains(p)) continue;
        if (reinterpret_cast<const uchar*>(mask.constScanLine(p.y()))[p.x()]) continue;
        if (!colorInRange(m_snapshot.pixel(p), seedColor)) continue;
        reinterpret_cast<uchar*>(mask.scanLine(p.y()))[p.x()] = 255;
        stack.push({p.x()+1,p.y()}); stack.push({p.x()-1,p.y()});
        stack.push({p.x(),p.y()+1}); stack.push({p.x(),p.y()-1});
    }
    return mask;
}

QImage MagicWandTool::globalSelect(QRgb seed) const
{
    QImage mask(m_snapshot.size(), QImage::Format_Grayscale8);
    mask.fill(0);
    for (int y = 0; y < m_snapshot.height(); ++y) {
        const QRgb *row = reinterpret_cast<const QRgb*>(m_snapshot.constScanLine(y));
        uchar      *mrow = mask.scanLine(y);
        for (int x = 0; x < m_snapshot.width(); ++x)
            mrow[x] = colorInRange(row[x], seed) ? 255 : 0;
    }
    return mask;
}

QPolygonF MagicWandTool::buildOutline(const QImage &mask) const
{
    QPolygonF poly;
    int W = mask.width(), H = mask.height();
    auto selected = [&](int x, int y) -> bool {
        if (x < 0 || y < 0 || x >= W || y >= H) return false;
        return mask.constScanLine(y)[x] > 127;
    };

    QPoint start(-1, -1);
    for (int y = 0; y < H && start.x() < 0; ++y)
        for (int x = 0; x < W && start.x() < 0; ++x)
            if (selected(x, y)) start = {x, y};
    if (start.x() < 0) return poly;

    const int dx[] = {1,0,-1,0}, dy[] = {0,1,0,-1};
    const int turnRight[] = {1,2,3,0}, turnLeft[] = {3,0,1,2};
    QPoint cur = start; int dir = 0, steps = 0;

    do {
        poly << (m_sceneOrigin + QPointF(cur.x(), cur.y()));
        int left = turnLeft[dir];
        QPoint leftPos = {cur.x()+dx[left], cur.y()+dy[left]};
        if (selected(leftPos.x(), leftPos.y())) { dir = left; cur = leftPos; }
        else {
            QPoint fwd = {cur.x()+dx[dir], cur.y()+dy[dir]};
            if (selected(fwd.x(), fwd.y())) cur = fwd;
            else dir = turnRight[dir];
        }
        if (++steps > W*H*4) break;
    } while (cur != start || poly.size() < 3);
    return poly;
}

void MagicWandTool::showActionMenu()
{
    QWidget *parent = QApplication::activeWindow();
    QMenu menu(parent);
    menu.setStyleSheet(
        "QMenu { background:#1e1e1e; color:#eee; border:1px solid #444; font-size:13px; }"
        "QMenu::item:selected { background:#0078d4; }");

    QAction *fillAct = menu.addAction("Fill");
    QAction *cutAct  = menu.addAction("Cut (Erase)");
    QAction *copyAct = menu.addAction("Copy");
    menu.addSeparator();
    menu.addAction("Cancel");

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == fillAct) {
        QColor color = QColorDialog::getColor(Qt::white, parent, "Choose Fill Color");
        if (color.isValid()) emit actionFill(m_outlinePolygon, color);
        cancel();
    } else if (chosen == cutAct) {
        emit actionCut(m_outlinePolygon); cancel();
    } else if (chosen == copyAct) {
        emit actionCopy(m_outlinePolygon); cancel();
    }
}

void MagicWandTool::cancel()
{
    m_selectionMask  = QImage();
    m_outlinePolygon = QPolygonF();
}
