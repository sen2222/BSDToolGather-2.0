#include "mainlogwidget.h"
#include "ui_mainlogwidget.h"

MainLogWidget::MainLogWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainLogWidget)
{
    ui->setupUi(this);
    ui->logTextEdit->setReadOnly(true);
    ui->logTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_logTimer = new QTimer(this);
    m_logTimer->setInterval(50);  // 50ms 刷新一次
    connect(m_logTimer, &QTimer::timeout, this, &MainLogWidget::appendLogEntry);
    m_logTimer->start();

    
}

MainLogWidget::~MainLogWidget()
{
    delete ui;
}

void MainLogWidget::appendLogEntry(void)
{
    QList<LOG_ENTRY_S> logList;
    int count = LogOutput::getInstance()->getAllLogEntries(logList);
    if (count <= 0)
    {
        return;
    }

    QPlainTextEdit* logEdit = ui->logTextEdit; 
    QTextDocument* doc = logEdit->document();
    int curLines = doc->lineCount();
    if (curLines > MAX_LOG_LINE_COUNT)
    {
        int delLines = curLines - MAX_LOG_LINE_COUNT;
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, delLines);
        cursor.removeSelectedText();
    }

    // logEdit->setUpdatesEnabled(false); // 暂停界面刷新，防闪烁

    QTextCursor cursor(logEdit->document());
    cursor.movePosition(QTextCursor::End);

    for (const auto& entry : logList)
    {
        QTextCharFormat fmt;
        fmt.setForeground(entry.color);
        cursor.setCharFormat(fmt);
        cursor.insertText(entry.message);
    }

    // logEdit->setUpdatesEnabled(true); // 恢复刷新
    logEdit->moveCursor(QTextCursor::End);
}
