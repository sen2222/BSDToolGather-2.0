#include "logoutput.h"
#include "alltoolfun.h"




LogOutput ::LogOutput()
{
    QString logDirTmp = CONFIG_READ_STRING(CONFIG_SECTION_LOG_OUTPUT, CONFIG_KEY_LOG_FILE_PATH, "/log");
    logLevel = CONFIG_READ_INT(CONFIG_SECTION_LOG_OUTPUT, CONFIG_KEY_LOG_LEVEL, 0);
    logDir = QDir::currentPath() + "/" + logDirTmp;
    
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDateTime currentDateTime = QDateTime::currentDateTime();
    currentLogDate = currentDateTime.date();
    QString logFileName = QString("LOG-%1.log").arg(currentLogDate.toString("yyyy-MMdd"));
    logFilePath = dir.filePath(logFileName);

    logFile = new QFile(logFilePath);
    logFile->open(QIODevice::Append | QIODevice::Text);
    logBuff = new QtSafeQueue<LOG_ENTRY_S>(50);
}
LogOutput ::~LogOutput()
{
    if (logBuff) {
        delete logBuff;
    }
    if (logFile) {
        logFile->close();
        delete logFile;    
    }    
}


void LogOutput::logOutToFile(LOG_LEVEL_E level, const QString& message)
{
    QMutexLocker locker(&log_mutex);
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDate nowDate = currentDateTime.date();
    if (nowDate != currentLogDate) {
        if (logFile && logFile->isOpen()) {
            logFile->close();
        }
        currentLogDate = nowDate;
        QString logFileName = QString("LOG-%1.log").arg(currentLogDate.toString("yyyy-MMdd"));
        logFilePath = logDir + "/" + logFileName;
        
        if (!logFile) {
            logFile = new QFile(logFilePath);
        } else {
            logFile->setFileName(logFilePath);
        }
        logFile->open(QIODevice::Append | QIODevice::Text);
    }
    if (!logFile || !logFile->isOpen()) {
        if (!logFile) {
            logFile = new QFile(logFilePath);
        }
        logFile->open(QIODevice::Append | QIODevice::Text);
    }
    QString logEntry = QString("(%1)%2 %3")
        .arg(currentDateTime.toString("hh:mm:ss"))
        .arg((level == LOG_ERRO) ? "[ERRO]" :
             (level == LOG_WARN) ? "[WARN]" :
             (level == LOG_DBUG) ? "[DBUG]" : "[INFO]")
        .arg(message);
    
    QTextStream out(logFile);
    out << logEntry;
    out.flush();
}
void LogOutput::logOutToView(LOG_LEVEL_E level, const QString &message)
{
    LOG_ENTRY_S logEntry;
    logEntry.level = level;
    logEntry.message = QString("(%1)%2 %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg((level == LOG_ERRO) ? "[ERRO]" :
             (level == LOG_WARN) ? "[WARN]" :
             (level == LOG_DBUG) ? "[DBUG]" : "[INFO]")
        .arg(message);
    switch (level) {
        case LOG_ERRO:
            logEntry.color = QColor(Qt::red);
            break;
        case LOG_WARN:
            logEntry.color = QColor(255, 165, 0); // 橙色
            break;
        case LOG_DBUG:
            logEntry.color = QColor(Qt::blue);
            break;
        case LOG_INFO:
        default:
            logEntry.color = QColor(Qt::black);
            break;
    }
    logBuff->enqueue(logEntry);
}

void LogOutput::logOut(LOG_LEVEL_E level, const QString &message)
{
    if (level > logLevel) {
        return;
    }
    logOutToView(level, message);
    logOutToFile(level, message);
}


/** @brief 从队列中取出日志条目 */
int LogOutput::getAllLogEntries(QList<LOG_ENTRY_S>& outList)
{
    outList.clear();
    return logBuff->takeAll(outList);
}
