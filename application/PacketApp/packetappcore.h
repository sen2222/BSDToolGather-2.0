#ifndef PACKETAPPCORE_H
#define PACKETAPPCORE_H

#include <QString>
#include <QStringList>
#include <QVector>

class PacketAppCore
{
public:
    enum PartitionType
    {
        PT_BOOT = 0,
        PT_TAG,
        PT_KERNEL,
        PT_ROOTFS,
        PT_RECOVERY,
        PT_SYSTEM,
        PT_CONFIG,
        PT_MODEL,
        PT_DATA,
    };

    struct PartitionInput
    {
        quint32 type;
        QString name;
        QString filePath;
        int order;
    };

    struct PackageOptions
    {
        QString projectName;
        QString customerName;
        int versionMajor;
        int versionMinor;
        int versionPatch;
        int alignment;
        QString outputDir;
        QVector<PartitionInput> partitions;
    };

    struct PackageResult
    {
        bool success = false;
        QString outputPath;
        QString errorMessage;
        quint64 totalSize = 0;
        QStringList messages;
    };

    static PackageResult pack(const PackageOptions &options);
    static QString buildOutputFileName(const QString &projectName,
                                       const QString &customerName,
                                       int versionMajor,
                                       int versionMinor,
                                       int versionPatch);
    static QString absoluteOutputDir(const QString &outputDir);
};

#endif // PACKETAPPCORE_H
