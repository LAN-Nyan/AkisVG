#ifndef ICONCONFIG_H
#define ICONCONFIG_H

#include <QString>
#include <QMap>

namespace IconConfig {

// Points to the prefix defined in your .qrc file
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
            {"addframe",        "addframe.svg"},
            {"removeframe",     "removeframe.svg"}
    };
}

inline QString getToolIconPath(const QString &toolName) {
    QMap<QString, QString> icons = toolIcons();
    if (icons.contains(toolName)) {
        return iconBasePath() + icons[toolName];
    }
    return ""; 
}

} // namespace IconConfig

#endif // ICONCONFIG_H
