#ifndef CDCCOMMANDCORE_H
#define CDCCOMMANDCORE_H

#include "SerialPort/serialportquery.h"

#include <QByteArray>
#include <QString>
#include <QtGlobal>

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

class CdcCommandCore
{
public:
    struct CommandResult
    {
        bool success = false;
        QString errorMessage;
        QByteArray request;
        QByteArray response;
    };

    static QByteArray buildTimeOsdPacket(const CAMERA_TIME_OSD_CTRL_S &timeOsdCtrl);
    static CommandResult sendTimeOsdCommand(SerialPort *port,
                                            const CAMERA_TIME_OSD_CTRL_S &timeOsdCtrl,
                                            int writeTimeoutMs = 3000,
                                            int readTimeoutMs = 1000);
    static CommandResult sendTimeOsdCommand(const SerialPort::Config &config,
                                            const CAMERA_TIME_OSD_CTRL_S &timeOsdCtrl,
                                            int writeTimeoutMs = 3000,
                                            int readTimeoutMs = 1000);
};

#endif // CDCCOMMANDCORE_H
