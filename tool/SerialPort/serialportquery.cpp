#include "serialportquery.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <algorithm>
#include <string>

#ifdef Q_OS_WIN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <initguid.h>
#include <setupapi.h>
#include <devguid.h>
#include <devpkey.h>
#include <cfgmgr32.h>

static QString queryRegString(HDEVINFO devInfoSet, SP_DEVINFO_DATA &devInfoData, const wchar_t *valueName)
{
    QString result;
    HKEY hKey = SetupDiOpenDevRegKey(devInfoSet, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

    if (hKey == INVALID_HANDLE_VALUE)
        return result;

    DWORD type = 0;
    DWORD size = 0;
    LONG ret = RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &size);

    if (ret == ERROR_SUCCESS && type == REG_SZ && size > 0) {
        QByteArray buffer;
        buffer.resize(static_cast<int>(size));
        ret = RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &size);

        if (ret == ERROR_SUCCESS)
            result = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
    }

    RegCloseKey(hKey);
    return result;
}

static QString getDeviceRegistryPropertyString(HDEVINFO devInfoSet, SP_DEVINFO_DATA &devInfoData, DWORD property)
{
    DWORD type = 0;
    DWORD requiredSize = 0;

    SetupDiGetDeviceRegistryPropertyW(devInfoSet, &devInfoData, property, &type, nullptr, 0, &requiredSize);

    if (requiredSize == 0)
        return QString();

    QByteArray buffer;
    buffer.resize(static_cast<int>(requiredSize));

    BOOL ok = SetupDiGetDeviceRegistryPropertyW(
        devInfoSet,
        &devInfoData,
        property,
        &type,
        reinterpret_cast<PBYTE>(buffer.data()),
        requiredSize,
        nullptr
    );

    if (!ok)
        return QString();

    if (type == REG_SZ)
        return QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));

    if (type == REG_MULTI_SZ) {
        const wchar_t *p = reinterpret_cast<const wchar_t *>(buffer.constData());
        QStringList list;

        while (*p) {
            QString item = QString::fromWCharArray(p);
            list << item;
            p += item.length() + 1;
        }

        return list.join(" | ");
    }

    return QString();
}

static QStringList getDevicePropertyStringList(
    HDEVINFO devInfoSet,
    SP_DEVINFO_DATA &devInfoData,
    const DEVPROPKEY &propertyKey
)
{
    DEVPROPTYPE propertyType = 0;
    DWORD requiredSize = 0;

    SetupDiGetDevicePropertyW(devInfoSet, &devInfoData, &propertyKey, &propertyType, nullptr, 0, &requiredSize, 0);

    if (requiredSize == 0)
        return {};

    QByteArray buffer;
    buffer.resize(static_cast<int>(requiredSize));

    BOOL ok = SetupDiGetDevicePropertyW(
        devInfoSet,
        &devInfoData,
        &propertyKey,
        &propertyType,
        reinterpret_cast<PBYTE>(buffer.data()),
        requiredSize,
        nullptr,
        0
    );

    if (!ok)
        return {};

    QStringList result;

    if (propertyType == DEVPROP_TYPE_STRING) {
        result << QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
    } else if (propertyType == DEVPROP_TYPE_STRING_LIST) {
        const wchar_t *p = reinterpret_cast<const wchar_t *>(buffer.constData());

        while (*p) {
            QString item = QString::fromWCharArray(p);
            result << item;
            p += item.length() + 1;
        }
    }

    return result;
}

static QStringList getDevNodePropertyStringList(DEVINST devInst, const DEVPROPKEY &propertyKey)
{
    DEVPROPTYPE propertyType = 0;
    ULONG requiredSize = 0;
    CONFIGRET ret = CM_Get_DevNode_PropertyW(devInst, &propertyKey, &propertyType, nullptr, &requiredSize, 0);

    if (ret != CR_BUFFER_SMALL || requiredSize == 0)
        return {};

    QByteArray buffer;
    buffer.resize(static_cast<int>(requiredSize));
    ret = CM_Get_DevNode_PropertyW(
        devInst,
        &propertyKey,
        &propertyType,
        reinterpret_cast<PBYTE>(buffer.data()),
        &requiredSize,
        0
    );

    if (ret != CR_SUCCESS)
        return {};

    QStringList result;

    if (propertyType == DEVPROP_TYPE_STRING) {
        result << QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
    } else if (propertyType == DEVPROP_TYPE_STRING_LIST) {
        const wchar_t *p = reinterpret_cast<const wchar_t *>(buffer.constData());

        while (*p) {
            QString item = QString::fromWCharArray(p);
            result << item;
            p += item.length() + 1;
        }
    }

    return result;
}

static QString getDeviceInstanceId(HDEVINFO devInfoSet, SP_DEVINFO_DATA &devInfoData)
{
    DWORD requiredSize = 0;
    SetupDiGetDeviceInstanceIdW(devInfoSet, &devInfoData, nullptr, 0, &requiredSize);

    if (requiredSize == 0)
        return QString();

    std::wstring buffer;
    buffer.resize(requiredSize);

    BOOL ok = SetupDiGetDeviceInstanceIdW(devInfoSet, &devInfoData, buffer.data(), requiredSize, nullptr);

    if (!ok)
        return QString();

    return QString::fromWCharArray(buffer.c_str());
}

static QString getParentInstanceId(SP_DEVINFO_DATA &devInfoData, DEVINST *parentDevInstOut = nullptr)
{
    DEVINST parentDevInst = 0;
    CONFIGRET ret = CM_Get_Parent(&parentDevInst, devInfoData.DevInst, 0);

    if (ret != CR_SUCCESS)
        return QString();

    if (parentDevInstOut)
        *parentDevInstOut = parentDevInst;

    wchar_t parentId[MAX_DEVICE_ID_LEN] = {0};
    ret = CM_Get_Device_IDW(parentDevInst, parentId, MAX_DEVICE_ID_LEN, 0);

    if (ret != CR_SUCCESS)
        return QString();

    return QString::fromWCharArray(parentId);
}

static QString parseMiNumber(const QString &text)
{
    QRegularExpression re("(MI_[0-9A-Fa-f]{2})");
    QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch())
        return match.captured(1).toUpper();

    return QString();
}

static QString parseUsbId(const QString &text, const QString &prefix)
{
    QRegularExpression re(QString("%1_([0-9A-Fa-f]{4})").arg(prefix));
    QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch())
        return match.captured(1).toUpper();

    return QString();
}

static QString normalizePhysicalPortPath(const QString &path)
{
    QString normalized = path;
    normalized.remove(QRegularExpression("#USBMI\\([0-9]+\\)$"));
    return normalized;
}

static QString makePhysicalPortKey(const QStringList &locationPaths, const QStringList &parentLocationPaths, const QString &fallback)
{
    QString bestKey;
    QStringList candidates;
    candidates << locationPaths;
    candidates << parentLocationPaths;

    for (const QString &candidate : candidates) {
        QString normalized = normalizePhysicalPortPath(candidate);

        if (normalized.length() > bestKey.length())
            bestKey = normalized;
    }

    if (!bestKey.isEmpty())
        return bestKey;

    return fallback;
}

#endif

QList<SerialPort> SerialPortQuery::availablePorts()
{
    QList<SerialPort> result;

#ifdef Q_OS_WIN
    HDEVINFO devInfoSet = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);

    if (devInfoSet == INVALID_HANDLE_VALUE)
        return result;

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA devInfoData;
        ZeroMemory(&devInfoData, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        BOOL ok = SetupDiEnumDeviceInfo(devInfoSet, index, &devInfoData);

        if (!ok) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;

            continue;
        }

        SerialPort::Info info;
        info.portName = queryRegString(devInfoSet, devInfoData, L"PortName");

        if (!info.portName.startsWith("COM", Qt::CaseInsensitive))
            continue;

        info.friendlyName = getDeviceRegistryPropertyString(devInfoSet, devInfoData, SPDRP_FRIENDLYNAME);
        info.hardwareId = getDeviceRegistryPropertyString(devInfoSet, devInfoData, SPDRP_HARDWAREID);
        info.locationInfo = getDeviceRegistryPropertyString(devInfoSet, devInfoData, SPDRP_LOCATION_INFORMATION);
        info.instanceId = getDeviceInstanceId(devInfoSet, devInfoData);
        info.vid = parseUsbId(info.instanceId, "VID");
        info.pid = parseUsbId(info.instanceId, "PID");

        if (info.vid.isEmpty())
            info.vid = parseUsbId(info.hardwareId, "VID");

        if (info.pid.isEmpty())
            info.pid = parseUsbId(info.hardwareId, "PID");

        info.locationPaths = getDevicePropertyStringList(devInfoSet, devInfoData, DEVPKEY_Device_LocationPaths);

        DEVINST parentDevInst = 0;
        info.parentInstanceId = getParentInstanceId(devInfoData, &parentDevInst);

        if (parentDevInst != 0)
            info.parentLocationPaths = getDevNodePropertyStringList(parentDevInst, DEVPKEY_Device_LocationPaths);

        info.interfaceNumber = parseMiNumber(info.instanceId);

        if (info.interfaceNumber.isEmpty())
            info.interfaceNumber = parseMiNumber(info.hardwareId);

        info.physicalPortKey = makePhysicalPortKey(info.locationPaths, info.parentLocationPaths, info.locationInfo);
        result << SerialPort(info);
    }

    SetupDiDestroyDeviceInfoList(devInfoSet);
#endif

    return result;
}

bool SerialPortQuery::findByPortName(const QString &portName, SerialPort *outPort)
{
    for (const SerialPort &port : availablePorts()) {
        if (port.portName().compare(portName, Qt::CaseInsensitive) == 0) {
            if (outPort)
                *outPort = port;

            return true;
        }
    }

    return false;
}

bool SerialPortQuery::samePhysicalPort(const SerialPort &left, const SerialPort &right)
{
    return !left.physicalPortKey().isEmpty()
        && left.physicalPortKey().compare(right.physicalPortKey(), Qt::CaseInsensitive) == 0;
}

QString SerialPortQuery::snapshot(const QList<SerialPort> &ports)
{
    QStringList lines;

    for (const SerialPort &port : ports) {
        const SerialPort::Info &info = port.info();
        lines << QString("%1|%2|%3|%4|%5|%6")
                     .arg(info.portName)
                     .arg(info.vid)
                     .arg(info.pid)
                     .arg(info.instanceId)
                     .arg(info.hardwareId)
                     .arg(info.physicalPortKey);
    }

    std::sort(lines.begin(), lines.end());
    return lines.join("\n");
}

QString SerialPortQuery::toDebugText(const QList<SerialPort> &ports)
{
    QStringList output;
    output << QString("\n");
    output << QString("================ Serial ports count: %1 ================\n").arg(ports.size());

    for (const SerialPort &port : ports) {
        const SerialPort::Info &info = port.info();
        output << QString("--------------------------------------------------\n");
        output << QString("PortName            : %1\n").arg(info.portName);
        output << QString("FriendlyName        : %1\n").arg(info.friendlyName);
        output << QString("VID/PID             : %1 / %2\n").arg(info.vid, info.pid);
        output << QString("InstanceId          : %1\n").arg(info.instanceId);
        output << QString("HardwareId          : %1\n").arg(info.hardwareId);
        output << QString("LocationInfo        : %1\n").arg(info.locationInfo);
        output << QString("PhysicalPortKey     : %1\n").arg(info.physicalPortKey);
        output << QString("InterfaceNumber     : %1\n").arg(info.interfaceNumber);

        output << QString("LocationPaths       :\n");
        if (info.locationPaths.isEmpty()) {
            output << QString("    <empty>\n");
        } else {
            for (const QString &path : info.locationPaths)
                output << QString("    %1\n").arg(path);
        }

        output << QString("ParentInstanceId    : %1\n").arg(info.parentInstanceId);
        output << QString("ParentLocationPaths :\n");
        if (info.parentLocationPaths.isEmpty()) {
            output << QString("    <empty>\n");
        } else {
            for (const QString &path : info.parentLocationPaths)
                output << QString("    %1\n").arg(path);
        }
    }

    output << QString("==================================================\n\n");
    return output.join(QString());
}

SerialPort::SerialPort()
    : m_port(new QSerialPort)
{
}

SerialPort::SerialPort(const Info &info)
    : m_port(new QSerialPort)
    , m_info(info)
{
}

const SerialPort::Info& SerialPort::info() const
{
    return m_info;
}

void SerialPort::setInfo(const Info &info)
{
    m_info = info;
}

QString SerialPort::portName() const
{
    return m_info.portName;
}

QString SerialPort::physicalPortKey() const
{
    return m_info.physicalPortKey;
}

bool SerialPort::open(const Config &config)
{
    close();
    m_lastError.clear();
    m_port->setPortName(config.portName);
    m_port->setBaudRate(config.baudRate);
    m_port->setDataBits(config.dataBits);
    m_port->setParity(config.parity);
    m_port->setStopBits(config.stopBits);
    m_port->setFlowControl(config.flowControl);

    if (!m_port->open(config.openMode)) {
        m_lastError = m_port->errorString();
        return false;
    }

    m_info.portName = config.portName;
    return true;
}

bool SerialPort::open(qint32 baudRate)
{
    return open(m_info.portName, baudRate);
}

bool SerialPort::open(const QString &portName, qint32 baudRate)
{
    Config config;
    config.portName = portName;
    config.baudRate = baudRate;
    return open(config);
}

void SerialPort::close()
{
    if (m_port->isOpen())
        m_port->close();
}

bool SerialPort::isOpen() const
{
    return m_port->isOpen();
}

QString SerialPort::errorString() const
{
    if (!m_lastError.isEmpty())
        return m_lastError;

    return m_port->errorString();
}

QSerialPort::SerialPortError SerialPort::error() const
{
    return m_port->error();
}

qint64 SerialPort::bytesAvailable() const
{
    return m_port->bytesAvailable();
}

void SerialPort::clear(QSerialPort::Directions directions)
{
    m_port->clear(directions);
}

bool SerialPort::waitForReadyRead(int timeoutMs)
{
    bool ok = m_port->waitForReadyRead(timeoutMs);

    if (!ok)
        m_lastError = m_port->errorString();

    return ok;
}

bool SerialPort::waitForBytesWritten(int timeoutMs)
{
    bool ok = m_port->waitForBytesWritten(timeoutMs);

    if (!ok)
        m_lastError = m_port->errorString();

    return ok;
}

qint64 SerialPort::write(const QByteArray &data, int timeoutMs)
{
    if (!m_port->isOpen()) {
        m_lastError = QString("Serial port is not open");
        return -1;
    }

    qint64 written = m_port->write(data);

    if (written < 0) {
        m_lastError = m_port->errorString();
        return -1;
    }

    if (timeoutMs >= 0 && !m_port->waitForBytesWritten(timeoutMs)) {
        m_lastError = m_port->errorString();
        return -1;
    }

    return written;
}

bool SerialPort::writeAll(const QByteArray &data, int timeoutMs)
{
    if (!m_port->isOpen()) {
        m_lastError = QString("Serial port is not open");
        return false;
    }

    QElapsedTimer timer;
    timer.start();
    qint64 offset = 0;

    while (offset < data.size()) {
        qint64 written = m_port->write(data.constData() + offset, data.size() - offset);

        if (written < 0) {
            m_lastError = m_port->errorString();
            return false;
        }

        offset += written;

        int remainMs = timeoutMs;
        if (timeoutMs >= 0) {
            remainMs = timeoutMs - static_cast<int>(timer.elapsed());
            if (remainMs <= 0) {
                m_lastError = QString("Serial port write timeout");
                return false;
            }
        }

        if (!m_port->waitForBytesWritten(remainMs)) {
            m_lastError = m_port->errorString();
            return false;
        }
    }

    return true;
}

QByteArray SerialPort::readAllAvailable()
{
    return m_port->readAll();
}

QByteArray SerialPort::read(int maxSize, int timeoutMs, bool *ok)
{
    if (ok)
        *ok = false;

    if (!m_port->isOpen()) {
        m_lastError = QString("Serial port is not open");
        return {};
    }

    if (m_port->bytesAvailable() <= 0 && !m_port->waitForReadyRead(timeoutMs)) {
        m_lastError = m_port->errorString();
        return {};
    }

    if (ok)
        *ok = true;

    return m_port->read(maxSize);
}

QByteArray SerialPort::readBytes(int expectedSize, int timeoutMs, bool *ok)
{
    if (ok)
        *ok = false;

    if (!m_port->isOpen()) {
        m_lastError = QString("Serial port is not open");
        return {};
    }

    QByteArray data;
    QElapsedTimer timer;
    timer.start();

    while (data.size() < expectedSize) {
        if (m_port->bytesAvailable() > 0)
            data += m_port->read(expectedSize - data.size());

        if (data.size() >= expectedSize)
            break;

        int remainMs = timeoutMs;
        if (timeoutMs >= 0) {
            remainMs = timeoutMs - static_cast<int>(timer.elapsed());
            if (remainMs <= 0) {
                m_lastError = QString("Serial port read timeout");
                return data;
            }
        }

        if (!m_port->waitForReadyRead(remainMs)) {
            m_lastError = m_port->errorString();
            return data;
        }
    }

    if (ok)
        *ok = true;

    return data;
}
