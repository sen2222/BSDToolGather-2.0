#include "cdccommandcore.h"

#include "Crc/crc8.h"

namespace {

constexpr quint8 kHeader0 = 0xEA;
constexpr quint8 kHeader1 = 0xFF;
constexpr quint8 kTimeOsdCommand = 0xAB;

#pragma pack(push, 1)
struct CAMERA_TIME_OSD_CTRL_S
{
    quint32 time_stamp;
    quint8 mjpeg_show_flag;
    quint16 mjpeg_show_x;
    quint16 mjpeg_show_y;
    quint8 h264_show_flag;
    quint16 h264_show_x;
    quint16 h264_show_y;
};
#pragma pack(pop)

static_assert(sizeof(CAMERA_TIME_OSD_CTRL_S) == 14, "CAMERA_TIME_OSD_CTRL_S size must be 14 bytes");

constexpr quint16 kTimeOsdDataSize = sizeof(CAMERA_TIME_OSD_CTRL_S);

void appendU8(QByteArray &data, quint8 value)
{
    data.append(static_cast<char>(value));
}

void appendBe16(QByteArray &data, quint16 value)
{
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
}

void appendBe32(QByteArray &data, quint32 value)
{
    data.append(static_cast<char>((value >> 24) & 0xFF));
    data.append(static_cast<char>((value >> 16) & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
}

QByteArray serializeTimeOsdData(const CAMERA_TIME_OSD_CTRL_S &ctrl)
{
    QByteArray data;
    data.reserve(kTimeOsdDataSize);
    appendBe32(data, ctrl.time_stamp);
    appendU8(data, ctrl.mjpeg_show_flag);
    appendBe16(data, ctrl.mjpeg_show_x);
    appendBe16(data, ctrl.mjpeg_show_y);
    appendU8(data, ctrl.h264_show_flag);
    appendBe16(data, ctrl.h264_show_x);
    appendBe16(data, ctrl.h264_show_y);
    return data;
}

} // namespace

QByteArray CdcCommandCore::buildTimeOsdPacket(const TimeOsdOptions &options)
{
    const CAMERA_TIME_OSD_CTRL_S ctrl = {
        options.timeStamp,
        options.mjpegShowFlag,
        options.mjpegShowX,
        options.mjpegShowY,
        options.h264ShowFlag,
        options.h264ShowX,
        options.h264ShowY
    };
    const QByteArray payload = serializeTimeOsdData(ctrl);

    QByteArray packet;
    packet.reserve(2 + 1 + 2 + kTimeOsdDataSize + 1);

    appendU8(packet, kHeader0);
    appendU8(packet, kHeader1);
    appendU8(packet, kTimeOsdCommand);
    appendBe16(packet, kTimeOsdDataSize);
    packet.append(payload);

    appendU8(packet, Crc8::calculate(payload));
    return packet;
}

CdcCommandCore::CommandResult CdcCommandCore::sendTimeOsdCommand(const SerialPort::Config &config,
                                                                 const TimeOsdOptions &options,
                                                                 int writeTimeoutMs,
                                                                 int readTimeoutMs)
{
    CommandResult result;
    result.request = buildTimeOsdPacket(options);

    SerialPort port;
    if (!port.open(config)) {
        result.errorMessage = QStringLiteral("打开串口失败：%1").arg(port.errorString());
        return result;
    }

    result = sendTimeOsdCommand(&port, options, writeTimeoutMs, readTimeoutMs);
    port.close();
    return result;
}

CdcCommandCore::CommandResult CdcCommandCore::sendTimeOsdCommand(SerialPort *port,
                                                                 const TimeOsdOptions &options,
                                                                 int writeTimeoutMs,
                                                                 int readTimeoutMs)
{
    CommandResult result;
    result.request = buildTimeOsdPacket(options);

    if (!port || !port->isOpen()) {
        result.errorMessage = QStringLiteral("串口未打开");
        return result;
    }

    port->clear(QSerialPort::Input);
    if (!port->writeAll(result.request, writeTimeoutMs)) {
        result.errorMessage = QStringLiteral("发送 CDC 指令失败：%1").arg(port->errorString());
        return result;
    }

    if (port->waitForReadyRead(readTimeoutMs)) {
        result.response = port->readAllAvailable();
    }

    result.success = true;
    return result;
}
