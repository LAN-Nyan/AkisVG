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

// Get full path for a tool icon using Qt resources
inline QString getToolIconPath(const QString &toolName) {
    // Map tool names to resource paths
    static QMap<QString, QString> iconMap = {
        {"select", ":/icons/select.svg"},  // Will need to be added
        {"pencil", ":/icons/pencil.svg"},
        {"brush", ":/icons/brush.svg"},
        {"eraser", ":/icons/eraser.svg"},
        {"fill", ":/icons/fill.svg"},
        {"rectangle", ":/icons/rectangle.svg"},
        {"ellipse", ":/icons/ellipse.svg"},
        {"text", ":/icons/text.svg"},  // Will need to be added
        {"line", ":/icons/line.svg"},
        {"eyedropper", ":/icons/eyedropper.svg"},
        {"blend", ":/icons/brush.svg"},  // Reuse brush icon for now
    };
    
    return iconMap.value(toolName, "");
}

// Legacy functions kept for compatibility
inline QString iconBasePath() {
    return ":/icons/";
}

inline QMap<QString, QString> toolIcons() {
    return {
        {"select",          "select.svg"},
        {"pencil",          "pencil.svg"},
        {"brush",           "brush.svg"},
        {"eraser",          "eraser.svg"},
        {"fill",            "fill.svg"},
        {"rectangle",       "rectangle.svg"},
        {"ellipse",         "ellipse.svg"},
        {"text",            "text.svg"},
    };
}

// Alternative icon sets (optional - for future use)
namespace AlternativeIcons {
inline QString iconBasePath() {
    return ":/icons/alternative/";
}

// You can define alternative icon sets here
// For example, a "minimal" style or "outlined" style
}

} // namespace IconConfig

#endif // ICONCONFIG_H
