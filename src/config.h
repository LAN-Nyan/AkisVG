#ifndef ICONCONFIG_H
#define ICONCONFIG_H

#include <QString>
#include <QMap>

/**
 * Icon Configuration
 *
 * This file contains all icon paths used in the application.
 * Edit these paths to change which icons are used for each tool.
 *
 * Icon files should be placed in the assets/icons/ directory.
 * You can use any SVG file - just update the path here.
 */

namespace IconConfig {

// Base path for icons (relative to executable)
inline QString iconBasePath() {
    return "assets/icons/";
}

// Tool icon paths - EDIT THESE to change which icon is used
inline QMap<QString, QString> toolIcons() {
    return {
            // Tool Name         Icon Filename
            {"select",          "select.svg"},
            {"pencil",          "pencil.svg"},
            {"brush",           "assets/Brush.svg"},
            {"eraser",          "eraser.svg"},
            {"fill",            "fill.svg"},
            {"rectangle",       "rectangle.svg"},
            {"ellipse",         "ellipse.svg"},
            {"text",            "text.svg"},

            // Add more tools here as needed:
            // {"your_tool",    "your_icon.svg"},
            };
}

// Get full path for a tool icon
inline QString getToolIconPath(const QString &toolName) {
    QMap<QString, QString> icons = toolIcons();
    if (icons.contains(toolName)) {
        return iconBasePath() + icons[toolName];
    }
    return "";  // Return empty if not found
}

// Alternative icon sets (optional - for future use)
namespace AlternativeIcons {
inline QString iconBasePath() {
    return "assets/icons/alternative/";
}

// You can define alternative icon sets here
// For example, a "minimal" style or "outlined" style
}

} // namespace IconConfig

#endif // ICONCONFIG_H
