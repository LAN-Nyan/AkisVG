#include "mainwindow.h"
#include "ui/startupdialog.h"
#include "core/project.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // ── DPI scaling ──────────────────────────────────────────────────────────
    // AA_EnableHighDpiScaling is deprecated in Qt 6 (always on), but harmless.
    // On Windows round to nearest integer scale; on Linux/macOS pass through
    // fractional factors so HiDPI monitors (e.g. 1.5×, 1.25×) look correct.
#ifdef Q_OS_WIN
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Round);
#else
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);
    app.setOrganizationName("AkisVG");
    app.setOrganizationDomain("akisvg by LAN-Nyan");
    app.setApplicationName("AkisVG");
    app.setApplicationVersion("1.0.0");

    // Apply Fusion style on all platforms — this ensures consistent widget
    // geometry, font metrics, and scaling behaviour regardless of desktop theme.
    // Without this, native GTK/KDE themes can produce mis-sized widgets on HiDPI.
    app.setStyle(QStyleFactory::create("Fusion"));

    // ── Startup dialog ───────────────────────────────────────────────────────
    StartupDialog startup;
    if (startup.exec() != QDialog::Accepted) {
        return 0; // user closed the dialog → quit
    }

    // ── Build main window ────────────────────────────────────────────────────
    MainWindow window;

    if (startup.action() == StartupDialog::Action::OpenProject) {
        // Open existing project file
        window.show();
        window.openProjectFile(startup.openPath());
    } else {
        // Apply new-project settings before showing the window
        window.applyStartupSettings(
            startup.projectName(),
            startup.canvasWidth(),
            startup.canvasHeight(),
            startup.fps());
        window.show();
    }

    return app.exec();
}
