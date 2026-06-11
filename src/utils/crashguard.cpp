#include "crashguard.h"
#include "debuglog.h"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <csignal>
#include <cstdlib>
#include <exception>

#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
#include <execinfo.h>
#include <unistd.h>
#endif

namespace {

QtMessageHandler s_prevHandler = nullptr;

void showCrashDialog(const QString &summary)
{
    if (!QApplication::instance())
        return;
    QTimer::singleShot(0, qApp, [summary]() {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("AkisVG — Error"),
            summary + QStringLiteral("\n\nDetails were written to:\n")
                + DebugLog::instance().logFilePath(),
            QMessageBox::Ok);
    });
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    DebugLog::Category cat = DebugLog::Category::App;
    QString prefix;
    switch (type) {
    case QtDebugMsg:    prefix = QStringLiteral("QtDebug"); break;
    case QtInfoMsg:     prefix = QStringLiteral("QtInfo"); break;
    case QtWarningMsg:  prefix = QStringLiteral("QtWarning"); cat = DebugLog::Category::Crash; break;
    case QtCriticalMsg: prefix = QStringLiteral("QtCritical"); cat = DebugLog::Category::Crash; break;
    case QtFatalMsg:    prefix = QStringLiteral("QtFatal"); cat = DebugLog::Category::Crash; break;
    }

    const QString full = QStringLiteral("%1: %2").arg(prefix, msg);
    DebugLog::instance().log(cat, full, ctx.file, ctx.line, ctx.function);

    if (type == QtFatalMsg) {
        showCrashDialog(QStringLiteral("A fatal Qt error occurred:\n") + msg);
        if (s_prevHandler)
            s_prevHandler(type, ctx, msg);
        else
            abort();
        return;
    }

    if (s_prevHandler)
        s_prevHandler(type, ctx, msg);
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
void posixSignalHandler(int sig)
{
    const char *name = "UNKNOWN";
    switch (sig) {
    case SIGSEGV: name = "SIGSEGV"; break;
    case SIGABRT: name = "SIGABRT"; break;
    case SIGFPE:  name = "SIGFPE"; break;
    case SIGILL:  name = "SIGILL"; break;
    case SIGBUS:  name = "SIGBUS"; break;
    }

    DebugLog::instance().log(
        DebugLog::Category::Crash,
        QStringLiteral("Caught signal %1 (%2) — backtrace follows").arg(sig).arg(name));

    void *frames[64];
    const int n = backtrace(frames, 64);
    char **symbols = backtrace_symbols(frames, n);
    if (symbols) {
        for (int i = 0; i < n; ++i)
            DebugLog::instance().log(DebugLog::Category::Crash, QString::fromLocal8Bit(symbols[i]));
        free(symbols);
    }

    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

class GlobalExceptionCatcher
{
public:
    GlobalExceptionCatcher() { std::set_terminate(terminateHandler); }

    static void terminateHandler()
    {
        try {
            if (auto e = std::current_exception()) {
                std::rethrow_exception(e);
            }
        } catch (const std::exception &e) {
            DebugLog::instance().log(DebugLog::Category::Crash,
                                     QStringLiteral("std::terminate: %1").arg(e.what()));
        } catch (...) {
            DebugLog::instance().log(DebugLog::Category::Crash,
                                     QStringLiteral("std::terminate: unknown exception"));
        }
        std::abort();
    }
};

GlobalExceptionCatcher s_globalExceptionCatcher;

} // namespace

void installCrashGuard()
{
    s_prevHandler = qInstallMessageHandler(qtMessageHandler);

#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
    signal(SIGSEGV, posixSignalHandler);
    signal(SIGABRT, posixSignalHandler);
    signal(SIGFPE,  posixSignalHandler);
    signal(SIGILL,  posixSignalHandler);
    signal(SIGBUS,  posixSignalHandler);
#endif

    AKIS_LOG(App, QStringLiteral("Crash guard installed — log file: %1")
                        .arg(DebugLog::instance().logFilePath()));
}
