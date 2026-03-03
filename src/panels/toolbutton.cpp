#include "toolbutton.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFile>
#include "utils/thememanager.h"

ToolButton::ToolButton(const QString &iconPath, const QString &toolName, const QString &shortcut, QWidget *parent)
    : QPushButton(parent)
    , m_svgRenderer(nullptr)
    , m_toolName(toolName)
    , m_shortcut(shortcut)
    , m_hovered(false)
{
    setCheckable(true);
    setFixedHeight(36);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setToolTip(QString("%1 (%2)").arg(toolName, shortcut));

    if (QFile::exists(iconPath)) {
        m_svgRenderer = new QSvgRenderer(iconPath, this);
    }

    // Apply theme-aware stylesheet from the start
    applyTheme();
}

void ToolButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Determine icon color based on state
    QColor iconColor;
    if (isChecked()) {
        iconColor = Qt::white;
    } else if (m_hovered) {
        iconColor = QColor(220, 220, 220);
    } else {
        iconColor = QColor(180, 180, 180);
    }

    // Calculate icon position (centered if not hovered, left-aligned if hovered)
    int iconSize = 24;
    QRect iconRect;

    if (m_hovered) {
        // Left-aligned when showing text
        iconRect = QRect(12, (height() - iconSize) / 2, iconSize, iconSize);
    } else {
        // Centered when icon only
        iconRect = QRect((width() - iconSize) / 2, (height() - iconSize) / 2, iconSize, iconSize);
    }

    // Render SVG icon
    if (m_svgRenderer && m_svgRenderer->isValid()) {
        // Create a temporary pixmap to render the SVG with color
        QPixmap pixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);

        QPainter svgPainter(&pixmap);
        svgPainter.setRenderHint(QPainter::Antialiasing);
        m_svgRenderer->render(&svgPainter);
        svgPainter.end();

        // Apply color tint
        QPainter colorPainter(&pixmap);
        colorPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        colorPainter.fillRect(pixmap.rect(), iconColor);
        colorPainter.end();

        painter.drawPixmap(iconRect, pixmap);
    }

    // Draw tool name on hover
    if (m_hovered) {
        painter.setPen(iconColor);
        QFont font = painter.font();
        font.setPointSize(10);
        font.setWeight(isChecked() ? QFont::Bold : QFont::Normal);
        painter.setFont(font);

        QRect textRect(iconRect.right() + 8, 0, width() - iconRect.right() - 16, height());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_toolName);
    }
}

void ToolButton::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    setMinimumWidth(140);  // Expand to show text
    update();
    QPushButton::enterEvent(event);
}

void ToolButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    setMinimumWidth(48);   // Collapse to icon only
    update();
    QPushButton::leaveEvent(event);
}

void ToolButton::applyTheme()
{
    const ThemeColors &t = theme();
    setStyleSheet(QString(
        "ToolButton { "
        "  border: 1px solid %1; "
        "  border-radius: 4px; "
        "  background-color: %2; "
        "  margin: 0px 2px; "
        "} "
        "ToolButton:hover { "
        "  background-color: %3; "
        "  border-color: %4; "
        "} "
        "ToolButton:checked { "
        "  background-color: %5; "
        "  border: 1px solid %6; "
        "}"
    ).arg(t.bg2, t.bg1, t.bg2, t.bg3, t.accent, t.accentHover));
}
