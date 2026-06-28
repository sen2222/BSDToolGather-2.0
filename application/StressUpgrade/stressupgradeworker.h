#ifndef STRESSUPGRADEWORKER_H
#define STRESSUPGRADEWORKER_H

#include "SerialPort/serialportquery.h"
#include "usbvidpidrules.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QThread>

#include <atomic>
#include <memory>

class StressUpgradeWorker : public QThread
{
    Q_OBJECT

public:
    enum class State
    {
        Idle,
        ValidatePort,
        LoadFirmware,
        FindPort,
        OpenPort,
        CheckRecovery,
        CheckVersion,
        GotoRecovery,
        WaitRecoveryPort,
        SendMd5,
        GotoTransparent,
        TransferData,
        TransferDone,
        WaitDeviceUpdate,
        WaitNormalPort,
        CycleDone,
        Stopping,
        Finished,
        Failed
    };
    Q_ENUM(State)

    struct Firmware
    {
        QString path;
        QByteArray data;
        QByteArray md5;
        QString versionString;
        int versionValue = 0;
    };

    struct Options
    {
        SerialPort::Info selectedPortInfo;
        SerialPort::Config serialConfig;
        QString firmwareAPath;
        QString firmwareBPath;
        int upgradeCount = 1;
        QString ruleFilePath;
    };

    struct Statistics
    {
        int total = 0;
        int finished = 0;
        int success = 0;
        int failed = 0;
        qint64 totalDurationMs = 0;
        qint64 lastDurationMs = 0;
    };

    explicit StressUpgradeWorker(const Options &options, QObject *parent = nullptr);
    ~StressUpgradeWorker() override;

    void requestStop();
    Statistics statistics() const;

signals:
    void stateChanged(StressUpgradeWorker::State state, QString text);
    void progressChanged(int cycleIndex, int progress);
    void statisticsChanged(StressUpgradeWorker::Statistics statistics);
    void logMessage(QString message);

protected:
    void run() override;

private:
    enum class PortSystem
    {
        Any,
        Normal,
        Recovery
    };

    static constexpr int kBigPacketSize = 8 * 1024;
    static constexpr int kTransferPacketSize = 380;
    static constexpr int kUpgradePortLostFailMs = 5000;
    static constexpr int kNormalPortLostFailMs = 30000;

    bool shouldStop() const;
    void setState(State state, const QString &text);
    void addFailure(const QString &message);
    void addSuccess(qint64 durationMs);
    void emitFinalSummary(const QString &title);
    QString formatDuration(qint64 durationMs) const;
    bool isStartEnumerateAbort(const QString &message) const;
    bool isDeviceLossFailure(const QString &message) const;

    bool loadFirmware(const QString &path, Firmware *firmware, QString *errorMessage) const;
    bool performOneUpgrade(const Firmware &firmware,
                           const Firmware &alternateFirmware,
                           int cycleIndex,
                           QString *errorMessage);

    bool findCurrentPhysicalPort(SerialPort *outPort,
                                 QString *errorMessage,
                                 int timeoutMs = 60000,
                                 PortSystem system = PortSystem::Any);
    bool waitRecoveryPhysicalPort(SerialPort *outPort, QString *errorMessage);
    bool waitOpenablePhysicalPort(SerialPort *outPort,
                                  QString *errorMessage,
                                  int timeoutMs,
                                  PortSystem system = PortSystem::Any);
    bool portMatchesSystem(const SerialPort::Info &info, PortSystem system) const;
    bool physicalPortPresent(PortSystem system = PortSystem::Any) const;
    bool openPort(const SerialPort &port, QString *errorMessage);
    void closePort();

    bool checkRecovery(bool *inRecovery, QString *errorMessage);
    bool waitRecoveryReady(QString *errorMessage);
    bool checkVersion(QString *versionString, int *versionValue, QString *errorMessage);
    bool gotoRecovery(QString *errorMessage);
    bool sendMd5(const Firmware &firmware, QString *errorMessage);
    bool gotoTransparent(quint32 fileSize, QString *errorMessage);
    bool transferData(const Firmware &firmware, int cycleIndex, int startProgress, QString *errorMessage);
    bool transferDone(QString *errorMessage);
    bool waitDeviceUpdate(int cycleIndex, QString *versionString, QString *errorMessage);
    bool waitNormalPortAndCheckVersion(const Firmware &firmware,
                                       int cycleIndex,
                                       QString *versionString,
                                       QString *errorMessage);

    bool sendPacket(quint8 command, const QByteArray &payload5, int timeoutMs, QString *errorMessage);
    bool receivePacket(QByteArray *data5, int timeoutMs, QString *errorMessage);
    bool commandAck(quint8 command, const QByteArray &payload5, QByteArray *response5, int timeoutMs, QString *errorMessage);
    bool writeAll(const QByteArray &data, int timeoutMs, QString *errorMessage);

    QByteArray buildPacket(quint8 command, const QByteArray &payload5 = QByteArray()) const;

    Options m_options;
    UsbVidPidRules m_rules;
    std::unique_ptr<SerialPort> m_port;
    SerialPort::Info m_currentPortInfo;
    QString m_physicalPortKey;
    std::atomic_bool m_stopRequested;
    Statistics m_statistics;
};

Q_DECLARE_METATYPE(StressUpgradeWorker::State)
Q_DECLARE_METATYPE(StressUpgradeWorker::Statistics)

#endif // STRESSUPGRADEWORKER_H
