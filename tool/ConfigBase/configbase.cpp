#include "configbase.h"


#define BASIC_CONFIG_FILE_PATH "/config/BasicConfig.ini"

ConfigBase::ConfigBase()
{
    QString appDir = QDir::currentPath();
    QString configPath = appDir + BASIC_CONFIG_FILE_PATH;
    m_basicSettings = new QSettings(configPath, QSettings::IniFormat);
}

ConfigBase::~ConfigBase()
{
    delete m_basicSettings;
}

int ConfigBase::ConfigReadInt(QString section, QString key, int defaultValue)
{
    if (section.isEmpty()) {
        return defaultValue;
    }

    QString fullKey = QString("%1/%2").arg(section).arg(key);
    QMutexLocker locker(&m_basicMutex);
    return m_basicSettings->value(fullKey, defaultValue).toInt();
}
int ConfigBase::ConfigWriteInt(QString section, QString key, int value)
{
    if (section.isEmpty()) {
        return -1;
    }

    QString fullKey = QString("%1/%2").arg(section).arg(key);
    QMutexLocker locker(&m_basicMutex);
    m_basicSettings->setValue(fullKey, value);
    return 0;
}

QString ConfigBase::ConfigReadString(QString section, QString key, QString defaultValue)
{
    if (section.isEmpty()) {
        return defaultValue;
    }

    QString fullKey = QString("%1/%2").arg(section).arg(key);
    QMutexLocker locker(&m_basicMutex);
    return m_basicSettings->value(fullKey, defaultValue).toString();
}
int ConfigBase::ConfigWriteString(QString section, QString key, QString &value)
{
    if (section.isEmpty()) {
        return -1;
    }

    QString fullKey = QString("%1/%2").arg(section).arg(key);
    QMutexLocker locker(&m_basicMutex);
    m_basicSettings->setValue(fullKey, value);
    return 0;
}
