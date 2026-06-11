#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <QObject>
#include <QString>
#include <QMutex>

class QTextEdit;

/** Central verbose logger — always on unless AKISVG_QUIET=1. */
class DebugLog : public QObject
{
    Q_OBJECT

public:
    enum class Category {
        App,
        Canvas,
        Tool,
        Undo,
        Layer,
        Project,
        Frame,
        Symbol,
        Asset,
        IO,
        UI,
        Crash,
        Event
    };
    Q_ENUM(Category)

    static DebugLog &instance();

    void log(Category cat, const QString &message,
             const char *file = nullptr, int lineNo = 0, const char *func = nullptr);

    void setPanel(QTextEdit *panel);
    QString logFilePath() const;

    static QString categoryName(Category cat);

signals:
    void lineLogged(const QString &formattedLine);

private:
    explicit DebugLog(QObject *parent = nullptr);
    void writeToFile(const QString &line);

    QMutex m_mutex;
    QTextEdit *m_panel = nullptr;
    QString m_logPath;
    bool m_quiet = false;
};

#define AKIS_LOG(cat, msg) \
    DebugLog::instance().log(DebugLog::Category::cat, (msg), __FILE__, __LINE__, Q_FUNC_INFO)

#define AKIS_LOGF(cat, fmt, ...) \
    AKIS_LOG(cat, QStringLiteral(fmt).arg(__VA_ARGS__))

#endif
