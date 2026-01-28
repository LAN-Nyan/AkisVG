#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // High DPI support is automatic in Qt6
    // QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);  // Deprecated in Qt6
    // QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);     // Deprecated in Qt6
    
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("Lumina Studio");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Lumina");
    
    // Set dark fusion style for modern look
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Dark palette
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
    
    app.setPalette(darkPalette);
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
