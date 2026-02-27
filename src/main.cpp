#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // ===== FIX FOR WINDOWS DPI SCALING ISSUES =====
    // Set DPI scaling attributes BEFORE creating QApplication
    #ifdef Q_OS_WIN
        // Enable DPI awareness
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        // Use Round scaling policy for better consistency on Windows
        // Changed from PassThrough to Round to fix "UI too big" issue
        QApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::Round);
    #else
        // On other platforms, use PassThrough for accuracy
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    #endif

    QApplication app(argc, argv);

    // Set application metadata
    app.setOrganizationName("AkisVG");
    app.setOrganizationDomain("akisvg.com");
    app.setApplicationName("AkisVG");
    app.setApplicationVersion("1.0.0");

    // Use native style as base for better OS integration
    #ifdef Q_OS_WIN
        app.setStyle(QStyleFactory::create("Fusion"));
    #endif

    MainWindow window;
    window.show();

    return app.exec();
}
