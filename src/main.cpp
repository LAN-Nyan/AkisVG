#include <QApplication>
#include <QStyleFactory>
#include <QIcon>
#include <QPalette>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // High DPI support is handled automatically in Qt 6.
    // Attributes like AA_EnableHighDpiScaling are now default/deprecated.

    QApplication app(argc, argv);

    // --- APPLICATION ICON ---
    // Uses the alias "app_icon.svg" under the "/icons" prefix from your .qrc
    app.setWindowIcon(QIcon(":/tools/assets/Apple.png"));

    // Set application metadata
    app.setApplicationName("AkisVG");
    app.setApplicationVersion("0.0.3");
    app.setOrganizationName("Lannyan Software");

    // --- LOOK AND FEEL ---
    // Set dark fusion style for a consistent modern look across platforms
    if (QStyleFactory::keys().contains("Fusion")) {
        app.setStyle(QStyleFactory::create("Fusion"));
    }

    // Configure Dark Palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    // Disabled state colors (optional but recommended for UX)
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

    app.setPalette(darkPalette);

    // --- LAUNCH ---
    MainWindow window;
    window.show();

    return app.exec();
}
