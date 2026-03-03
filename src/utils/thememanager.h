#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QColor>

struct ThemeColors {
    QString accent;       // e.g. "#c0392b"
    QString accentHover;  // e.g. "#e05241"
    QString bg0;          // deepest bg  e.g. "#0a0a0a"
    QString bg1;          // panel bg    e.g. "#1a1a1a"
    QString bg2;          // widget bg   e.g. "#2d2d2d"
    QString bg3;          // lighter     e.g. "#3a3a3a"
    QString bg4;          // input bg    e.g. "#1e1e1e"

    // Convenience: QColor versions for QPainter use
    QColor accentColor()      const { return QColor(accent); }
    QColor accentHoverColor() const { return QColor(accentHover); }
    QColor bg0Color()         const { return QColor(bg0); }
    QColor bg1Color()         const { return QColor(bg1); }
    QColor bg2Color()         const { return QColor(bg2); }
    QColor bg3Color()         const { return QColor(bg3); }
    QColor bg4Color()         const { return QColor(bg4); }
};

class ThemeManager
{
public:
    static ThemeManager& instance() {
        static ThemeManager inst;
        return inst;
    }

    // 0=Studio Grey (timeline look, default)
    // 1=Red & Black  2=Blue & Black  3=Grey & Blue  4=Grey & Red
    void setTheme(int index) {
        static const ThemeColors themes[5] = {
            // Studio Grey — matches timeline widget palette
            { "#c0392b", "#e05241", "#1e1e1e", "#282828", "#333333", "#3a3a3a", "#242424" },
            // Red & Black — the original near-black panel look
            { "#c0392b", "#e05241", "#0a0a0a", "#1a1a1a", "#2d2d2d", "#3a3a3a", "#0f0f0f" },
            // Blue & Black
            { "#2a82da", "#3a92ea", "#0a0a0a", "#1a1a1a", "#2d2d2d", "#3a3a3a", "#1e1e1e" },
            // Grey & Blue
            { "#2a82da", "#3a92ea", "#2b2b2b", "#3a3a3a", "#474747", "#555555", "#333333" },
            // Grey & Red
            { "#c0392b", "#e05241", "#2b2b2b", "#3a3a3a", "#474747", "#555555", "#333333" },
        };
        if (index >= 0 && index < 5)
            m_current = themes[index];
        m_index = index;
    }

    const ThemeColors& colors() const { return m_current; }
    int currentIndex()           const { return m_index; }

private:
    ThemeManager() { setTheme(0); }
    ThemeColors m_current;
    int m_index = 0;
};

// Shorthand accessor used everywhere
inline const ThemeColors& theme() { return ThemeManager::instance().colors(); }

#endif // THEMEMANAGER_H
