#ifndef SERIALPORTQUERY_H
#define SERIALPORTQUERY_H

#include <QByteArray>
#include <QIODeviceBase>
#include <QList>
#include <QSerialPort>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

/**
 * @brief 串口对象。
 *
 * 这个类同时保存串口的枚举信息和实际读写句柄：
 * - SerialPortQuery::availablePorts() 返回的每个 SerialPort 都带有 Info 信息；
 * - 调用 open/read/write/close 可以直接对这个串口进行阻塞式收发；
 * - read/write 的 timeoutMs 是总超时时间，适合在线程中使用；
 * - 不建议在 UI 线程里使用较长超时，避免界面卡顿。
 */
class SerialPort
{
public:
    /**
     * @brief 串口枚举信息。
     *
     * portName/vid/pid 用于识别设备本身；physicalPortKey 用于判断物理 USB 口。
     * 当不同 VID/PID 的设备插到同一个物理接口时，physicalPortKey 通常保持一致。
     */
    struct Info
    {
        QString portName;                ///< 串口名，例如 COM9。
        QString friendlyName;            ///< 设备管理器中的友好名称。
        QString vid;                     ///< USB Vendor ID，例如 A108。
        QString pid;                     ///< USB Product ID，例如 2240。
        QString instanceId;              ///< Windows 设备实例 ID。
        QString hardwareId;              ///< Windows 硬件 ID。
        QString locationInfo;            ///< 传统位置描述，可能不如 physicalPortKey 稳定。
        QString physicalPortKey;         ///< 归一化后的 USB 拓扑路径，用于判断物理接口。
        QString interfaceNumber;         ///< 复合 USB 设备接口号，例如 MI_04。
        QStringList locationPaths;       ///< 原始设备位置路径。
        QString parentInstanceId;        ///< 父设备实例 ID。
        QStringList parentLocationPaths; ///< 父设备位置路径。
    };

    /**
     * @brief 串口打开参数。
     */
    struct Config
    {
        QString portName;                                            ///< 串口名，例如 COM9。
        qint32 baudRate = 115200;                                     ///< 波特率。
        QSerialPort::DataBits dataBits = QSerialPort::Data8;          ///< 数据位。
        QSerialPort::Parity parity = QSerialPort::NoParity;           ///< 校验位。
        QSerialPort::StopBits stopBits = QSerialPort::OneStop;        ///< 停止位。
        QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl; ///< 流控。
        QIODeviceBase::OpenMode openMode = QIODeviceBase::ReadWrite;  ///< 打开模式。
    };

    SerialPort();
    explicit SerialPort(const Info &info);

    /**
     * @brief 获取/设置枚举信息。
     */
    const Info& info() const;
    void setInfo(const Info &info);

    /**
     * @brief 快捷获取串口名和物理口 key。
     */
    QString portName() const;
    QString physicalPortKey() const;

    /**
     * @brief 打开串口。
     *
     * open(baudRate) 会使用当前对象 Info 中的 portName。
     */
    bool open(const Config &config);
    bool open(qint32 baudRate = 115200);
    bool open(const QString &portName, qint32 baudRate = 115200);

    /**
     * @brief 关闭串口。
     */
    void close();

    bool isOpen() const;

    /**
     * @brief 获取最近一次错误信息。
     */
    QString errorString() const;
    QSerialPort::SerialPortError error() const;

    qint64 bytesAvailable() const;

    /**
     * @brief 清空输入/输出缓冲区。
     */
    void clear(QSerialPort::Directions directions = QSerialPort::AllDirections);

    /**
     * @brief 阻塞等待可读/写完成信号。
     */
    bool waitForReadyRead(int timeoutMs);
    bool waitForBytesWritten(int timeoutMs);

    /**
     * @brief 写数据。
     *
     * write 只保证提交一次写请求；writeAll 会循环直到全部数据写完或超时。
     * 返回 false/-1 时可调用 errorString() 查看原因。
     */
    qint64 write(const QByteArray &data, int timeoutMs = 3000);
    bool writeAll(const QByteArray &data, int timeoutMs = 3000);

    /**
     * @brief 读数据。
     *
     * readAllAvailable 读取当前缓冲区已有数据，不等待。
     * read 最多读取 maxSize 字节，必要时等待一次数据到达。
     * readBytes 会等待直到收到 expectedSize 字节或总超时。
     */
    QByteArray readAllAvailable();
    QByteArray read(int maxSize, int timeoutMs = 3000, bool *ok = nullptr);
    QByteArray readBytes(int expectedSize, int timeoutMs = 3000, bool *ok = nullptr);

private:
    QSharedPointer<QSerialPort> m_port;
    Info m_info;
    QString m_lastError;
};

/**
 * @brief 串口枚举和辅助工具。
 *
 * Windows 下通过 SetupAPI/CfgMgr32 获取 COM 口、VID/PID 和物理 USB 拓扑路径。
 */
class SerialPortQuery
{
public:
    /**
     * @brief 枚举当前系统所有串口。
     */
    static QList<SerialPort> availablePorts();

    /**
     * @brief 按 COM 名查找串口。
     */
    static bool findByPortName(const QString &portName, SerialPort *outPort);

    /**
     * @brief 判断两个串口是否在同一个物理 USB 接口。
     */
    static bool samePhysicalPort(const SerialPort &left, const SerialPort &right);

    /**
     * @brief 生成用于比较串口列表变化的快照字符串。
     */
    static QString snapshot(const QList<SerialPort> &ports);

    /**
     * @brief 生成调试日志文本。
     */
    static QString toDebugText(const QList<SerialPort> &ports);
};

#endif // SERIALPORTQUERY_H
