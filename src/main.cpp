#include "mainwindow.h"
#include "ui/startupdialog.h"
#include "core/project.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // ── DPI scaling ──────────────────────────────────────────────────────────
#ifdef Q_OS_WIN
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Round);
#else
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);
    app.setOrganizationName("AkisVG");
    app.setOrganizationDomain("akisvg by LAN-Nyan");
    app.setApplicationName("AkisVG");
    app.setApplicationVersion("1.0.0");

#ifdef Q_OS_WIN
    app.setStyle(QStyleFactory::create("Fusion"));
#endif

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
