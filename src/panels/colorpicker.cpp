#include "colorpicker.h"

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

void ColorWheel::setColor(const QColor &color)
{
    m_currentColor = color;
    update();
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

    qreal angle = hue * 2 * M_PI - M_PI / 2;
    QPointF pWhite = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;
    angle += 2 * M_PI / 3;
    QPointF pBlack = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;
    angle += 2 * M_PI / 3;
    QPointF pPure = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;

    QColor pureHue = QColor::fromHsvF(hue, 1.0, 1.0);

    QPainterPath triangle;
    triangle.moveTo(pWhite);
    triangle.lineTo(pPure);
    triangle.lineTo(pBlack);
    triangle.closeSubpath();

    // Create a proper HSV triangle:
    // - Top vertex (pWhite) = White (S=0, V=1)
    // - Bottom right (pPure) = Pure hue (S=1, V=1)
    // - Bottom left (pBlack) = Black (S=0, V=0)

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
        qreal angle = qAtan2(delta.y(), delta.x()) + M_PI / 2;
        if (angle < 0) angle += 2 * M_PI;
        qreal hue = angle / (2 * M_PI);

        m_currentColor.setHsvF(hue, m_currentColor.saturationF(), m_currentColor.valueF());
        emit colorChanged(m_currentColor);
        update();
    } else if (m_isSelectingSV) {
        qreal hue = m_currentColor.hueF();
        if (hue < 0) hue = 0;

        qreal angle = hue * 2 * M_PI - M_PI / 2;
        QPointF pWhite = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;
        angle += 2 * M_PI / 3;
        QPointF pBlack = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;
        angle += 2 * M_PI / 3;
        QPointF pColor = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;

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
    qreal angle = hue * 2 * M_PI - M_PI / 2;
    qreal radius = (m_outerRadius + m_innerRadius) / 2;
    return m_center + QPointF(qCos(angle), qSin(angle)) * radius;
}

QPointF ColorWheel::svToPoint(qreal saturation, qreal value) const
{
    qreal hue = m_currentColor.hueF();
    if (hue < 0) hue = 0;

    qreal angle = hue * 2 * M_PI - M_PI / 2;
    QPointF pWhite = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;
    angle += 2 * M_PI / 3;
    QPointF pBlack = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;
    angle += 2 * M_PI / 3;
    QPointF pColor = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.85;

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
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    QLabel *title = new QLabel("COLOR PICKER");
    title->setStyleSheet("font-weight: bold; font-size: 11px; color: #888; letter-spacing: 1px;");
    mainLayout->addWidget(title);

    m_colorWheel = new ColorWheel();
    m_colorWheel->setFixedSize(220, 220);
    mainLayout->addWidget(m_colorWheel, 0, Qt::AlignCenter);

    connect(m_colorWheel, &ColorWheel::colorChanged, this, &ColorPicker::onWheelColorChanged);

    // Opacity slider
    QLabel *opacityLabel = new QLabel("Opacity");
    opacityLabel->setStyleSheet("font-size: 10px; color: #aaa;");
    mainLayout->addWidget(opacityLabel);

    QHBoxLayout *opacityLayout = new QHBoxLayout();
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   background: #3a3a3a;"
        "   height: 20px;"
        "   border-radius: 4px;"
        "   border: 1px solid #555;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: white;"
        "   width: 16px;"
        "   height: 16px;"
        "   margin: -8px 0;"
        "   border-radius: 8px;"
        "   border: 2px solid #2a82da;"
        "}"
        );

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
    QLabel *hexLabel = new QLabel("Hex Color");
    hexLabel->setStyleSheet("font-size: 10px; color: #aaa;");
    mainLayout->addWidget(hexLabel);

    m_hexInput = new QLineEdit();
    m_hexInput->setPlaceholderText("#000000");
    m_hexInput->setMaxLength(7);
    m_hexInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #2d2d2d;"
        "   border: 2px solid #3a3a3a;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "   color: white;"
        "   font-family: monospace;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #2a82da;"
        "}"
        );
    connect(m_hexInput, &QLineEdit::editingFinished, this, &ColorPicker::onHexChanged);
    mainLayout->addWidget(m_hexInput);

    // Color Palettes
    QLabel *paletteLabel = new QLabel("Color Palettes");
    paletteLabel->setStyleSheet("font-size: 10px; color: #aaa; margin-top: 4px;");
    mainLayout->addWidget(paletteLabel);

    m_paletteCombo = new QComboBox();
    m_paletteCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: #2d2d2d;"
        "   border: 2px solid #3a3a3a;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "   color: white;"
        "   font-size: 10px;"
        "}"
        "QComboBox:hover {"
        "   border-color: #2a82da;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #2d2d2d;"
        "   color: white;"
        "   selection-background-color: #2a82da;"
        "}"
    );

    loadPalettes();  // Load palette data

    // Populate palette dropdown
    for (const QString &paletteName : m_palettes.keys()) {
        m_paletteCombo->addItem(paletteName);
    }

    connect(m_paletteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColorPicker::onPaletteChanged);

    mainLayout->addWidget(m_paletteCombo);

    // Palette colors display
    m_paletteWidget = new QWidget();
    QGridLayout *paletteGrid = new QGridLayout(m_paletteWidget);
    paletteGrid->setSpacing(4);
    paletteGrid->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_paletteWidget);

    // Initialize palette display
    updatePaletteDisplay();

    // Recent colors
    QLabel *recentLabel = new QLabel("Recent Colors");
    recentLabel->setStyleSheet("font-size: 10px; color: #aaa; margin-top: 8px;");
    mainLayout->addWidget(recentLabel);

    m_recentColorsWidget = new QWidget();
    QGridLayout *recentGrid = new QGridLayout(m_recentColorsWidget);
    recentGrid->setSpacing(4);
    recentGrid->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(m_recentColorsWidget);

    // Texture selector
    QLabel *textureLabel = new QLabel("Brush Texture");
    textureLabel->setStyleSheet("font-size: 10px; color: #aaa; margin-top: 8px;");
    mainLayout->addWidget(textureLabel);

    m_textureCombo = new QComboBox();
    m_textureCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: #2d2d2d;"
        "   border: 2px solid #3a3a3a;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "   color: white;"
        "}"
        "QComboBox:hover {"
        "   border-color: #2a82da;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #2d2d2d;"
        "   color: white;"
        "   selection-background-color: #2a82da;"
        "}"
        );

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
    m_hexInput->blockSignals(true);
    m_hexInput->setText(m_currentColor.name());
    m_hexInput->blockSignals(false);

    m_opacitySlider->blockSignals(true);
    m_opacitySlider->setValue(m_currentColor.alpha() * 100 / 255);
    m_opacitySlider->blockSignals(false);

    QGridLayout *grid = qobject_cast<QGridLayout*>(m_recentColorsWidget->layout());
    if (grid) {
        while (grid->count() > 0) {
            QLayoutItem *item = grid->takeAt(0);
            delete item->widget();
            delete item;
        }

        for (int i = 0; i < m_recentColors.size() && i < 16; ++i) {
            QPushButton *colorBtn = new QPushButton();
            colorBtn->setFixedSize(28, 28);
            colorBtn->setCursor(Qt::PointingHandCursor);
            colorBtn->setStyleSheet(QString(
                                        "QPushButton {"
                                        "   background-color: %1;"
                                        "   border: 2px solid #555;"
                                        "   border-radius: 4px;"
                                        "}"
                                        "QPushButton:hover {"
                                        "   border-color: #2a82da;"
                                        "}"
                                        ).arg(m_recentColors[i].name()));

            connect(colorBtn, &QPushButton::clicked, this, [this, i]() {
                setColor(m_recentColors[i]);
                emit colorChanged(m_currentColor);
            });

            grid->addWidget(colorBtn, i / 8, i % 8);
        }
    }
}

QColor ColorPicker::currentColor() const
{
    return m_currentColor;
}

void ColorPicker::setColor(const QColor &color)
{
    m_currentColor = color;
    m_colorWheel->setColor(color);
    updateUI();
}

void ColorPicker::onWheelColorChanged(const QColor &color)
{
    m_currentColor = color;
    m_currentColor.setAlpha(m_opacitySlider->value() * 255 / 100);
    updateUI();
    addToRecentColors(m_currentColor);
    emit colorChanged(m_currentColor);
}

void ColorPicker::onOpacityChanged(int value)
{
    m_currentColor.setAlpha(value * 255 / 100);
    emit colorChanged(m_currentColor);
}

void ColorPicker::onHexChanged()
{
    QString hex = m_hexInput->text();
    if (QColor::isValidColorName(hex)) {
        QColor color(hex);
        color.setAlpha(m_currentColor.alpha());
        setColor(color);
        addToRecentColors(color);
        emit colorChanged(m_currentColor);
    }
}

void ColorPicker::onRecentColorClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        int index = btn->property("colorIndex").toInt();
        if (index >= 0 && index < m_recentColors.size()) {
            setColor(m_recentColors[index]);
            emit colorChanged(m_currentColor);
        }
    }
}

void ColorPicker::addToRecentColors(const QColor &color)
{
    m_recentColors.removeAll(color);
    m_recentColors.prepend(color);
    if (m_recentColors.size() > 16) {
        m_recentColors.resize(16);
    }
    updateUI();
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
