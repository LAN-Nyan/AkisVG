#include "debuglog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextEdit>
#include <QScrollBar>
#include <QCoreApplication>
#include <cstdio>

DebugLog &DebugLog::instance()
{
    static DebugLog log;
    return log;
}

DebugLog::DebugLog(QObject *parent)
    : QObject(parent)
{
    m_quiet = qEnvironmentVariableIsSet("AKISVG_QUIET");
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_logPath = dir + "/debug.log";

    if (!m_quiet) {
        QFile f(m_logPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            f.write(QString("\n--- AkisVG session %1 ---\n")
                        .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                        .toUtf8());
            f.close();
        }
    }
}

QString DebugLog::categoryName(Category cat)
{
    switch (cat) {
    case Category::App:    return QStringLiteral("APP");
    case Category::Canvas:  return QStringLiteral("CANVAS");
    case Category::Tool:    return QStringLiteral("TOOL");
    case Category::Undo:    return QStringLiteral("UNDO");
    case Category::Layer:   return QStringLiteral("LAYER");
    case Category::Project: return QStringLiteral("PROJECT");
    case Category::Frame:   return QStringLiteral("FRAME");
    case Category::Symbol:  return QStringLiteral("SYMBOL");
    case Category::Asset:   return QStringLiteral("ASSET");
    case Category::IO:      return QStringLiteral("IO");
    case Category::UI:      return QStringLiteral("UI");
    case Category::Crash:   return QStringLiteral("CRASH");
    case Category::Event:   return QStringLiteral("EVENT");
    }
    return QStringLiteral("???");
}

void DebugLog::setPanel(QTextEdit *panel)
{
    QMutexLocker lock(&m_mutex);
    m_panel = panel;
}

QString DebugLog::logFilePath() const
{
    return m_logPath;
}

void DebugLog::writeToFile(const QString &line)
{
    if (m_quiet) return;
    QFile f(m_logPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        f.write(line.toUtf8() + '\n');
}

void DebugLog::log(Category cat, const QString &message,
                   const char *file, int lineNo, const char *func)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    QString loc;
    if (file && func)
        loc = QStringLiteral(" [%1:%2 %3]").arg(file).arg(lineNo).arg(func);
    const QString formatted = QStringLiteral("%1 [%2]%3 %4")
                                  .arg(ts, categoryName(cat), loc, message);

    {
        QMutexLocker lock(&m_mutex);
        fprintf(stderr, "%s\n", formatted.toLocal8Bit().constData());
        fflush(stderr);
        writeToFile(formatted);
        if (m_panel) {
            m_panel->append(formatted);
            if (auto *bar = m_panel->verticalScrollBar())
                bar->setValue(bar->maximum());
        }
    }
    emit lineLogged(formatted);
}
