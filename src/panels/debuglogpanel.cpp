#include "debuglogpanel.h"
#include "utils/debuglog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

DebugLogPanel::DebugLogPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto *toolbar = new QHBoxLayout();
    auto *clearBtn = new QPushButton(tr("Clear"));
    auto *openBtn  = new QPushButton(tr("Open Log File"));
    toolbar->addWidget(clearBtn);
    toolbar->addWidget(openBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setStyleSheet(
        "QTextEdit { background:#111; color:#8f8; font-family:monospace; font-size:10px; }");
    layout->addWidget(m_logView);

    DebugLog::instance().setPanel(m_logView);

    connect(clearBtn, &QPushButton::clicked, m_logView, &QTextEdit::clear);
    connect(openBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(DebugLog::instance().logFilePath()));
    });

    AKIS_LOG(UI, QStringLiteral("Debug log panel ready"));
}
