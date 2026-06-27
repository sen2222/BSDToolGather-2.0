#ifndef FONTCONVERTCORE_H
#define FONTCONVERTCORE_H

#include <QString>
#include <QStringList>

class FontConvertCore
{
public:
    enum Format
    {
        RGBA1555 = 0,
    };

    struct Options
    {
        QString fontPath;
        QString outputPath;
        int width = 0;
        int height = 0;
        int fontSize = 0;
        QString suffix;
        QString chars;
        Format format = RGBA1555;
    };

    struct Result
    {
        bool success = false;
        QString errorMessage;
        QStringList messages;
        int glyphCount = 0;
    };

    static Result convert(const Options &options);
};

#endif // FONTCONVERTCORE_H
