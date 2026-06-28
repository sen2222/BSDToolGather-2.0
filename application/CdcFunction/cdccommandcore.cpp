#include "cdccommandcore.h"

#include "Crc/crc8.h"

#define CDC_PACKET_HEADER0 0xEA
#define CDC_PACKET_HEADER1 0xFF
#define CDC_TIME_OSD_COMMAND 0xAB

namespace {

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

} // namespace

QByteArray CdcCommandCore::buildTimeOsdPacket(const CAMERA_TIME_OSD_CTRL_S &timeOsdCtrl)
{
    QByteArray payload;
    payload.reserve(sizeof(CAMERA_TIME_OSD_CTRL_S));
    appendBe32(payload, timeOsdCtrl.time_stamp);
    appendU8(payload, timeOsdCtrl.mjpeg_show_flag);
    appendBe16(payload, timeOsdCtrl.mjpeg_show_x);
    appendBe16(payload, timeOsdCtrl.mjpeg_show_y);
    appendU8(payload, timeOsdCtrl.h264_show_flag);
    appendBe16(payload, timeOsdCtrl.h264_show_x);
    appendBe16(payload, timeOsdCtrl.h264_show_y);

    QByteArray packet;
    packet.reserve(2 + 1 + 2 + sizeof(CAMERA_TIME_OSD_CTRL_S) + 1);

    appendU8(packet, CDC_PACKET_HEADER0);
    appendU8(packet, CDC_PACKET_HEADER1);
    appendU8(packet, CDC_TIME_OSD_COMMAND);
    appendBe16(packet, static_cast<quint16>(sizeof(CAMERA_TIME_OSD_CTRL_S)));
    packet.append(payload);

    appendU8(packet, Crc8::calculate(payload));
    return packet;
}

CdcCommandCore::CommandResult CdcCommandCore::sendTimeOsdCommand(const SerialPort::Config &config,
                                                                 const CAMERA_TIME_OSD_CTRL_S &timeOsdCtrl,
                                                                 int writeTimeoutMs,
                                                                 int readTimeoutMs)
{
    CommandResult result;
    result.request = buildTimeOsdPacket(timeOsdCtrl);

    SerialPort port;
    if (!port.open(config)) {
        result.errorMessage = QStringLiteral("打开串口失败：%1").arg(port.errorString());
        return result;
    }

    result = sendTimeOsdCommand(&port, timeOsdCtrl, writeTimeoutMs, readTimeoutMs);
    port.close();
    return result;
}

CdcCommandCore::CommandResult CdcCommandCore::sendTimeOsdCommand(SerialPort *port,
                                                                 const CAMERA_TIME_OSD_CTRL_S &timeOsdCtrl,
                                                                 int writeTimeoutMs,
                                                                 int readTimeoutMs)
{
    CommandResult result;
    result.request = buildTimeOsdPacket(timeOsdCtrl);

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
