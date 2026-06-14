#ifndef __LOGOUTPUT_H__
#define __LOGOUTPUT_H__



#include <QFile>
#include <QDate>
#include <QObject>
#include <QMutex>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QColor>
#include "SafeQueue/SafeQueue.h"



#define CONFIG_SECTION_LOG_OUTPUT   "LogOutput"
#define CONFIG_KEY_LOG_LEVEL        "logLevel"
#define CONFIG_KEY_LOG_FILE_PATH    "logFileDir"

#define LOG_OUTPUT_BUFF_SIZE        1024

typedef enum
{
    LOG_ERRO = 0,
    LOG_WARN,
    LOG_INFO,
    LOG_DBUG,
} LOG_LEVEL_E;

typedef struct
{
    LOG_LEVEL_E level;
    QColor      color;
    QString     message;
} LOG_ENTRY_S;

class LogOutput
{
public:
    static LogOutput *getInstance()
    {
        static LogOutput instance;
        return &instance;
    }
    void logOut(LOG_LEVEL_E level, const QString &message);
    int getAllLogEntries(QList<LOG_ENTRY_S>& outList);
    
private:
    explicit LogOutput();
    ~LogOutput();
    void logOutToFile(LOG_LEVEL_E level, const QString& message);
    void logOutToView(LOG_LEVEL_E level, const QString& message);

    int logLevel;
    QString logFilePath;
    QString logDir;
    QFile *logFile;
    QDate currentLogDate;
    QMutex log_mutex;

    QtSafeQueue<LOG_ENTRY_S> *logBuff;
};











#endif
