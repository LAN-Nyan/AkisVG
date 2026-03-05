#pragma once
#include "tool.h"
#include <QPolygonF>
#include <QPointF>
#include <QVector>
#include <QPainterPath>

// Freehand Lasso Selection Tool
//
// WORKFLOW:
//   1. Press & drag  — draw the freehand selection outline
//   2. Release       — loop auto-closes; marching ants appear
//   3. In PULL MODE  — drag inside selection to move the cut geometry
//                      (MainWindow splits the objects and hands them to
//                       SelectTool so they move with the cursor)
//   4. Right-click   — context menu: Fill, Split/Cut, Copy, Cancel
//   5. Escape        — cancel selection
//
// The tool emits actionPull(polygon, dragStart) when the user starts
// dragging inside a closed selection. MainWindow then:
//   • splits intersected objects along the lasso boundary
//   • activates SelectTool with those pieces pre-selected
//   • the user drags them to their new position
class LassoTool : public Tool
{
    Q_OBJECT

public:
    explicit LassoTool(QObject *parent = nullptr);
    ~LassoTool() override = default;

    ToolType toolType() const override { return ToolType::Lasso; }

    void mousePressEvent  (QMouseEvent *event, QPointF scenePos) override;
    void mouseMoveEvent   (QMouseEvent *event, QPointF scenePos) override;
    void mouseReleaseEvent(QMouseEvent *event, QPointF scenePos) override;
    void keyPressEvent    (QKeyEvent   *event) override;
    void draw(QPainter *painter) override;

    void cancel();
    bool      hasSelection()     const { return m_closed; }
    QPolygonF selectionPolygon() const { return m_polygon; }

    // FIX #22: "Work as Fill Tool" mode
    void setFillMode(bool on) { m_fillMode = on; }
    bool fillMode() const { return m_fillMode; }

signals:
    void selectionChanged();
    void actionFill(const QPolygonF &polygon, const QColor &color);
    void actionCut (const QPolygonF &polygon);
    void actionCopy(const QPolygonF &polygon);
    // User started dragging inside the closed selection — MainWindow should
    // split+select the geometry so it moves with the cursor.
    void actionPull(const QPolygonF &polygon, QPointF dragStart);

private:
    static constexpr qreal MIN_POINT_DISTANCE = 3.0;

    // Drawing phase
    QPolygonF m_polygon;
    QPointF   m_cursorPos;
    bool      m_drawing = false;
    bool      m_closed  = false;

    // Pull phase (after geometry has been split by MainWindow)
    bool    m_pullActive   = false;  // true while dragging pulled pieces
    QPointF m_pullOrigin;            // where the drag started (scene coords)
    bool    m_pullEmitted  = false;  // so we only emit actionPull once per gesture

    // Marching ants
    qreal   m_antOffset  = 0.0;
    qint64  m_lastTickMs = 0;

    // FIX #22: Fill tool mode — click to add points, Del to remove last, close to fill
    bool    m_fillMode   = false;

    bool isInsideSelection(QPointF scenePos) const;
    void closeLasso();
    void showActionMenu();
    void updateAntOffset();
};
