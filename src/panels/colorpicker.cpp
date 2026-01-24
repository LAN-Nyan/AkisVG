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

    // Draw hue wheel
    for (int i = 0; i < 360; ++i) {
        qreal angle = i * M_PI / 180.0;
        QColor hueColor = QColor::fromHsvF(i / 360.0, 1.0, 1.0);

        QPainterPath path;
        path.moveTo(m_center);
        path.arcTo(m_center.x() - m_outerRadius, m_center.y() - m_outerRadius,
                   m_outerRadius * 2, m_outerRadius * 2, i, 1);
        path.lineTo(m_center);

        QPainterPath innerPath;
        innerPath.addEllipse(m_center, m_innerRadius, m_innerRadius);

        path = path.subtracted(innerPath);

        painter.fillPath(path, hueColor);
    }

    // Draw SV triangle
    qreal hue = m_currentColor.hueF();
    if (hue < 0) hue = 0;

    qreal angle = hue * 2 * M_PI;
    QPointF p1 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95;
    angle += 2 * M_PI / 3;
    QPointF p2 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95;
    angle += 2 * M_PI / 3;
    QPointF p3 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95;

    // Draw gradient triangle
    QLinearGradient satGradient(p2, p3);
    satGradient.setColorAt(0, Qt::white);
    satGradient.setColorAt(1, QColor::fromHsvF(hue, 1.0, 1.0));

    QPainterPath triangle;
    triangle.moveTo(p1);
    triangle.lineTo(p2);
    triangle.lineTo(p3);
    triangle.closeSubpath();

    painter.fillPath(triangle, satGradient);

    // Overlay black gradient for value
    QLinearGradient valGradient(p1, (p2 + p3) / 2);
    valGradient.setColorAt(0, QColor(0, 0, 0, 0));
    valGradient.setColorAt(1, QColor(0, 0, 0, 255));
    painter.fillPath(triangle, valGradient);

    // Draw current color indicator
    QPointF svPoint = svToPoint(m_currentColor.saturationF(), m_currentColor.valueF());
    painter.setPen(QPen(Qt::white, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(svPoint, 6, 6);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(svPoint, 6, 6);

    // Draw hue indicator
    QPointF huePoint = hueToPoint(hue);
    painter.setPen(QPen(Qt::white, 3));
    painter.drawEllipse(huePoint, 8, 8);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(huePoint, 8, 8);
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
    qreal dist = QLineF(m_center, pos).length();

    if (m_isSelectingHue) {
        qreal angle = qAtan2(delta.y(), delta.x());
        qreal hue = angle / (2 * M_PI);
        if (hue < 0) hue += 1.0;

        m_currentColor.setHsvF(hue, m_currentColor.saturationF(), m_currentColor.valueF());
        emit colorChanged(m_currentColor);
        update();
    } else if (m_isSelectingSV) {
        qreal hue = m_currentColor.hueF();
        if (hue < 0) hue = 0;

        qreal angle = hue * 2 * M_PI;
        QPointF p1 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95; // Black
        angle += 2 * M_PI / 3;
        QPointF p2 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95; // White
        angle += 2 * M_PI / 3;
        QPointF p3 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95; // Pure Color

        // --- THE FIX ---
        // Use vector projections to find Saturation and Value relative to the triangle's orientation
        QLineF baseLine(p2, p3);       // Line from White to Pure Hue
        QLineF valLine(p1, (p2+p3)/2); // Line from Black to the center of the White-Hue edge

        // Calculate Value: 0 at p1 (Black), 1 at the p2-p3 edge
        // We project the mouse position onto the line that bisects the triangle from the black point
        qreal val = 1.0 - qBound(0.0, QLineF(pos, (p2+p3)/2).length() / valLine.length(), 1.0);

        // Calculate Saturation: 0 at p2 (White), 1 at p3 (Pure Hue)
        // We project the mouse position onto the baseline
        qreal sat = qBound(0.0, QLineF(p2, pos).length() / baseLine.length(), 1.0);

        m_currentColor.setHsvF(hue, sat, val);
        emit colorChanged(m_currentColor);
        update();
    }
}

QPointF ColorWheel::hueToPoint(qreal hue) const
{
    qreal angle = hue * 2 * M_PI;
    qreal radius = (m_outerRadius + m_innerRadius) / 2;
    return m_center + QPointF(qCos(angle), qSin(angle)) * radius;
}

QPointF ColorWheel::svToPoint(qreal saturation, qreal value) const
{
    qreal hue = m_currentColor.hueF();
    if (hue < 0) hue = 0;

    qreal angle = hue * 2 * M_PI;
    QPointF p1 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95;
    angle += 2 * M_PI / 3;
    QPointF p2 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95;
    angle += 2 * M_PI / 3;
    QPointF p3 = m_center + QPointF(qCos(angle), qSin(angle)) * m_innerRadius * 0.95;

    QPointF whitePoint = p2;
    QPointF colorPoint = p3;
    QPointF blackPoint = p1;

    QPointF satPoint = whitePoint + (colorPoint - whitePoint) * saturation;
    QPointF finalPoint = satPoint + (blackPoint - satPoint) * (1.0 - value);

    return finalPoint;
}

// ============= ColorPicker Implementation =============

ColorPicker::ColorPicker(QWidget *parent)
    : QWidget(parent)
    , m_currentColor(Qt::black)
{
    setupUI();

    // Add some default recent colors
    m_recentColors << Qt::black << Qt::white << Qt::red << Qt::green
                   << Qt::blue << Qt::yellow << Qt::cyan << Qt::magenta;
    updateUI();
}

void ColorPicker::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // Title
    QLabel *title = new QLabel("COLOR PICKER");
    title->setStyleSheet("font-weight: bold; font-size: 11px; color: #888; letter-spacing: 1px;");
    mainLayout->addWidget(title);

    // Color wheel
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
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "       stop:0 transparent, stop:1 " + m_currentColor.name() + ");"
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

    // Recent colors
    QLabel *recentLabel = new QLabel("Recent Colors");
    recentLabel->setStyleSheet("font-size: 10px; color: #aaa;");
    mainLayout->addWidget(recentLabel);

    m_recentColorsWidget = new QWidget();
    QGridLayout *recentGrid = new QGridLayout(m_recentColorsWidget);
    recentGrid->setSpacing(4);
    recentGrid->setContentsMargins(0, 0, 0, 0);
    m_recentColorsWidget->setLayout(recentGrid);

    mainLayout->addWidget(m_recentColorsWidget);
    mainLayout->addStretch();
}

void ColorPicker::updateUI()
{
    m_hexInput->blockSignals(true);
    m_hexInput->setText(m_currentColor.name());
    m_hexInput->blockSignals(false);

    m_opacitySlider->blockSignals(true);
    m_opacitySlider->setValue(m_currentColor.alpha() * 100 / 255);
    m_opacitySlider->blockSignals(false);

    // Update recent colors grid
    QGridLayout *grid = qobject_cast<QGridLayout*>(m_recentColorsWidget->layout());
    if (grid) {
        // Clear existing
        while (grid->count() > 0) {
            QLayoutItem *item = grid->takeAt(0);
            delete item->widget();
            delete item;
        }

        // Add recent colors
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
    if (QColor::isValidColor(hex)) {
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
