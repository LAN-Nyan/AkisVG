// TODO:
// 1. This is more annoying than a software destroyer but the ring around the triangle is offset so Blue on the ring, is red on the triangle?


#include "colorpicker.h"
#include "utils/thememanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <QtMath>
#include <QBitmap>
#include <QScrollArea>

// ============= ColorWheel Implementation =============

ColorWheel::ColorWheel(QWidget *parent)
    : QWidget(parent)
    , m_currentColor(Qt::red)
    , m_isSelectingHue(false)
    , m_isSelectingSV(false)
    , m_outerRadius(100)
    , m_innerRadius(70)
{
    setMinimumSize(220, 220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

// Look for the ColorWheel section near the top of colorpicker.cpp
void ColorWheel::setColor(const QColor &color)
{
    m_currentColor = color;
    update(); // This tells Qt to redraw the wheel with the new color
}

void ColorWheel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    m_center = QPointF(width() / 2.0, height() / 2.0);
    m_outerRadius = qMin(width(), height()) / 2.0 - 10;
    m_innerRadius = m_outerRadius * 0.7;

    // Draw hue wheel with conical gradient
    QConicalGradient hueGradient(m_center, -90);
    for (int i = 0; i < 360; ++i) {
        hueGradient.setColorAt(i / 360.0, QColor::fromHsvF(i / 360.0, 1.0, 1.0));
    }

    QPainterPath ring;
    ring.addEllipse(m_center, m_outerRadius, m_outerRadius);
    QPainterPath innerCircle;
    innerCircle.addEllipse(m_center, m_innerRadius, m_innerRadius);
    ring = ring.subtracted(innerCircle);

    painter.fillPath(ring, hueGradient);

    // Draw SV triangle with proper gradient
    qreal hue = m_currentColor.hueF();
    if (hue < 0) hue = 0;

    // Angle formula: hue * 2*PI + PI/2
    // This maps the hue correctly onto the QConicalGradient(center, -90) ring:
    //   hue=0.0 (red)   → bottom,  hue=0.25 (green) → left
    //   hue=0.5 (cyan)  → top,     hue=0.75 (blue)  → right
    // pPure is placed at the hue indicator position so triangle and ring agree.
    qreal angle = hue * 2 * M_PI + M_PI / 2;
    QPointF pPure  = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;
    angle += 2 * M_PI / 3;
    QPointF pWhite = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;
    angle += 2 * M_PI / 3;
    QPointF pBlack = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;

    QPainterPath triangle;
    triangle.moveTo(pPure);
    triangle.lineTo(pWhite);
    triangle.lineTo(pBlack);
    triangle.closeSubpath();

    // HSV triangle layout:
    // - pPure  = Pure hue (S=1, V=1) — aligned with hue ring indicator
    // - pWhite = White    (S=0, V=1)
    // - pBlack = Black    (S=0, V=0)

    // Draw using a raster approach for accurate gradient
    QImage triangleImage(width(), height(), QImage::Format_ARGB32);
    triangleImage.fill(Qt::transparent);

    QPainter imagePainter(&triangleImage);
    imagePainter.setRenderHint(QPainter::Antialiasing);

    // For each pixel in the triangle, calculate the correct HSV color
    QRect boundingRect = triangle.boundingRect().toRect();
    boundingRect = boundingRect.intersected(triangleImage.rect());

    for (int y = boundingRect.top(); y <= boundingRect.bottom(); ++y) {
        for (int x = boundingRect.left(); x <= boundingRect.right(); ++x) {
            QPointF point(x, y);
            if (!triangle.contains(point)) continue;

            // Calculate barycentric coordinates
            QPointF v0 = pPure - pWhite;
            QPointF v1 = pBlack - pWhite;
            QPointF v2 = point - pWhite;

            qreal dot00 = QPointF::dotProduct(v0, v0);
            qreal dot01 = QPointF::dotProduct(v0, v1);
            qreal dot02 = QPointF::dotProduct(v0, v2);
            qreal dot11 = QPointF::dotProduct(v1, v1);
            qreal dot12 = QPointF::dotProduct(v1, v2);

            qreal denom = dot00 * dot11 - dot01 * dot01;
            if (qAbs(denom) < 0.0001) continue;

            qreal invDenom = 1.0 / denom;
            qreal u = (dot11 * dot02 - dot01 * dot12) * invDenom;
            qreal v = (dot00 * dot12 - dot01 * dot02) * invDenom;

            // Clamp to triangle
            if (u < 0 || v < 0 || u + v > 1) continue;

            // u = saturation (0 at white vertex, 1 at pure hue vertex)
            // v = inverse value (0 at top/bottom edge, 1 at black vertex)
            qreal saturation = u;
            qreal value = 1.0 - v;

            QColor pixelColor = QColor::fromHsvF(hue, saturation, value);
            imagePainter.setPen(pixelColor);
            imagePainter.drawPoint(x, y);
        }
    }

    painter.drawImage(0, 0, triangleImage);

    // Draw triangle outline
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.drawPath(triangle);

    // SV indicator
    QPointF svPoint = svToPoint(m_currentColor.saturationF(), m_currentColor.valueF());
    painter.setPen(QPen(QColor(0, 0, 0, 100), 2));
    painter.drawEllipse(svPoint.x() - 7, svPoint.y() - 7, 14, 14);
    painter.setPen(QPen(Qt::white, 3));
    painter.drawEllipse(svPoint, 6, 6);
    painter.setPen(QPen(Qt::black, 1.5));
    painter.drawEllipse(svPoint, 5, 5);

    // Hue indicator
    QPointF huePoint = hueToPoint(hue);
    painter.setPen(QPen(QColor(0, 0, 0, 100), 2));
    painter.drawEllipse(huePoint.x() - 9, huePoint.y() - 9, 18, 18);
    painter.setPen(QPen(Qt::white, 3));
    painter.drawEllipse(huePoint, 7, 7);
    painter.setPen(QPen(Qt::black, 1.5));
    painter.drawEllipse(huePoint, 6, 6);
}

void ColorWheel::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = event->pos();
    qreal dist = QLineF(m_center, pos).length();

    if (dist > m_innerRadius && dist < m_outerRadius) {
        m_isSelectingHue = true;
        updateColor(event->pos());
    } else if (dist < m_innerRadius) {
        m_isSelectingSV = true;
        updateColor(event->pos());
    }
}

void ColorWheel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isSelectingHue || m_isSelectingSV) {
        updateColor(event->pos());
    }
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_isSelectingHue = false;
    m_isSelectingSV = false;
}

void ColorWheel::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_wheelPixmap = QPixmap();
    m_trianglePixmap = QPixmap();
}

void ColorWheel::updateColor(const QPoint &pos)
{
    QPointF delta = pos - m_center;

    if (m_isSelectingHue) {
        // Inverse of placement formula: angle_placed = hue*2pi + pi/2
        // → hue = (atan2(dy,dx) - pi/2) / (2pi)
        qreal angle = qAtan2(delta.y(), delta.x()) - M_PI / 2;
        if (angle < 0) angle += 2 * M_PI;
        qreal hue = angle / (2 * M_PI);

        m_currentColor.setHsvF(hue, m_currentColor.saturationF(), m_currentColor.valueF());
        emit colorChanged(m_currentColor);
        update();
    } else if (m_isSelectingSV) {
        qreal hue = m_currentColor.hueF();
        if (hue < 0) hue = 0;

        // FIX: Use correct angle formula — hue*2pi + pi/2
        qreal angle = hue * 2 * M_PI + M_PI / 2;
        QPointF pColor = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;  // pPure
        angle += 2 * M_PI / 3;
        QPointF pWhite = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;
        angle += 2 * M_PI / 3;
        QPointF pBlack = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;

        QPointF v0 = pColor - pWhite;
        QPointF v1 = pBlack - pWhite;
        QPointF v2 = pos - pWhite;

        qreal dot00 = QPointF::dotProduct(v0, v0);
        qreal dot01 = QPointF::dotProduct(v0, v1);
        qreal dot02 = QPointF::dotProduct(v0, v2);
        qreal dot11 = QPointF::dotProduct(v1, v1);
        qreal dot12 = QPointF::dotProduct(v1, v2);

        qreal invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
        qreal u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        qreal v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        if (u < 0) u = 0;
        if (v < 0) v = 0;
        if (u + v > 1) {
            qreal total = u + v;
            u /= total;
            v /= total;
        }

        qreal saturation = u;
        qreal value = 1.0 - v;

        m_currentColor.setHsvF(hue, saturation, value);
        emit colorChanged(m_currentColor);
        update();
    }
}

QPointF ColorWheel::hueToPoint(qreal hue) const
{
    // FIX: hue*2pi + pi/2 matches the QConicalGradient(-90) ring layout
    qreal angle = hue * 2 * M_PI + M_PI / 2;
    qreal radius = (m_outerRadius + m_innerRadius) / 2;
    return m_center + QPointF(qCos(angle), qSin(angle)) * radius;
}

QPointF ColorWheel::svToPoint(qreal saturation, qreal value) const
{
    qreal hue = m_currentColor.hueF();
    if (hue < 0) hue = 0;

    // FIX: Use correct angle formula — hue*2pi + pi/2
    qreal angle = hue * 2 * M_PI + M_PI / 2;
    QPointF pColor = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;  // pPure
    angle += 2 * M_PI / 3;
    QPointF pWhite = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;
    angle += 2 * M_PI / 3;
    QPointF pBlack = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.97;

    QPointF satPoint = pWhite + (pColor - pWhite) * saturation;
    QPointF finalPoint = satPoint + (pBlack - satPoint) * (1.0 - value);

    return finalPoint;
}

// ============= ColorPicker Implementation =============

ColorPicker::ColorPicker(QWidget *parent)
    : QWidget(parent)
    , m_currentColor(Qt::black)
{
    setupUI();

    m_recentColors << Qt::black << Qt::white << Qt::red << Qt::green
                   << Qt::blue << Qt::yellow << Qt::cyan << Qt::magenta;
    updateUI();
}

void ColorPicker::setupUI()
{
    // Use the global 'theme()' helper that your other files use
    const auto &t = theme();

    // Outer layout: just holds the scroll area
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // FIX: This removes the default Blue (#2596be) by forcing the background
    scrollArea->setStyleSheet(QString(
        "QScrollArea { border: none; background: %1; }"
        "QScrollBar:vertical { background: %1; width: 8px; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    ).arg(t.bg0, t.bg2, t.accent));

    outerLayout->addWidget(scrollArea);

    // Inner widget that holds all the actual controls
    QWidget *inner = new QWidget();
    // FIX: This forces the inner "canvas" of the picker to match the theme
    inner->setStyleSheet(QString("background-color: %1;").arg(t.bg0));
    scrollArea->setWidget(inner);

    QVBoxLayout *mainLayout = new QVBoxLayout(inner);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    m_titleLabel = new QLabel("COLOR PICKER");
    m_titleLabel->setStyleSheet(QString("font-weight: bold; font-size: 10px; color: %1; letter-spacing: 2px;").arg(t.accent));
    mainLayout->addWidget(m_titleLabel);

    m_colorWheel = new ColorWheel();
    m_colorWheel->setFixedSize(220, 220);
    mainLayout->addWidget(m_colorWheel, 0, Qt::AlignCenter);

    connect(m_colorWheel, &ColorWheel::colorChanged, this, &ColorPicker::onWheelColorChanged);

    // Opacity slider
    m_opacityLabel = new QLabel("Opacity");
    m_opacityLabel->setStyleSheet(QString("font-size: 10px; color: %1;").arg(t.accent));
    mainLayout->addWidget(m_opacityLabel);

    QHBoxLayout *opacityLayout = new QHBoxLayout();
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setStyleSheet(QString(
        "QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: %3; }"
    ).arg(t.bg1, t.accent, t.accentHover));

    QLabel *opacityValue = new QLabel("100%");
    opacityValue->setFixedWidth(40);
    opacityValue->setStyleSheet("color: #ccc; font-size: 11px;");

    connect(m_opacitySlider, &QSlider::valueChanged, this, [opacityValue](int value) {
        opacityValue->setText(QString("%1%").arg(value));
    });
    connect(m_opacitySlider, &QSlider::valueChanged, this, &ColorPicker::onOpacityChanged);

    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(opacityValue);
    mainLayout->addLayout(opacityLayout);

    // Hex input
    m_hexLabel = new QLabel("Hex Color");
    m_hexLabel->setStyleSheet(QString("font-size: 10px; color: %1;").arg(t.accent));
    mainLayout->addWidget(m_hexLabel);

    m_hexInput = new QLineEdit();
    m_hexInput->setPlaceholderText("#000000");
    m_hexInput->setMaxLength(7);
    m_hexInput->setStyleSheet(QString(
        "QLineEdit {"
        "   background-color: %1;"
        "   border: 1px solid %2;"
        "   border-radius: 4px; padding: 6px; color: white; font-family: monospace;"
        "}"
        "QLineEdit:focus { border-color: %3; }"
    ).arg(t.bg4, t.bg1, t.accent));
    connect(m_hexInput, &QLineEdit::editingFinished, this, &ColorPicker::onHexChanged);
    mainLayout->addWidget(m_hexInput);

    // Color Palettes
    m_paletteLabel = new QLabel("Color Palettes");
    m_paletteLabel->setStyleSheet(QString("font-size: 10px; color: %1; margin-top: 4px;").arg(t.accent));
    mainLayout->addWidget(m_paletteLabel);

    m_paletteCombo = new QComboBox();
    QString comboStyle = QString(
        "QComboBox {"
        "   background-color: %1;"
        "   border: 2px solid %2;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "   color: white;"
        "   font-size: 10px;"
        "}"
        "QComboBox:hover {"
        "   border-color: %3;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: %1;"
        "   color: white;"
        "   selection-background-color: %3;"
        "}"
    ).arg(t.bg4, t.bg1, t.accent);
    m_paletteCombo->setStyleSheet(comboStyle);

    loadPalettes();

    for (const QString &paletteName : m_palettes.keys()) {
        m_paletteCombo->addItem(paletteName);
    }

    connect(m_paletteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColorPicker::onPaletteChanged);

    mainLayout->addWidget(m_paletteCombo);

    m_paletteWidget = new QWidget();
    QGridLayout *paletteGrid = new QGridLayout(m_paletteWidget);
    paletteGrid->setSpacing(4);
    paletteGrid->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_paletteWidget);

    updatePaletteDisplay();

    // Recent colors
    m_recentLabel = new QLabel("Recent Colors");
    m_recentLabel->setStyleSheet(QString("font-size: 10px; color: %1; margin-top: 8px;").arg(t.accent));
    mainLayout->addWidget(m_recentLabel);

    m_recentColorsWidget = new QWidget();
    QGridLayout *recentGrid = new QGridLayout(m_recentColorsWidget);
    recentGrid->setSpacing(4);
    recentGrid->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_recentColorsWidget);

    // Texture selector
    m_textureLabel = new QLabel("Brush Texture");
    m_textureLabel->setStyleSheet(QString("font-size: 10px; color: %1; margin-top: 8px;").arg(t.accent));
    mainLayout->addWidget(m_textureLabel);

    m_textureCombo = new QComboBox();
    m_textureCombo->setStyleSheet(comboStyle);

    m_textureCombo->addItem("Smooth", 0);
    m_textureCombo->addItem("Grainy", 1);
    m_textureCombo->addItem("Chalk", 2);
    m_textureCombo->addItem("Canvas", 3);

    for (int i = 0; i < m_textureCombo->count(); ++i) {
        QPixmap icon = generateTextureIcon(i);
        m_textureCombo->setItemIcon(i, QIcon(icon));
    }

    connect(m_textureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColorPicker::textureChanged);

    mainLayout->addWidget(m_textureCombo);
    mainLayout->addStretch();
}

QPixmap ColorPicker::generateTextureIcon(int textureType)
{
    QPixmap icon(48, 24);
    icon.fill(Qt::transparent);

    QPainter p(&icon);
    p.setRenderHint(QPainter::Antialiasing);

    QBrush brush(Qt::white);

    if (textureType == 1) {
        uchar bits[] = { 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44 };
        brush.setTexture(QBitmap::fromData(QSize(8, 8), bits));
    } else if (textureType == 2) {
        uchar bits[] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };
        brush.setTexture(QBitmap::fromData(QSize(8, 8), bits));
    } else if (textureType == 3) {
        uchar bits[] = { 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
        brush.setTexture(QBitmap::fromData(QSize(8, 8), bits));
    }

    QPen pen(brush, 6);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.drawLine(4, 12, 44, 12);

    return icon;
}

void ColorPicker::updateUI()
{
    // Only sync the text/slider fields — do NOT rebuild the recent-colors grid here
    // (that's handled by addToRecentColors to avoid re-entrant signal loops)
    m_hexInput->blockSignals(true);
    m_hexInput->setText(m_currentColor.name());
    m_hexInput->blockSignals(false);

    m_opacitySlider->blockSignals(true);
    m_opacitySlider->setValue(m_currentColor.alpha() * 100 / 255);
    m_opacitySlider->blockSignals(false);
}

QColor ColorPicker::currentColor() const
{
    return m_currentColor;
}

void ColorPicker::setColor(const QColor &color)
{
    if (!color.isValid()) return;

    // Preserve alpha from opacity slider if the incoming color has full opacity
    // (e.g. when switching tools the stored tool-color has alpha=255)
    QColor c = color;
    if (c.alpha() == 255) {
        c.setAlpha(m_opacitySlider->value() * 255 / 100);
    }

    m_currentColor = c;

    // Block the wheel's colorChanged signal so setColor here doesn't
    // re-fire onWheelColorChanged → infinite loop
    m_colorWheel->blockSignals(true);
    m_colorWheel->setColor(c);
    m_colorWheel->blockSignals(false);

    // Update peripheral controls without triggering their own changed signals
    m_hexInput->blockSignals(true);
    m_hexInput->setText(c.name());
    m_hexInput->blockSignals(false);

    m_opacitySlider->blockSignals(true);
    m_opacitySlider->setValue(c.alpha() * 100 / 255);
    m_opacitySlider->blockSignals(false);

    // Rebuild recent-colors display (does not emit colorChanged)
    updateUI();
}

void ColorPicker::onWheelColorChanged(const QColor &color)
{
    // Merge new hue/saturation/value with the current alpha
    m_currentColor = color;
    m_currentColor.setAlpha(m_opacitySlider->value() * 255 / 100);

    // Sync hex field only (no full updateUI — that would call setColor on the wheel again)
    m_hexInput->blockSignals(true);
    m_hexInput->setText(m_currentColor.name());
    m_hexInput->blockSignals(false);

    addToRecentColors(m_currentColor);
    emit colorChanged(m_currentColor);
}

void ColorPicker::onOpacityChanged(int value)
{
    m_currentColor.setAlpha(value * 255 / 100);
    // Don't re-set the wheel — only the alpha changed, hue/sat/val are untouched
    emit colorChanged(m_currentColor);
}

void ColorPicker::onHexChanged()
{
    QString hex = m_hexInput->text().trimmed();
    if (!hex.startsWith('#')) hex.prepend('#');
    if (QColor::isValidColorName(hex)) {
        QColor color(hex);
        color.setAlpha(m_currentColor.alpha()); // preserve current opacity
        // Use setColor which properly blocks re-entrant signals
        setColor(color);
        addToRecentColors(m_currentColor);
        emit colorChanged(m_currentColor);
    }
}

void ColorPicker::onRecentColorClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int index = btn->property("colorIndex").toInt();
    if (index >= 0 && index < m_recentColors.size()) {
        setColor(m_recentColors[index]);
        emit colorChanged(m_currentColor);
    }
}

void ColorPicker::addToRecentColors(const QColor &color)
{
    // Deduplicate and prepend
    m_recentColors.removeAll(color);
    m_recentColors.prepend(color);
    if (m_recentColors.size() > 16)
        m_recentColors.resize(16);

    // Rebuild only the recent-colors grid, not the full UI
    // (calling updateUI would also re-set hex/opacity which triggers more signals)
    QGridLayout *grid = qobject_cast<QGridLayout*>(m_recentColorsWidget->layout());
    if (!grid) return;

    while (grid->count() > 0) {
        QLayoutItem *item = grid->takeAt(0);
        delete item->widget();
        delete item;
    }

    for (int i = 0; i < m_recentColors.size() && i < 16; ++i) {
        QPushButton *colorBtn = new QPushButton();
        colorBtn->setFixedSize(28, 28);
        colorBtn->setCursor(Qt::PointingHandCursor);
        colorBtn->setProperty("colorIndex", i);
        colorBtn->setStyleSheet(QString(
            "QPushButton { background-color:%1; border:2px solid #555; border-radius:4px; }"
            "QPushButton:hover { border-color:#2a82da; }").arg(m_recentColors[i].name()));
        connect(colorBtn, &QPushButton::clicked, this, &ColorPicker::onRecentColorClicked);
        grid->addWidget(colorBtn, i / 8, i % 8);
    }
}

void ColorPicker::loadPalettes()
{
    // Default palette
    m_palettes["Default"] = {
        Qt::black, Qt::white, QColor("#808080"), QColor("#C0C0C0"),
        Qt::red, QColor("#FF8080"), QColor("#800000"), QColor("#FF0080"),
        Qt::green, QColor("#80FF80"), QColor("#008000"), QColor("#00FF80"),
        Qt::blue, QColor("#8080FF"), QColor("#000080"), QColor("#0080FF"),
        Qt::yellow, QColor("#FFFF80"), QColor("#808000"), QColor("#FF8000")
    };

    // Skin Tones
    m_palettes["Skin Tones"] = {
        QColor("#8D5524"), QColor("#C68642"), QColor("#E0AC69"),QColor("#F1C27D"),
        QColor("#FFDBAC"), QColor("#FFF0DC"), QColor("#4A312C"), QColor("#3E2723"),
        QColor("#6D4C41"), QColor("#A1887F"), QColor("#D7CCC8"), QColor("#EFEBE9"),
        QColor("#5D4037"), QColor("#795548"), QColor("#A0826D"), QColor("#DDB5A2")
    };

    // Pastel
    m_palettes["Pastel"] = {
        QColor("#FFB3BA"), QColor("#FFDFBA"), QColor("#FFFFBA"), QColor("#BAFFC9"),
        QColor("#BAE1FF"), QColor("#D4BAFF"), QColor("#FFBAF3"), QColor("#FFE5BA"),
        QColor("#FFC1CC"), QColor("#FFDAB9"), QColor("#E6E6FA"), QColor("#B0E0E6"),
        QColor("#FFE4E1"), QColor("#F0E68C"), QColor("#DDA0DD"), QColor("#F5DEB3")
    };

    // Vibrant
    m_palettes["Vibrant"] = {
        QColor("#FF0000"), QColor("#FF7F00"), QColor("#FFFF00"), QColor("#00FF00"),
        QColor("#00FFFF"), QColor("#0000FF"), QColor("#8B00FF"), QColor("#FF00FF"),
        QColor("#FF1493"), QColor("#FF4500"), QColor("#FFD700"), QColor("#32CD32"),
        QColor("#1E90FF"), QColor("#9400D3"), QColor("#FF69B4"), QColor("#FF8C00")
    };

    // Earth Tones
    m_palettes["Earth Tones"] = {
        QColor("#8B4513"), QColor("#A0522D"), QColor("#D2691E"), QColor("#CD853F"),
        QColor("#DEB887"), QColor("#F5DEB3"), QColor("#556B2F"), QColor("#6B8E23"),
        QColor("#808000"), QColor("#BDB76B"), QColor("#BC8F8F"), QColor("#CD5C5C"),
        QColor("#A52A2A"), QColor("#8B0000"), QColor("#B8860B"), QColor("#DAA520")
    };

    // Grayscale
    m_palettes["Grayscale"] = {
        QColor("#000000"), QColor("#111111"), QColor("#222222"), QColor("#333333"),
        QColor("#444444"), QColor("#555555"), QColor("#666666"), QColor("#777777"),
        QColor("#888888"), QColor("#999999"), QColor("#AAAAAA"), QColor("#BBBBBB"),
        QColor("#CCCCCC"), QColor("#DDDDDD"), QColor("#EEEEEE"), QColor("#FFFFFF")
    };

    // Ocean Blues
    m_palettes["Ocean Blues"] = {
        QColor("#001f3f"), QColor("#003d5c"), QColor("#005b7f"), QColor("#0074a3"),
        QColor("#008dc7"), QColor("#00a6ed"), QColor("#40c4ff"), QColor("#80d8ff"),
        QColor("#00838f"), QColor("#00acc1"), QColor("#00bcd4"), QColor("#26c6da"),
        QColor("#4dd0e1"), QColor("#b2ebf2"), QColor("#e0f7fa"), QColor("#006064")
    };
}

void ColorPicker::updatePaletteDisplay()
{
    QGridLayout *grid = qobject_cast<QGridLayout*>(m_paletteWidget->layout());
    if (!grid) return;

    // Clear existing buttons
    while (grid->count() > 0) {
        QLayoutItem *item = grid->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Get current palette
    QString paletteName = m_paletteCombo->currentText();
    if (!m_palettes.contains(paletteName)) return;

    const QVector<QColor> &colors = m_palettes[paletteName];

    // Add color buttons in a 8-column grid
    for (int i = 0; i < colors.size(); ++i) {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(24, 24);
        btn->setProperty("colorIndex", i);
        btn->setStyleSheet(
            QString("QPushButton {"
                    "   background-color: %1;"
                    "   border: 2px solid #555;"
                    "   border-radius: 3px;"
                    "}"
                    "QPushButton:hover {"
                    "   border-color: white;"
                    "   border-width: 2px;"
                    "}"
                    ).arg(colors[i].name()));

        connect(btn, &QPushButton::clicked, this, &ColorPicker::onPaletteColorClicked);

        grid->addWidget(btn, i / 8, i % 8);
    }
}

void ColorPicker::onPaletteChanged(int index)
{
    Q_UNUSED(index);
    updatePaletteDisplay();
}

void ColorPicker::onPaletteColorClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        int index = btn->property("colorIndex").toInt();
        QString paletteName = m_paletteCombo->currentText();

        if (m_palettes.contains(paletteName) && index >= 0 && index < m_palettes[paletteName].size()) {
            setColor(m_palettes[paletteName][index]);
            emit colorChanged(m_currentColor);
        }
    }
}

void ColorPicker::applyTheme()
{
    const ThemeColors &t = theme();

    // Update the scroll area background
    if (QScrollArea *sa = findChild<QScrollArea*>()) {
        sa->setStyleSheet(QString(
            "QScrollArea { border: none; background: %1; }"
            "QScrollBar:vertical { background: %1; width: 8px; }"
            "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 20px; }"
            "QScrollBar::handle:vertical:hover { background: %3; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        ).arg(t.bg0, t.bg2, t.accent));
        // Also update the inner widget
        if (QWidget *inner = sa->widget())
            inner->setStyleSheet(QString("background-color: %1;").arg(t.bg0));
    }

    m_titleLabel->setStyleSheet(
        QString("font-weight: bold; font-size: 10px; color: %1; letter-spacing: 2px;").arg(t.accent));
    m_opacityLabel->setStyleSheet(
        QString("font-size: 10px; color: %1;").arg(t.accent));
    m_hexLabel->setStyleSheet(
        QString("font-size: 10px; color: %1;").arg(t.accent));
    m_paletteLabel->setStyleSheet(
        QString("font-size: 10px; color: %1; margin-top: 4px;").arg(t.accent));
    m_recentLabel->setStyleSheet(
        QString("font-size: 10px; color: %1; margin-top: 8px;").arg(t.accent));
    m_textureLabel->setStyleSheet(
        QString("font-size: 10px; color: %1; margin-top: 8px;").arg(t.accent));

    m_opacitySlider->setStyleSheet(
        QString("QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
                "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
                "QSlider::handle:horizontal:hover { background: %3; }")
        .arg(t.bg1, t.accent, t.accentHover));

    m_hexInput->setStyleSheet(
        QString("QLineEdit { background: %1; border: 1px solid %2; border-radius: 4px;"
                " color: white; padding: 6px; font-family: monospace; }"
                "QLineEdit:focus { border-color: %3; }")
        .arg(t.bg4, t.bg1, t.accent));

    auto comboStyle = [&](const QString &accent) {
        return QString(
            "QComboBox { background: %1; border: 1px solid %2; border-radius: 4px;"
            " color: white; padding: 6px; }"
            "QComboBox:hover { border-color: %3; }"
            "QComboBox QAbstractItemView { background: %1; color: white;"
            " selection-background-color: %3; border: 1px solid %2; }")
            .arg(t.bg4, t.bg1, accent);
    };

    m_paletteCombo->setStyleSheet(comboStyle(t.accent));
    m_textureCombo->setStyleSheet(comboStyle(t.accent));
}
