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

    // Scale the button height with UI scale — MUST use setFixedHeight not CSS
    // because QSS min-height cannot override setFixedHeight.
    setFixedHeight(sc(36));
    setMinimumWidth(sc(48));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setToolTip(QString("%1 (%2)").arg(toolName, shortcut));

    if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
        m_svgRenderer = new QSvgRenderer(iconPath, this);
    }

    applyTheme();
}

void ToolButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QColor iconColor;
    if (isChecked())        iconColor = Qt::white;
    else if (m_hovered)     iconColor = QColor(220, 220, 220);
    else                    iconColor = QColor(180, 180, 180);

    // Icon size scales with UI scale
    int iconSize = sc(20);
    QRect iconRect;
    if (m_hovered)
        iconRect = QRect(sc(10), (height() - iconSize) / 2, iconSize, iconSize);
    else
        iconRect = QRect((width() - iconSize) / 2, (height() - iconSize) / 2, iconSize, iconSize);

    if (m_svgRenderer && m_svgRenderer->isValid()) {
        QPixmap pixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);

        QPainter svgPainter(&pixmap);
        svgPainter.setRenderHint(QPainter::Antialiasing);
        m_svgRenderer->render(&svgPainter);
        svgPainter.end();

        QPainter colorPainter(&pixmap);
        colorPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        colorPainter.fillRect(pixmap.rect(), iconColor);
        colorPainter.end();

        painter.drawPixmap(iconRect, pixmap);
    }

    if (m_hovered) {
        painter.setPen(iconColor);
        QFont font = painter.font();
        font.setPointSizeF(9.0 * uiScale());
        font.setWeight(isChecked() ? QFont::Bold : QFont::Normal);
        painter.setFont(font);

        QRect textRect(iconRect.right() + sc(8), 0, width() - iconRect.right() - sc(16), height());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_toolName);
    }
}

void ToolButton::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    setMinimumWidth(sc(140));
    update();
    QPushButton::enterEvent(event);
}

void ToolButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    setMinimumWidth(sc(48));
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
