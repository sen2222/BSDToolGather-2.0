#ifndef CRC8_H
#define CRC8_H

#include <QByteArray>
#include <QtGlobal>

class Crc8
{
public:
    static quint8 calculate(const char *data, int length);
    static quint8 calculate(const QByteArray &data);
};

#endif // CRC8_H
