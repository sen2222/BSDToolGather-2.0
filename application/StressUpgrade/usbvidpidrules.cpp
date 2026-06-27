#include "usbvidpidrules.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool UsbVidPidRules::load(const QString &filePath, QString *errorMessage)
{
    m_rules.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法打开 VID/PID 规则文件：%1").arg(filePath);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || (!doc.isArray() && !doc.isObject())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("VID/PID 规则文件格式错误：%1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonArray devices = doc.isArray()
        ? doc.array()
        : doc.object().value(QStringLiteral("devices")).toArray();
    for (const QJsonValue &value : devices) {
        const QJsonObject object = value.toObject();
        Rule rule;
        rule.vid = normalizedUsbId(object.value(QStringLiteral("VID")).toString(object.value(QStringLiteral("vid")).toString()));
        rule.pid = normalizedUsbId(object.value(QStringLiteral("PID")).toString(object.value(QStringLiteral("pid")).toString()));
        rule.typeString = object.value(QStringLiteral("TypeString")).toString(
            object.value(QStringLiteral("typeString")).toString());
        if (!rule.vid.isEmpty() && !rule.pid.isEmpty()) {
            m_rules.append(rule);
        }
    }

    if (m_rules.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("VID/PID 规则文件没有有效设备规则");
        }
        return false;
    }

    return true;
}

bool UsbVidPidRules::isAllowed(const SerialPort::Info &info) const
{
    const QString vid = normalizedUsbId(info.vid);
    const QString pid = normalizedUsbId(info.pid);
    for (const Rule &rule : m_rules) {
        if (rule.vid == vid && rule.pid == pid) {
            return true;
        }
    }
    return false;
}

QString UsbVidPidRules::describe(const SerialPort::Info &info) const
{
    const QString vid = normalizedUsbId(info.vid);
    const QString pid = normalizedUsbId(info.pid);
    for (const Rule &rule : m_rules) {
        if (rule.vid == vid && rule.pid == pid) {
            return rule.typeString.isEmpty() ? QStringLiteral("%1:%2").arg(rule.vid, rule.pid) : rule.typeString;
        }
    }
    return QStringLiteral("未支持设备 %1:%2").arg(vid, pid);
}

const QList<UsbVidPidRules::Rule>& UsbVidPidRules::rules() const
{
    return m_rules;
}

QString UsbVidPidRules::normalizedUsbId(QString value)
{
    value = value.trimmed().toUpper();
    value.remove(QStringLiteral("0X"));
    while (value.length() < 4) {
        value.prepend(QChar('0'));
    }
    return value;
}
