#ifndef USBVIDPIDRULES_H
#define USBVIDPIDRULES_H

#include <QList>
#include <QString>

#include "SerialPort/serialportquery.h"

class UsbVidPidRules
{
public:
    struct Rule
    {
        QString vid;
        QString pid;
        QString typeString;
    };

    bool load(const QString &filePath, QString *errorMessage = nullptr);

    bool isAllowed(const SerialPort::Info &info) const;
    QString describe(const SerialPort::Info &info) const;
    const QList<Rule>& rules() const;

private:
    static QString normalizedUsbId(QString value);

    QList<Rule> m_rules;
};

#endif // USBVIDPIDRULES_H
