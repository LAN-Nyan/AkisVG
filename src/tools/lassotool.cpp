#include "lassotool.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QApplication>
#include <QDateTime>
#include <QTimer>
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
    // FIX #10: Quick bounding-rect reject before the O(n) contains test
    if (!m_polygon.boundingRect().contains(p)) return false;
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

    // ── FIX #22: Fill-tool / point-click mode ─────────────────────────────────
    // In this mode, each click adds a precise point. Del removes the last point.
    // Clicking near the start point (or double-clicking) closes the polygon.
    if (m_fillMode) {
        if (m_closed) {
            // Closed selection exists — treat as fill/action
            event->accept();
            showActionMenu();
            return;
        }

        if (!m_polygon.isEmpty()) {
            // Check if near start → close
            QPointF ds = scenePos - m_polygon.first();
            if (m_polygon.size() >= 3 && std::hypot(ds.x(), ds.y()) < 20.0) {
                closeLasso();
                event->accept();
                return;
            }
        }

        // Add new point
        if (m_polygon.isEmpty()) {
            m_drawing = true;
            m_antOffset = 0.0;
            m_lastTickMs = QDateTime::currentMSecsSinceEpoch();
        }
        m_polygon << scenePos;
        m_cursorPos = scenePos;
        emit selectionChanged();
        event->accept();
        return;
    }
    // ── End fill-tool mode ────────────────────────────────────────────────────

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
        if (m_fillMode) {
            // Fill-tool mode: just track cursor for rubber-band preview, don't accumulate
            // Points are placed on click only
        } else {
            QPointF d = scenePos - m_polygon.last();
            if (std::hypot(d.x(), d.y()) >= MIN_POINT_DISTANCE) {
                // FIX #10: Cap point count — very long strokes with thousands of
                // points cause slow QPainterPath construction and can OOM/segfault
                // downstream when boolean ops are applied.
                constexpr int MAX_LASSO_POINTS = 4000;
                if (m_polygon.size() < MAX_LASSO_POINTS)
                    m_polygon << scenePos;
                emit selectionChanged();
            }
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

    // In fill-tool mode, points are placed on click — don't close on release
    if (m_fillMode) {
        event->accept();
        return;
    }

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

    // FIX #10: Snapshot the polygon so the painter never reads a list that is
    // being mutated by a concurrent mouse event (can happen during fast gestures).
    // Also skip any point that contains NaN/Inf which crashes QPainterPath.
    QPolygonF poly;
    poly.reserve(m_polygon.size());
    for (const QPointF &pt : m_polygon) {
        if (std::isfinite(pt.x()) && std::isfinite(pt.y()))
            poly << pt;
    }
    if (poly.isEmpty()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (m_drawing && !m_closed) {
        // ── Live freehand path while dragging ────────────────────────────────
        QPainterPath path;
        path.moveTo(poly.first());
        for (int i = 1; i < poly.size(); ++i)
            path.lineTo(poly[i]);

        if (m_fillMode && !poly.isEmpty())
            path.lineTo(m_cursorPos);
        else if (!m_fillMode)
            path.lineTo(m_cursorPos);

        // Dark shadow for contrast on any background
        painter->setPen(QPen(QColor(0,0,0,160), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);

        // White dash on top
        QPen dp(Qt::white, 1.5, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        dp.setDashPattern({5, 4});
        painter->setPen(dp);
        painter->drawPath(path);

        // Start-point handle
        painter->setPen(QPen(QColor(0,0,0,200), 1.5));
        painter->setBrush(Qt::white);
        painter->drawEllipse(poly.first(), 5.0, 5.0);

        // Fill-tool mode: dot at each placed point
        if (m_fillMode && poly.size() > 1) {
            painter->setPen(QPen(QColor(255,140,0), 1.5));
            painter->setBrush(QColor(255,200,50,200));
            for (int i = 1; i < poly.size(); ++i)
                painter->drawEllipse(poly[i], 4.0, 4.0);
        }

        // Closing-snap indicator: turn green when near start
        QPointF ds = m_cursorPos - poly.first();
        if (poly.size() >= 3 && std::hypot(ds.x(), ds.y()) < 20.0) {
            painter->setPen(QPen(QColor(0,220,80), 2.0));
            painter->setBrush(QColor(0,220,80,60));
            painter->drawEllipse(poly.first(), 10.0, 10.0);
        }

    } else if (m_closed) {
        // ── Closed selection — marching ants + pull hint ──────────────────────
        QPainterPath selPath;
        selPath.moveTo(poly.first());
        for (int i = 1; i < poly.size(); ++i)
            selPath.lineTo(poly[i]);
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

        bool inside = isInsideSelection(m_cursorPos);
        QRectF bb = selPath.boundingRect();

        if (inside) {
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
    // FIX #22: In fill-tool mode, immediately show the action menu so user can fill
    if (m_fillMode) {
        QTimer::singleShot(50, this, [this]() { showActionMenu(); });
    }
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

// FIX #22: Del key removes last point in fill-tool (click-to-place) mode
void LassoTool::keyPressEvent(QKeyEvent *event)
{
    if (m_fillMode && !m_closed &&
        (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)) {
        if (!m_polygon.isEmpty()) {
            m_polygon.removeLast();
            if (m_polygon.isEmpty()) m_drawing = false;
            emit selectionChanged();
            event->accept();
        }
        return;
    }
    event->ignore();
}
