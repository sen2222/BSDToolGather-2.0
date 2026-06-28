#include "stressupgradeworker.h"

#include "Crc/crc8.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

StressUpgradeWorker::StressUpgradeWorker(const Options &options, QObject *parent)
    : QThread(parent)
    , m_options(options)
    , m_stopRequested(false)
{
    m_statistics.total = qMax(0, options.upgradeCount);
}

StressUpgradeWorker::~StressUpgradeWorker()
{
    requestStop();
    wait(3000);
}

void StressUpgradeWorker::requestStop()
{
    m_stopRequested.store(true);
}

StressUpgradeWorker::Statistics StressUpgradeWorker::statistics() const
{
    return m_statistics;
}

void StressUpgradeWorker::run()
{
    QString errorMessage;
    Firmware firmwareA;
    Firmware firmwareB;

    setState(State::ValidatePort, QStringLiteral("校验串口 VID/PID"));
    if (!m_rules.load(m_options.ruleFilePath, &errorMessage)) {
        addFailure(errorMessage);
        return;
    }

    const SerialPort::Info selectedInfo = m_options.selectedPortInfo;
    m_physicalPortKey = selectedInfo.physicalPortKey;
    if (m_physicalPortKey.isEmpty()) {
        addFailure(QStringLiteral("当前串口缺少物理端口信息，无法在 VID/PID 切换后跟踪同一个 USB 口"));
        return;
    }
    if (!m_rules.isAllowed(selectedInfo)) {
        addFailure(QStringLiteral("当前串口不在支持规则内：%1").arg(m_rules.describe(selectedInfo)));
        return;
    }

    setState(State::LoadFirmware, QStringLiteral("读取固件文件"));
    if (!loadFirmware(m_options.firmwareAPath, &firmwareA, &errorMessage)
        || !loadFirmware(m_options.firmwareBPath, &firmwareB, &errorMessage)) {
        addFailure(errorMessage);
        return;
    }

    bool abortedByEnumerateTimeout = false;
    for (int index = 0; index < m_options.upgradeCount && !shouldStop(); ++index) {
        const Firmware &firmware = (index % 2 == 0) ? firmwareA : firmwareB;
        const Firmware &alternateFirmware = (index % 2 == 0) ? firmwareB : firmwareA;
        QElapsedTimer timer;
        timer.start();

        emit logMessage(QStringLiteral("------------------------------------------------------------"));
        emit logMessage(QStringLiteral("第 %1/%2 次升级，目标版本：%3，文件：%4")
            .arg(index + 1)
            .arg(m_options.upgradeCount)
            .arg(firmware.versionString)
            .arg(QFileInfo(firmware.path).fileName()));

        if (performOneUpgrade(firmware, alternateFirmware, index + 1, &errorMessage)) {
            addSuccess(timer.elapsed());
        } else if (!shouldStop()) {
            if (isStartEnumerateAbort(errorMessage)) {
                abortedByEnumerateTimeout = true;
                emit logMessage(errorMessage);
                break;
            }
            m_statistics.finished++;
            m_statistics.failed++;
            m_statistics.lastDurationMs = timer.elapsed();
            m_statistics.totalDurationMs += m_statistics.lastDurationMs;
            emit logMessage(QStringLiteral("升级失败：%1").arg(errorMessage));
            emit statisticsChanged(m_statistics);
            if (isDeviceLossFailure(errorMessage) && index + 1 >= m_options.upgradeCount) {
                SerialPort recoveredPort;
                QString recoverError;
                emit logMessage(QStringLiteral("最后一轮疑似设备断开，等待设备重新枚举，最长 3 分钟"));
                setState(State::FindPort, QStringLiteral("等待设备重新枚举"));
                if (!waitOpenablePhysicalPort(&recoveredPort, &recoverError, 180000, PortSystem::Any)) {
                    abortedByEnumerateTimeout = true;
                    emit logMessage(QStringLiteral("最后一轮失败后等待设备枚举超过 3 分钟，压测异常终止"));
                    break;
                }
                emit logMessage(QStringLiteral("设备已重新枚举：%1，%2")
                    .arg(recoveredPort.portName(), m_rules.describe(recoveredPort.info())));
            }
        }
    }

    closePort();
    const QString finishText = shouldStop()
        ? QStringLiteral("已停止升级")
        : abortedByEnumerateTimeout ? QStringLiteral("压测异常停止") : QStringLiteral("压测升级完成");
    emitFinalSummary(shouldStop()
        ? QStringLiteral("压测已停止")
        : abortedByEnumerateTimeout ? QStringLiteral("压测异常停止") : QStringLiteral("压测完成"));
    setState(shouldStop() ? State::Stopping : abortedByEnumerateTimeout ? State::Failed : State::Finished, finishText);
}

bool StressUpgradeWorker::shouldStop() const
{
    return m_stopRequested.load();
}

void StressUpgradeWorker::setState(State state, const QString &text)
{
    emit stateChanged(state, text);
}

void StressUpgradeWorker::addFailure(const QString &message)
{
    closePort();
    m_statistics.failed++;
    emit logMessage(message);
    emit statisticsChanged(m_statistics);
    setState(State::Failed, message);
}

void StressUpgradeWorker::addSuccess(qint64 durationMs)
{
    m_statistics.finished++;
    m_statistics.success++;
    m_statistics.lastDurationMs = durationMs;
    m_statistics.totalDurationMs += durationMs;
    emit statisticsChanged(m_statistics);
    emit logMessage(QStringLiteral("升级成功，用时 %1 分钟").arg(formatDuration(durationMs)));
}

void StressUpgradeWorker::emitFinalSummary(const QString &title)
{
    const qint64 average = m_statistics.finished > 0
        ? m_statistics.totalDurationMs / m_statistics.finished
        : 0;
    emit logMessage(QStringLiteral("============================================================"));
    emit logMessage(QStringLiteral("%1：总数:%2 完成:%3 成功:%4 失败:%5 总耗时:%6 分钟 平均耗时:%7 分钟 最近耗时:%8 分钟")
        .arg(title)
        .arg(m_statistics.total)
        .arg(m_statistics.finished)
        .arg(m_statistics.success)
        .arg(m_statistics.failed)
        .arg(formatDuration(m_statistics.totalDurationMs))
        .arg(formatDuration(average))
        .arg(formatDuration(m_statistics.lastDurationMs)));
    emit logMessage(QStringLiteral("============================================================"));
}

QString StressUpgradeWorker::formatDuration(qint64 durationMs) const
{
    qint64 totalSeconds = (durationMs + 500) / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1.%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

bool StressUpgradeWorker::isStartEnumerateAbort(const QString &message) const
{
    return message.contains(QStringLiteral("进入新一轮升级时等待设备枚举超过 3 分钟"));
}

bool StressUpgradeWorker::isDeviceLossFailure(const QString &message) const
{
    return message.contains(QStringLiteral("串口断开"))
        || message.contains(QStringLiteral("设备断开"))
        || message.contains(QStringLiteral("串口失效"))
        || message.contains(QStringLiteral("打开串口失败"))
        || message.contains(QStringLiteral("串口写入失败"))
        || message.contains(QStringLiteral("串口未打开"))
        || message.contains(QStringLiteral("系统找不到指定的文件"))
        || message.contains(QStringLiteral("指定不存在的设备"))
        || message.contains(QStringLiteral("设备不识别此命令"));
}

bool StressUpgradeWorker::loadFirmware(const QString &path, Firmware *firmware, QString *errorMessage) const
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("固件文件不存在：%1").arg(path);
        }
        return false;
    }

    QRegularExpression regex(QStringLiteral(R"(^([^-]+)-([^-]+)-(\d+)\.(\d+)\.(\d+)-OTA\.bin$)"),
                             QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = regex.match(fileInfo.fileName());
    if (!match.hasMatch()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("固件文件名格式不正确，应为：项目-客户-x.x.x-OTA.bin，当前：%1")
                .arg(fileInfo.fileName());
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法打开固件文件：%1").arg(path);
        }
        return false;
    }

    firmware->path = path;
    firmware->data = file.readAll();
    if (firmware->data.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("固件文件为空：%1").arg(path);
        }
        return false;
    }

    const int major = match.captured(3).toInt();
    const int minor = match.captured(4).toInt();
    const int patch = match.captured(5).toInt();
    firmware->versionString = QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
    firmware->versionValue = major * 100 + minor * 10 + patch;
    firmware->md5 = QCryptographicHash::hash(firmware->data, QCryptographicHash::Md5);
    return true;
}

bool StressUpgradeWorker::performOneUpgrade(const Firmware &firmware,
                                            const Firmware &alternateFirmware,
                                            int cycleIndex,
                                            QString *errorMessage)
{
    SerialPort foundPort;
    bool inRecovery = false;
    QString currentVersion;
    int currentVersionValue = 0;
    const Firmware *targetFirmware = &firmware;

    setState(State::FindPort, QStringLiteral("查找当前物理端口"));
    if (!waitOpenablePhysicalPort(&foundPort, errorMessage, 180000, PortSystem::Any)) {
        if (errorMessage && !shouldStop()) {
            *errorMessage = QStringLiteral("进入新一轮升级时等待设备枚举超过 3 分钟，压测异常终止");
        }
        return false;
    }

    setState(State::OpenPort, QStringLiteral("打开串口"));
    msleep(500);
    if (portMatchesSystem(foundPort.info(), PortSystem::Recovery)) {
        msleep(500);
    }

    setState(State::CheckRecovery, QStringLiteral("检查 recovery 状态"));
    if (!checkRecovery(&inRecovery, errorMessage)) {
        return false;
    }

    if (!inRecovery) {
        setState(State::CheckVersion, QStringLiteral("检查当前版本"));
        if (!checkVersion(&currentVersion, &currentVersionValue, errorMessage)) {
            return false;
        }

        if (currentVersionValue == firmware.versionValue) {
            targetFirmware = &alternateFirmware;
            emit logMessage(QStringLiteral("当前版本已是默认目标版本 %1，本轮切换为目标版本 %2")
                .arg(currentVersion, targetFirmware->versionString));
        }

        setState(State::GotoRecovery, QStringLiteral("进入 recovery"));
        if (!gotoRecovery(errorMessage)) {
            return false;
        }
        closePort();
        msleep(2000);

        setState(State::WaitRecoveryPort, QStringLiteral("等待 recovery 串口重新枚举"));
        if (!waitRecoveryPhysicalPort(&foundPort, errorMessage)) {
            return false;
        }
    }

    emit progressChanged(cycleIndex, 0);
    setState(State::SendMd5, QStringLiteral("发送 MD5"));
    if (!sendMd5(*targetFirmware, errorMessage)) {
        return false;
    }
    emit progressChanged(cycleIndex, 5);

    setState(State::GotoTransparent, QStringLiteral("进入透传模式"));
    if (!gotoTransparent(static_cast<quint32>(targetFirmware->data.size()), errorMessage)) {
        return false;
    }
    emit progressChanged(cycleIndex, 10);

    setState(State::TransferData, QStringLiteral("传输固件数据"));
    if (!transferData(*targetFirmware, cycleIndex, 10, errorMessage)) {
        return false;
    }

    setState(State::TransferDone, QStringLiteral("结束透传"));
    if (!transferDone(errorMessage)) {
        return false;
    }
    emit progressChanged(cycleIndex, 75);

    setState(State::WaitDeviceUpdate, QStringLiteral("等待设备升级完成"));
    if (!waitDeviceUpdate(cycleIndex, &currentVersion, errorMessage)) {
        return false;
    }
    emit logMessage(QStringLiteral("设备升级完成响应版本：%1").arg(currentVersion));

    closePort();
    setState(State::WaitNormalPort, QStringLiteral("等待设备正常重启并复查版本"));
    if (!waitNormalPortAndCheckVersion(*targetFirmware, cycleIndex, &currentVersion, errorMessage)) {
        return false;
    }
    emit logMessage(QStringLiteral("正常系统复查版本：%1").arg(currentVersion));
    emit progressChanged(cycleIndex, 100);
    setState(State::CycleDone, QStringLiteral("本轮升级完成"));
    return true;
}

bool StressUpgradeWorker::findCurrentPhysicalPort(SerialPort *outPort,
                                                  QString *errorMessage,
                                                  int timeoutMs,
                                                  PortSystem system)
{
    QElapsedTimer timer;
    timer.start();

    while (!shouldStop() && timer.elapsed() < timeoutMs) {
        const QList<SerialPort> ports = SerialPortQuery::availablePorts();
        for (const SerialPort &port : ports) {
            const bool samePort = !m_physicalPortKey.isEmpty()
                && port.physicalPortKey().compare(m_physicalPortKey, Qt::CaseInsensitive) == 0;
            if (!samePort) {
                continue;
            }
            if (!m_rules.isAllowed(port.info())) {
                continue;
            }
            if (!portMatchesSystem(port.info(), system)) {
                continue;
            }
            if (outPort) {
                *outPort = port;
            }
            emit logMessage(QStringLiteral("匹配到物理端口：%1，%2")
                .arg(port.portName(), m_rules.describe(port.info())));
            return true;
        }
        msleep(500);
    }

    if (errorMessage) {
        *errorMessage = shouldStop()
            ? QStringLiteral("升级已停止")
            : QStringLiteral("等待同一物理口重新枚举超时");
    }
    return false;
}

bool StressUpgradeWorker::waitRecoveryPhysicalPort(SerialPort *outPort, QString *errorMessage)
{
    QElapsedTimer timer;
    QElapsedTimer normalTimer;
    bool normalSeen = false;
    timer.start();

    while (!shouldStop() && timer.elapsed() < 60000) {
        const QList<SerialPort> ports = SerialPortQuery::availablePorts();
        bool sawNormal = false;

        for (const SerialPort &port : ports) {
            const bool samePort = !m_physicalPortKey.isEmpty()
                && port.physicalPortKey().compare(m_physicalPortKey, Qt::CaseInsensitive) == 0;
            if (!samePort || !m_rules.isAllowed(port.info())) {
                continue;
            }

            QString openError;
            if (!openPort(port, &openError)) {
                emit logMessage(openError);
                continue;
            }
            msleep(500);

            bool inRecovery = false;
            QString checkError;
            const bool checkOk = checkRecovery(&inRecovery, &checkError);
            if (checkOk && inRecovery) {
                if (outPort) {
                    *outPort = port;
                }
                emit logMessage(QStringLiteral("匹配到物理端口：%1，%2")
                    .arg(port.portName(), m_rules.describe(port.info())));
                return true;
            }

            closePort();
            if (checkOk && !inRecovery) {
                sawNormal = true;
            }
        }

        if (sawNormal) {
            if (!normalSeen) {
                normalSeen = true;
                normalTimer.restart();
            } else if (normalTimer.elapsed() >= 3000) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("设备进入 recovery 失败，已重新枚举为正常系统");
                }
                return false;
            }
        } else {
            normalSeen = false;
        }

        msleep(500);
    }

    if (errorMessage) {
        *errorMessage = shouldStop()
            ? QStringLiteral("升级已停止")
            : QStringLiteral("等待 recovery 串口重新枚举超时");
    }
    return false;
}

bool StressUpgradeWorker::waitOpenablePhysicalPort(SerialPort *outPort,
                                                   QString *errorMessage,
                                                   int timeoutMs,
                                                   PortSystem system)
{
    QElapsedTimer timer;
    timer.start();

    while (!shouldStop() && timer.elapsed() < timeoutMs) {
        const QList<SerialPort> ports = SerialPortQuery::availablePorts();
        for (const SerialPort &port : ports) {
            const bool samePort = !m_physicalPortKey.isEmpty()
                && port.physicalPortKey().compare(m_physicalPortKey, Qt::CaseInsensitive) == 0;
            if (!samePort || !m_rules.isAllowed(port.info()) || !portMatchesSystem(port.info(), system)) {
                continue;
            }

            QString openError;
            if (!openPort(port, &openError)) {
                emit logMessage(openError);
                msleep(300);
                continue;
            }

            if (outPort) {
                *outPort = port;
            }
            emit logMessage(QStringLiteral("匹配到可打开物理端口：%1，%2")
                .arg(port.portName(), m_rules.describe(port.info())));
            return true;
        }
        msleep(500);
    }

    if (errorMessage) {
        *errorMessage = shouldStop()
            ? QStringLiteral("升级已停止")
            : QStringLiteral("等待同一物理口可打开串口超时");
    }
    return false;
}

bool StressUpgradeWorker::portMatchesSystem(const SerialPort::Info &info, PortSystem system) const
{
    if (system == PortSystem::Any) {
        return true;
    }

    const QString typeString = m_rules.describe(info);
    const bool isRecovery = typeString.contains(QStringLiteral("recovery"), Qt::CaseInsensitive);
    return system == PortSystem::Recovery ? isRecovery : !isRecovery;
}

bool StressUpgradeWorker::physicalPortPresent(PortSystem system) const
{
    const QList<SerialPort> ports = SerialPortQuery::availablePorts();
    for (const SerialPort &port : ports) {
        const bool samePort = !m_physicalPortKey.isEmpty()
            && port.physicalPortKey().compare(m_physicalPortKey, Qt::CaseInsensitive) == 0;
        if (samePort && m_rules.isAllowed(port.info()) && portMatchesSystem(port.info(), system)) {
            return true;
        }
    }
    return false;
}

bool StressUpgradeWorker::openPort(const SerialPort &port, QString *errorMessage)
{
    closePort();
    m_port.reset(new SerialPort(port.info()));
    m_currentPortInfo = port.info();
    SerialPort::Config config = m_options.serialConfig;
    config.portName = port.portName();
    if (!m_port->open(config)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("打开串口失败：%1，%2").arg(config.portName, m_port->errorString());
        }
        return false;
    }
    m_port->clear();
    return true;
}

void StressUpgradeWorker::closePort()
{
    if (m_port) {
        m_port->close();
        m_port.reset();
    }
}

bool StressUpgradeWorker::checkRecovery(bool *inRecovery, QString *errorMessage)
{
    QByteArray response;
    if (!commandAck(0xE3, QByteArray(), &response, 3000, errorMessage)) {
        return false;
    }
    if (response.at(0) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("获取 recovery 状态失败");
        }
        return false;
    }
    *inRecovery = static_cast<quint8>(response.at(1)) == 0x02;
    return true;
}

bool StressUpgradeWorker::waitRecoveryReady(QString *errorMessage)
{
    QElapsedTimer timer;
    timer.start();

    while (!shouldStop() && timer.elapsed() < 30000) {
        bool inRecovery = false;
        QString checkError;
        if (checkRecovery(&inRecovery, &checkError) && inRecovery) {
            return true;
        }
        msleep(500);
    }

    if (errorMessage) {
        *errorMessage = shouldStop()
            ? QStringLiteral("升级已停止")
            : QStringLiteral("等待 recovery 协议就绪超时");
    }
    return false;
}

bool StressUpgradeWorker::checkVersion(QString *versionString, int *versionValue, QString *errorMessage)
{
    QByteArray response;
    if (!commandAck(0xE0, QByteArray(), &response, 3000, errorMessage)) {
        return false;
    }
    if (response.at(0) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("获取版本失败");
        }
        return false;
    }
    const int major = static_cast<quint8>(response.at(1));
    const int minor = static_cast<quint8>(response.at(2));
    const int patch = static_cast<quint8>(response.at(3));
    *versionString = QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
    *versionValue = major * 100 + minor * 10 + patch;
    return true;
}

bool StressUpgradeWorker::gotoRecovery(QString *errorMessage)
{
    QByteArray response;
    if (!commandAck(0xF1, QByteArray(), &response, 3000, errorMessage)) {
        return false;
    }
    if (response.at(0) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("进入 recovery 失败");
        }
        return false;
    }
    return true;
}

bool StressUpgradeWorker::sendMd5(const Firmware &firmware, QString *errorMessage)
{
    QByteArray packet = buildPacket(0xF3);
    packet.append(firmware.md5);
    if (!writeAll(packet, 3000, errorMessage)) {
        return false;
    }

    QByteArray response;
    if (!receivePacket(&response, 3000, errorMessage)) {
        return false;
    }
    if (response.at(0) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("MD5 校验失败，错误码 0x%1")
                .arg(static_cast<quint8>(response.at(0)), 2, 16, QChar('0'));
        }
        return false;
    }
    return true;
}

bool StressUpgradeWorker::gotoTransparent(quint32 fileSize, QString *errorMessage)
{
    QByteArray payload;
    payload.append(static_cast<char>((fileSize >> 24) & 0xff));
    payload.append(static_cast<char>((fileSize >> 16) & 0xff));
    payload.append(static_cast<char>((fileSize >> 8) & 0xff));
    payload.append(static_cast<char>(fileSize & 0xff));
    payload.append(char(0));

    QByteArray response;
    if (!commandAck(0xF5, payload, &response, 3000, errorMessage)) {
        return false;
    }
    if (response.at(0) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("进入透传模式失败");
        }
        return false;
    }
    return true;
}

bool StressUpgradeWorker::transferData(const Firmware &firmware, int cycleIndex, int startProgress, QString *errorMessage)
{
    int offset = 0;
    while (offset < firmware.data.size() && !shouldStop()) {
        const int chunkSize = qMin(kBigPacketSize, firmware.data.size() - offset);
        const char *chunk = firmware.data.constData() + offset;

        QByteArray header;
        header.append(static_cast<char>((chunkSize >> 24) & 0xff));
        header.append(static_cast<char>((chunkSize >> 16) & 0xff));
        header.append(static_cast<char>((chunkSize >> 8) & 0xff));
        header.append(static_cast<char>(chunkSize & 0xff));
        header.append(static_cast<char>(Crc8::calculate(chunk, chunkSize)));

        if (!sendPacket(0xDA, header, 3000, errorMessage)) {
            return false;
        }
        msleep(15);

        int sent = 0;
        while (sent < chunkSize && !shouldStop()) {
            const int packetSize = qMin(kTransferPacketSize, chunkSize - sent);
            if (!writeAll(QByteArray(chunk + sent, packetSize), 3000, errorMessage)) {
                return false;
            }
            sent += packetSize;
        }

        offset += chunkSize;
        const int progress = startProgress
            + static_cast<int>((static_cast<double>(offset) / firmware.data.size()) * 60.0);
        emit progressChanged(cycleIndex, qBound(0, progress, 70));
    }

    if (shouldStop()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("升级已停止");
        }
        return false;
    }
    return true;
}

bool StressUpgradeWorker::transferDone(QString *errorMessage)
{
    QByteArray response;
    if (!commandAck(0xF7, QByteArray(), &response, 3000, errorMessage)) {
        return false;
    }
    if (response.at(0) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("退出透传状态失败");
        }
        return false;
    }
    return true;
}

bool StressUpgradeWorker::waitDeviceUpdate(int cycleIndex, QString *versionString, QString *errorMessage)
{
    QElapsedTimer timer;
    QElapsedTimer lostTimer;
    bool portLost = false;
    timer.start();
    QByteArray response;
    while (!shouldStop() && timer.elapsed() < 300000) {
        const int progress = 75 + static_cast<int>((timer.elapsed() / 300000.0) * 23.0);
        emit progressChanged(cycleIndex, qBound(75, progress, 98));

        if (physicalPortPresent()) {
            portLost = false;
        } else {
            if (!portLost) {
                portLost = true;
                lostTimer.restart();
            } else if (lostTimer.elapsed() >= kUpgradePortLostFailMs) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("等待设备升级完成时串口断开，判定本轮失败");
                }
                return false;
            }
        }

        QString receiveError;
        if (receivePacket(&response, 1000, &receiveError)) {
            if (response.at(0) != 0) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("设备升级完成响应错误：0x%1")
                        .arg(static_cast<quint8>(response.at(0)), 2, 16, QChar('0'));
                }
                return false;
            }
            *versionString = QStringLiteral("%1.%2.%3")
                .arg(static_cast<quint8>(response.at(1)))
                .arg(static_cast<quint8>(response.at(2)))
                .arg(static_cast<quint8>(response.at(3)));
            emit progressChanged(cycleIndex, 98);
            return true;
        }
    }

    if (errorMessage) {
        *errorMessage = shouldStop() ? QStringLiteral("升级已停止") : QStringLiteral("等待设备升级完成超时");
    }
    return false;
}

bool StressUpgradeWorker::waitNormalPortAndCheckVersion(const Firmware &firmware,
                                                        int cycleIndex,
                                                        QString *versionString,
                                                        QString *errorMessage)
{
    QElapsedTimer timer;
    QElapsedTimer lostTimer;
    bool portLost = false;
    timer.start();

    while (!shouldStop() && timer.elapsed() < 180000) {
        emit progressChanged(cycleIndex, 98);

        SerialPort foundPort;
        QString findError;
        if (!findCurrentPhysicalPort(&foundPort, &findError, 2000, PortSystem::Normal)) {
            if (physicalPortPresent()) {
                portLost = false;
            } else {
                if (!portLost) {
                    portLost = true;
                    lostTimer.restart();
                } else if (lostTimer.elapsed() >= kNormalPortLostFailMs) {
                    if (errorMessage) {
                        *errorMessage = QStringLiteral("等待正常系统串口时设备断开，判定本轮失败");
                    }
                    return false;
                }
            }
            msleep(300);
            continue;
        }
        portLost = false;

        QString openError;
        if (!openPort(foundPort, &openError)) {
            emit logMessage(openError);
            msleep(500);
            continue;
        }
        msleep(500);

        bool inRecovery = false;
        QString stateError;
        if (!checkRecovery(&inRecovery, &stateError)) {
            closePort();
            msleep(500);
            continue;
        }

        if (inRecovery) {
            closePort();
            msleep(500);
            continue;
        }

        int versionValue = 0;
        QString checkedVersion;
        if (!checkVersion(&checkedVersion, &versionValue, errorMessage)) {
            closePort();
            return false;
        }

        if (versionValue != firmware.versionValue) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("升级后版本不匹配，目标:%1，当前:%2")
                    .arg(firmware.versionString, checkedVersion);
            }
            return false;
        }

        if (versionString) {
            *versionString = checkedVersion;
        }
        return true;
    }

    if (errorMessage) {
        *errorMessage = shouldStop()
            ? QStringLiteral("升级已停止")
            : QStringLiteral("等待设备正常系统串口并复查版本超时");
    }
    return false;
}

bool StressUpgradeWorker::sendPacket(quint8 command, const QByteArray &payload5, int timeoutMs, QString *errorMessage)
{
    if (m_port && m_port->isOpen()) {
        m_port->clear(QSerialPort::Input);
    }
    return writeAll(buildPacket(command, payload5), timeoutMs, errorMessage);
}

bool StressUpgradeWorker::receivePacket(QByteArray *data5, int timeoutMs, QString *errorMessage)
{
    QByteArray buffer;
    QElapsedTimer timer;
    timer.start();
    while (!shouldStop() && timer.elapsed() < timeoutMs) {
        bool ok = false;
        const int remainMs = qMax(1, timeoutMs - static_cast<int>(timer.elapsed()));
        if (!m_port || !m_port->isOpen()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("串口未打开");
            }
            return false;
        }
        const QByteArray data = m_port->read(256, remainMs, &ok);
        if (ok && !data.isEmpty()) {
            buffer.append(data);
        }

        for (int i = 0; i + 8 <= buffer.size(); ++i) {
            if (static_cast<quint8>(buffer.at(i)) == 0xAA
                && static_cast<quint8>(buffer.at(i + 1)) == 0x55) {
                *data5 = buffer.mid(i + 3, 5);
                return true;
            }
        }
    }

    if (errorMessage) {
        *errorMessage = shouldStop()
            ? QStringLiteral("升级已停止")
            : QStringLiteral("串口接收超时，已接收 %1 字节：%2")
                .arg(buffer.size())
                .arg(QString::fromLatin1(buffer.toHex(' ').left(96)));
    }
    return false;
}

bool StressUpgradeWorker::commandAck(quint8 command,
                                     const QByteArray &payload5,
                                     QByteArray *response5,
                                     int timeoutMs,
                                     QString *errorMessage)
{
    if (!sendPacket(command, payload5, timeoutMs, errorMessage)) {
        return false;
    }
    return receivePacket(response5, timeoutMs, errorMessage);
}

bool StressUpgradeWorker::writeAll(const QByteArray &data, int timeoutMs, QString *errorMessage)
{
    if (!m_port || !m_port->isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("串口未打开");
        }
        return false;
    }
    if (!m_port->writeAll(data, timeoutMs)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("串口写入失败：%1").arg(m_port->errorString());
        }
        return false;
    }
    return true;
}

QByteArray StressUpgradeWorker::buildPacket(quint8 command, const QByteArray &payload5) const
{
    QByteArray payload = payload5.left(5);
    while (payload.size() < 5) {
        payload.append(char(0));
    }

    QByteArray packet;
    packet.reserve(8);
    packet.append(char(0xAA));
    packet.append(char(0x55));
    packet.append(static_cast<char>(command));
    packet.append(payload);
    return packet;
}
