#include "lassotool.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QApplication>
#include <QDateTime>
#include <cmath>

LassoTool::LassoTool(QObject *parent)
    : Tool(ToolType::Lasso, parent)
{
    setName("Lasso");
    setCursor(Qt::CrossCursor);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

bool LassoTool::isInsideSelection(QPointF p) const
{
    if (!m_closed || m_polygon.isEmpty()) return false;
    QPainterPath pp;
    pp.addPolygon(m_polygon);
    pp.closeSubpath();
    return pp.contains(p);
}

void LassoTool::updateAntOffset()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastTickMs == 0) { m_lastTickMs = now; return; }
    qreal dt = static_cast<qreal>(now - m_lastTickMs) / 800.0;
    m_antOffset  = std::fmod(m_antOffset + dt * 10.0, 10.0);
    m_lastTickMs = now;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse events
// ─────────────────────────────────────────────────────────────────────────────

void LassoTool::mousePressEvent(QMouseEvent *event, QPointF scenePos)
{
    // ── Right-click: menu (only when selection exists) ────────────────────────
    if (event->button() == Qt::RightButton) {
        if (m_closed) {
            event->accept();
            showActionMenu();
        }
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    // ── Left-click inside a closed selection → start pull ────────────────────
    if (m_closed && isInsideSelection(scenePos)) {
        m_pullActive  = true;
        m_pullEmitted = false;
        m_pullOrigin  = scenePos;
        event->accept();
        return;
    }

    // ── Left-click outside (or no selection) → start fresh drawing ───────────
    cancel();          // clears any old selection
    m_polygon.clear();
    m_polygon << scenePos;
    m_cursorPos  = scenePos;
    m_drawing    = true;
    m_antOffset  = 0.0;
    m_lastTickMs = QDateTime::currentMSecsSinceEpoch();
    event->accept();
}

void LassoTool::mouseMoveEvent(QMouseEvent *event, QPointF scenePos)
{
    m_cursorPos = scenePos;
    updateAntOffset();

    // ── Pull mode: emit once to trigger split+select in MainWindow ────────────
    if (m_pullActive && m_closed) {
        if (!m_pullEmitted) {
            QPointF d = scenePos - m_pullOrigin;
            if (std::hypot(d.x(), d.y()) > 4.0) {
                m_pullEmitted = true;
                // Snapshot the polygon before clearing state, so the signal
                // carries valid data even after we reset our own fields.
                QPolygonF polySnapshot = m_polygon;
                QPointF   originSnapshot = m_pullOrigin;

                // Clear lasso state BEFORE emitting — prevents use-after-free
                // if the signal handler calls activateTool() which changes what
                // tool is current, potentially deleting this tool's context.
                m_polygon.clear();
                m_drawing    = false;
                m_closed     = false;
                m_pullActive = false;
                m_antOffset  = 0.0;

                emit actionPull(polySnapshot, originSnapshot);
                // Don't call cancel() here — state is already cleared above,
                // and cancel() would emit selectionChanged which can crash if
                // the signal chain has already switched tools.
            }
        }
        event->accept();
        return;
    }

    // ── Drawing phase: accumulate points ─────────────────────────────────────
    if (m_drawing && !m_closed && !m_polygon.isEmpty()) {
        QPointF d = scenePos - m_polygon.last();
        if (std::hypot(d.x(), d.y()) >= MIN_POINT_DISTANCE) {
            m_polygon << scenePos;
            emit selectionChanged();
        }
    }

    event->accept();
}

void LassoTool::mouseReleaseEvent(QMouseEvent *event, QPointF scenePos)
{
    if (event->button() != Qt::LeftButton) return;

    // ── End pull drag ─────────────────────────────────────────────────────────
    if (m_pullActive) {
        m_pullActive  = false;
        m_pullEmitted = false;
        event->accept();
        return;
    }

    // ── End drawing: close the loop ───────────────────────────────────────────
    if (!m_drawing) return;

    // Add final point if far enough
    if (!m_polygon.isEmpty()) {
        QPointF d = scenePos - m_polygon.last();
        if (std::hypot(d.x(), d.y()) >= MIN_POINT_DISTANCE)
            m_polygon << scenePos;
    }

    if (m_polygon.size() >= 3) {
        closeLasso();
    } else {
        cancel();
    }

    event->accept();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Drawing overlay
// ─────────────────────────────────────────────────────────────────────────────

void LassoTool::draw(QPainter *painter)
{
    if (m_polygon.isEmpty()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (m_drawing && !m_closed) {
        // ── Live freehand path while dragging ────────────────────────────────
        QPainterPath path;
        path.moveTo(m_polygon.first());
        for (int i = 1; i < m_polygon.size(); ++i)
            path.lineTo(m_polygon[i]);
        path.lineTo(m_cursorPos);  // rubber-band tail

        // Dark shadow for contrast on any background
        painter->setPen(QPen(QColor(0,0,0,160), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);

        // White dash on top
        QPen dp(Qt::white, 1.5, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        dp.setDashPattern({5, 4});
        painter->setPen(dp);
        painter->drawPath(path);

        // Start-point handle — shows where the loop will close to
        painter->setPen(QPen(QColor(0,0,0,200), 1.5));
        painter->setBrush(Qt::white);
        painter->drawEllipse(m_polygon.first(), 5.0, 5.0);

        // Closing-snap indicator: when cursor is close to start, turn green
        QPointF ds = m_cursorPos - m_polygon.first();
        if (!m_polygon.isEmpty() && m_polygon.size() >= 3
                && std::hypot(ds.x(), ds.y()) < 20.0) {
            painter->setPen(QPen(QColor(0,220,80), 2.0));
            painter->setBrush(QColor(0,220,80,60));
            painter->drawEllipse(m_polygon.first(), 10.0, 10.0);
        }

    } else if (m_closed) {
        // ── Closed selection — marching ants + pull hint ──────────────────────
        QPainterPath selPath;
        selPath.moveTo(m_polygon.first());
        for (int i = 1; i < m_polygon.size(); ++i)
            selPath.lineTo(m_polygon[i]);
        selPath.closeSubpath();

        // Subtle fill tint
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 120, 255, 25));
        painter->drawPath(selPath);

        // Dark outline
        painter->setPen(QPen(QColor(0,0,0,180), 2.0, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(selPath);

        // Marching ants
        QPen antPen(Qt::white, 1.5, Qt::CustomDashLine, Qt::FlatCap);
        antPen.setDashPattern({5, 5});
        antPen.setDashOffset(m_antOffset);
        painter->setPen(antPen);
        painter->drawPath(selPath);

        // Cursor-aware hint text
        bool inside = isInsideSelection(m_cursorPos);
        QRectF bb = selPath.boundingRect();

        if (inside) {
            // Highlight the fill to show "you can drag"
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0, 140, 255, 45));
            painter->drawPath(selPath);

            painter->setFont(QFont("sans-serif", 9, QFont::Bold));
            painter->setPen(QColor(255,255,255,220));
            painter->drawText(bb.center() + QPointF(0, -8), "Drag to Pull");
            painter->setPen(QColor(0,0,0,120));
            painter->drawText(bb.center() + QPointF(1, -7), "Drag to Pull");
        } else {
            painter->setFont(QFont("sans-serif", 8));
            painter->setPen(QColor(255,255,255,180));
            QString hint = "Drag inside to Pull  •  RMB for options  •  Esc cancel";
            painter->drawText(bb.bottomLeft() + QPointF(0, 14), hint);
        }
    }

    painter->restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Close / menu / cancel
// ─────────────────────────────────────────────────────────────────────────────

void LassoTool::closeLasso()
{
    m_drawing = false;
    m_closed  = true;
    emit selectionChanged();
    // Don't show menu immediately — let user pull or right-click
}

void LassoTool::showActionMenu()
{
    QWidget *parent = QApplication::activeWindow();
    QMenu menu(parent);
    menu.setStyleSheet(
        "QMenu { background:#1e1e1e; color:#eee; border:1px solid #444;"
        "        font-size:13px; padding:2px; }"
        "QMenu::item { padding:5px 20px; }"
        "QMenu::item:selected { background:#0078d4; border-radius:3px; }"
        "QMenu::separator { height:1px; background:#444; margin:3px 8px; }");

    QAction *fillAct   = menu.addAction("✏  Fill Selection…");
    QAction *cutAct    = menu.addAction("✂  Split / Cut");
    QAction *copyAct   = menu.addAction("⎘  Copy");
    menu.addSeparator();
    menu.addAction("✕  Cancel");

    QAction *chosen = menu.exec(QCursor::pos());

    if (chosen == fillAct) {
        QColor c = QColorDialog::getColor(Qt::white, parent, "Fill Color",
                                          QColorDialog::ShowAlphaChannel);
        if (c.isValid()) emit actionFill(m_polygon, c);
        cancel();
    } else if (chosen == cutAct) {
        emit actionCut(m_polygon);
        cancel();
    } else if (chosen == copyAct) {
        emit actionCopy(m_polygon);
        cancel();
    }
    // else: dismissed or Cancel — leave selection active
}

void LassoTool::cancel()
{
    m_polygon.clear();
    m_drawing    = false;
    m_closed     = false;
    m_pullActive = false;
    m_pullEmitted= false;
    m_antOffset  = 0.0;
    emit selectionChanged();
}
