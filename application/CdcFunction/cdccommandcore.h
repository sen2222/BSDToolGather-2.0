#ifndef CDCCOMMANDCORE_H
#define CDCCOMMANDCORE_H

#include "SerialPort/serialportquery.h"

#include <QByteArray>
#include <QString>

class CdcCommandCore
{
public:
    struct TimeOsdOptions
    {
        quint32 timeStamp = 0;
        quint8 mjpegShowFlag = 0;
        quint16 mjpegShowX = 0;
        quint16 mjpegShowY = 0;
        quint8 h264ShowFlag = 0;
        quint16 h264ShowX = 0;
        quint16 h264ShowY = 0;
    };

    struct CommandResult
    {
        bool success = false;
        QString errorMessage;
        QByteArray request;
        QByteArray response;
    };

    static QByteArray buildTimeOsdPacket(const TimeOsdOptions &options);
    static CommandResult sendTimeOsdCommand(SerialPort *port,
                                            const TimeOsdOptions &options,
                                            int writeTimeoutMs = 3000,
                                            int readTimeoutMs = 1000);
    static CommandResult sendTimeOsdCommand(const SerialPort::Config &config,
                                            const TimeOsdOptions &options,
                                            int writeTimeoutMs = 3000,
                                            int readTimeoutMs = 1000);
};

#endif // CDCCOMMANDCORE_H
