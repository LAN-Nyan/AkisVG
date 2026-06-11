#include "mainwindow.h"
#include "ui/startupdialog.h"
#include "utils/crashguard.h"
#include "utils/debuglog.h"
#include <QApplication>
#include <QCoreApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // DPI scaling
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
    installCrashGuard();
    AKIS_LOG(App, QStringLiteral("AkisVG starting (pid %1)").arg(QCoreApplication::applicationPid()));

    app.setOrganizationName("AkisVG");
    app.setOrganizationDomain("akisvg by LAN-Nyan");
    app.setApplicationName("AkisVG");
    app.setApplicationVersion("1.0.0");

    // Apply Fusion style on all platforms — this ensures consistent widget
    app.setStyle(QStyleFactory::create("Fusion"));

    // ── Startup dialog ───────────────────────────────────────────────────────
    StartupDialog startup;
    if (startup.exec() != QDialog::Accepted) {
        AKIS_LOG(App, QStringLiteral("Startup cancelled — exiting"));
        return 0;
    }
    AKIS_LOG(App, QStringLiteral("Startup accepted"));

    // ── Build main window ────────────────────────────────────────────────────
    MainWindow window;

    if (startup.action() == StartupDialog::Action::OpenProject) {
        AKIS_LOG(IO, QStringLiteral("Opening project: %1").arg(startup.openPath()));
        window.show();
        window.openProjectFile(startup.openPath());
    } else {
        AKIS_LOG(Project, QStringLiteral("New project %1x%2 @%3fps")
                            .arg(startup.canvasWidth()).arg(startup.canvasHeight()).arg(startup.fps()));
        // Apply new-project settings before showing the window
        window.applyStartupSettings(
            startup.projectName(),
            startup.canvasWidth(),
            startup.canvasHeight(),
            startup.fps());
        window.show();
    }

    AKIS_LOG(App, QStringLiteral("Entering event loop"));
    const int code = app.exec();
    AKIS_LOG(App, QStringLiteral("Event loop finished (code %1)").arg(code));
    return code;
}
