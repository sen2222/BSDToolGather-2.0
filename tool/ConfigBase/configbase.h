#ifndef CONFIGBASE_H
#define CONFIGBASE_H



#include <QString>
#include <QSettings>
#include <QMutex>
#include <QFile>
#include <QDir>




class ConfigBase
{

public:
    static ConfigBase* getInstance()
    {
        static ConfigBase instance;
        return &instance;
    }
    ConfigBase(const ConfigBase&) = delete;
    ConfigBase& operator=(const ConfigBase&) = delete;

public:
    int ConfigReadInt(QString section, QString key, int defaultValue);
    int ConfigWriteInt(QString section, QString key, int value);
    QString ConfigReadString(QString section, QString key, QString defaultValue);
    int ConfigWriteString(QString section, QString key, QString &value);

private:
    ConfigBase();
    ~ConfigBase();
    QSettings *m_basicSettings;
    QMutex     m_basicMutex;
};






#endif // CONFIGBASE_H
