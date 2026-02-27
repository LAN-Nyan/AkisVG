#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    // 1. High DPI scaling is ENABLED by default in Qt 6.
    // Manual AA_EnableHighDpiScaling calls are actually ignored in Qt 6.
    
    // 2. Fix for Windows "Tiny/Huge" scaling inconsistency:
    // We set the policy BEFORE the QApplication constructor.
#ifdef Q_OS_WIN
    // 'Round' can cause things to snap to 100% (too small) or 200% (too big).
    // 'PassThrough' allows Qt to use the exact Windows scaling (e.g., 125%).
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);

    // 3. Metadata
    app.setOrganizationName("AkisVG");
    app.setApplicationName("AkisVG");
    app.setApplicationVersion("1.0.0");

    // 4. Style: Fusion is the best cross-platform dark-theme base.
    // It handles custom color palettes much better than the native 'Windows' style.
    app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow window;
    window.show();

    return app.exec();
}
