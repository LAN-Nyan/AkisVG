#pragma once
#include "canvas/objects/vectorobject.h"
#include <QImage>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QTransform>
#include <optional>

// ─── HandleRole ──────────────────────────────────────────────────────────────
// The 9 control handles drawn around a selected image object.

enum class HandleRole {
    TopLeft,     TopCenter,    TopRight,
    MiddleLeft,               MiddleRight,
    BottomLeft,  BottomCenter, BottomRight,
    Rotate
};

// ─── TransformableImageObject ────────────────────────────────────────────────
// Drop-in replacement for ImageObject that adds interactive transform handles:
// 8 scale handles (corners + edges) and an orange rotation handle.
//
// INTEGRATION – see CANVASVIEW_PATCHES.cpp and MAINWINDOW_PATCHES.cpp.

class TransformableImageObject : public VectorObject
{
public:
    explicit TransformableImageObject(const QImage &image,
                                      QGraphicsItem *parent = nullptr);
    ~TransformableImageObject() override = default;

    // ── VectorObject interface ────────────────────────────────────────────────
    VectorObject*    clone()      const override;
    VectorObjectType objectType() const override { return VectorObjectType::Image; }

    QRectF boundingRect() const override;
    void   paint(QPainter *painter,
                 const QStyleOptionGraphicsItem *option,
                 QWidget *widget = nullptr) override;

    // ── Serialization helper ─────────────────────────────────────────────────
    QImage getImage() const { return m_image; }

    // ── Geometry ─────────────────────────────────────────────────────────────
    QPointF position()  const { return m_pos;   }
    qreal   imgWidth()  const { return m_w;     }
    qreal   imgHeight() const { return m_h;     }
    qreal   imgAngle()  const { return m_angle; }

    void setPosition(QPointF p)        { prepareGeometryChange(); m_pos   = p;   }
    void setImgSize (qreal w, qreal h) { prepareGeometryChange(); m_w = w; m_h = h; }
    void setImgAngle(qreal deg)        { prepareGeometryChange(); m_angle = deg; }

    // ── Selection & handles ───────────────────────────────────────────────────
    bool isSelected()  const  { return m_selected; }
    void setSelected(bool s)  { m_selected = s; update(); }

    static constexpr qreal HANDLE_RADIUS      = 6.0;
    static constexpr qreal ROTATE_HANDLE_DIST = 28.0;

    /// Returns true if scenePos is inside the image body.
    bool hitTestImage(QPointF scenePos) const;

    /// Returns the HandleRole hit at scenePos, or nullopt.
    std::optional<HandleRole> hitTestHandle(QPointF scenePos) const;

    // ── Transform drag (call from CanvasView mouse handlers) ─────────────────
    void beginTransform   (HandleRole role, QPointF startScene);
    void continueTransform(QPointF currentScene);
    void endTransform     ();

    /// Draw the selection box + handles. Call from CanvasView after scene render.
    void drawHandles(QPainter *painter) const;

private:
    QImage   m_image;
    QPixmap  m_pixmap;
    qreal    m_w, m_h;
    QPointF  m_pos;         // scene-space centre of the image
    qreal    m_angle = 0.0; // rotation in degrees
    bool     m_selected = false;

    // Drag state
    HandleRole m_activeHandle = HandleRole::TopLeft;
    QPointF    m_dragStart;
    QPointF    m_origPos;
    qreal      m_origW, m_origH, m_origAngle;
    bool       m_transforming = false;

    // Helpers
    QTransform localToScene() const;
    QTransform sceneToLocal() const;
    QVector<QPointF> handlePositions() const;
    QRectF localRect() const { return {-m_w/2, -m_h/2, m_w, m_h}; }
};
