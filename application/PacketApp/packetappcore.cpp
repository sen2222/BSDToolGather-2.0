#include "packetappcore.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <algorithm>
#include <utility>

namespace {

constexpr int kOtaBlockSize = 64;
constexpr quint64 kMaxUInt32 = 0xFFFFFFFFULL;

struct SelectedPartition
{
    quint32 type;
    QString name;
    QString filePath;
    int order;
    quint32 size;
};

void writeLe32(QByteArray &buffer, int offset, quint32 value)
{
    buffer[offset + 0] = static_cast<char>(value & 0xFF);
    buffer[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
    buffer[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
    buffer[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
}

void writeLe64(QByteArray &buffer, int offset, quint64 value)
{
    for (int i = 0; i < 8; ++i) {
        buffer[offset + i] = static_cast<char>((value >> (i * 8)) & 0xFF);
    }
}

QByteArray makePacketHeader(quint8 major, quint8 minor, quint8 patch, quint32 totalSize, quint8 entryCount)
{
    QByteArray header(kOtaBlockSize, '\0');
    header[0] = static_cast<char>(major);
    header[1] = static_cast<char>(minor);
    header[2] = static_cast<char>(patch);
    writeLe32(header, 3, totalSize);
    header[7] = static_cast<char>(entryCount);
    writeLe32(header, 8, kOtaBlockSize);
    return header;
}

QByteArray makePartitionEntry(quint32 type, quint32 size, quint32 dataOffset)
{
    QByteArray entry(kOtaBlockSize, '\0');
    writeLe32(entry, 0, type);
    writeLe32(entry, 4, size);
    writeLe32(entry, 8, dataOffset);
    writeLe64(entry, 12, 0);
    return entry;
}

bool containsInvalidFileNameChar(const QString &value)
{
    static const QString invalidChars = QStringLiteral("\\/:*?\"<>|");
    for (const QChar &ch : value) {
        if (invalidChars.contains(ch)) {
            return true;
        }
    }
    return false;
}

QString formatFileSize(quint64 size)
{
    if (size < 1024) {
        return QStringLiteral("%1 B").arg(size);
    }
    if (size < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(QString::number(size / 1024.0, 'f', 2));
    }
    return QStringLiteral("%1 MB").arg(QString::number(size / 1024.0 / 1024.0, 'f', 2));
}

}

PacketAppCore::PackageResult PacketAppCore::pack(const PackageOptions &options)
{
    PackageResult result;

    const QString projectName = options.projectName.trimmed();
    const QString customerName = options.customerName.trimmed();
    if (projectName.isEmpty()) {
        result.errorMessage = QStringLiteral("请输入项目名称");
        return result;
    }
    if (customerName.isEmpty()) {
        result.errorMessage = QStringLiteral("请输入客户名称");
        return result;
    }
    if (containsInvalidFileNameChar(projectName) || containsInvalidFileNameChar(customerName)) {
        result.errorMessage = QStringLiteral("项目名称或客户名称包含文件名非法字符");
        return result;
    }
    if (options.versionMajor < 0 || options.versionMajor > 255 ||
        options.versionMinor < 0 || options.versionMinor > 255 ||
        options.versionPatch < 0 || options.versionPatch > 255) {
        result.errorMessage = QStringLiteral("版本号需要在 0 到 255 之间");
        return result;
    }
    if (!(options.alignment == 0 || options.alignment == 8 || options.alignment == 16 || options.alignment == 32)) {
        result.errorMessage = QStringLiteral("对齐方式不支持");
        return result;
    }
    if (options.partitions.isEmpty()) {
        result.errorMessage = QStringLiteral("请至少选择一个分区固件");
        return result;
    }
    if (options.partitions.size() > 255) {
        result.errorMessage = QStringLiteral("分区数量超过限制");
        return result;
    }

    QVector<SelectedPartition> selectedPartitions;
    QSet<int> usedOrders;
    quint64 totalSize = kOtaBlockSize;
    for (const PartitionInput &partition : options.partitions) {
        if (usedOrders.contains(partition.order)) {
            result.errorMessage = QStringLiteral("分区顺序不能重复：%1").arg(partition.order);
            return result;
        }
        usedOrders.insert(partition.order);

        const QFileInfo fileInfo(partition.filePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            result.errorMessage = QStringLiteral("%1 分区固件文件不存在").arg(partition.name);
            return result;
        }
        if (fileInfo.size() > static_cast<qint64>(kMaxUInt32)) {
            result.errorMessage = QStringLiteral("%1 分区固件超过 4GB，无法打包").arg(partition.name);
            return result;
        }

        selectedPartitions.push_back({
            partition.type,
            partition.name,
            partition.filePath,
            partition.order,
            static_cast<quint32>(fileInfo.size())
        });
        totalSize += kOtaBlockSize + static_cast<quint64>(fileInfo.size());
    }

    std::sort(selectedPartitions.begin(), selectedPartitions.end(),
              [](const SelectedPartition &left, const SelectedPartition &right) {
                  return left.order < right.order;
              });

    if (options.alignment > 0) {
        const quint64 remainder = totalSize % static_cast<quint64>(options.alignment);
        if (remainder != 0) {
            totalSize += static_cast<quint64>(options.alignment) - remainder;
        }
    }
    if (totalSize > kMaxUInt32) {
        result.errorMessage = QStringLiteral("OTA 包超过 4GB，无法打包");
        return result;
    }

    QDir outputDir(absoluteOutputDir(options.outputDir));
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        result.errorMessage = QStringLiteral("无法创建输出目录：%1").arg(outputDir.absolutePath());
        return result;
    }

    const QString outputPath = outputDir.filePath(buildOutputFileName(
        projectName,
        customerName,
        options.versionMajor,
        options.versionMinor,
        options.versionPatch));

    for (const SelectedPartition &partition : std::as_const(selectedPartitions)) {
        if (QFileInfo(partition.filePath).absoluteFilePath() == QFileInfo(outputPath).absoluteFilePath()) {
            result.errorMessage = QStringLiteral("输出文件不能覆盖输入固件：%1").arg(partition.name);
            return result;
        }
    }

    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = QStringLiteral("无法创建输出文件：%1").arg(outputPath);
        return result;
    }

    outputFile.write(makePacketHeader(
        static_cast<quint8>(options.versionMajor),
        static_cast<quint8>(options.versionMinor),
        static_cast<quint8>(options.versionPatch),
        static_cast<quint32>(totalSize),
        static_cast<quint8>(selectedPartitions.size())));

    quint64 currentOffset = kOtaBlockSize;
    for (const SelectedPartition &partition : std::as_const(selectedPartitions)) {
        const quint32 dataOffset = static_cast<quint32>(currentOffset + kOtaBlockSize);
        outputFile.write(makePartitionEntry(partition.type, partition.size, dataOffset));
        currentOffset += kOtaBlockSize;

        QFile inputFile(partition.filePath);
        if (!inputFile.open(QIODevice::ReadOnly)) {
            outputFile.close();
            QFile::remove(outputPath);
            result.errorMessage = QStringLiteral("无法读取 %1 分区固件").arg(partition.name);
            return result;
        }

        while (!inputFile.atEnd()) {
            const QByteArray chunk = inputFile.read(1024 * 1024);
            if (chunk.isEmpty() && inputFile.error() != QFile::NoError) {
                outputFile.close();
                QFile::remove(outputPath);
                result.errorMessage = QStringLiteral("读取 %1 分区固件失败").arg(partition.name);
                return result;
            }
            if (outputFile.write(chunk) != chunk.size()) {
                outputFile.close();
                QFile::remove(outputPath);
                result.errorMessage = QStringLiteral("写入 OTA 包失败");
                return result;
            }
            currentOffset += static_cast<quint64>(chunk.size());
        }

        result.messages.append(QStringLiteral("OTA 分区 %1 已加入，顺序：%2，大小：%3")
            .arg(partition.name)
            .arg(partition.order)
            .arg(formatFileSize(partition.size)));
    }

    if (currentOffset < totalSize) {
        const QByteArray padding(static_cast<int>(totalSize - currentOffset), '\0');
        if (outputFile.write(padding) != padding.size()) {
            outputFile.close();
            QFile::remove(outputPath);
            result.errorMessage = QStringLiteral("写入对齐补零失败");
            return result;
        }
    }

    if (!outputFile.flush()) {
        outputFile.close();
        QFile::remove(outputPath);
        result.errorMessage = QStringLiteral("写入 OTA 包失败");
        return result;
    }
    outputFile.close();

    result.success = true;
    result.outputPath = outputPath;
    result.totalSize = totalSize;
    result.messages.append(QStringLiteral("OTA 打包完成：%1，大小：%2，对齐：%3")
        .arg(outputPath)
        .arg(formatFileSize(totalSize))
        .arg(options.alignment > 0 ? QStringLiteral("%1 字节").arg(options.alignment) : QStringLiteral("不对齐")));
    return result;
}

QString PacketAppCore::buildOutputFileName(const QString &projectName,
                                           const QString &customerName,
                                           int versionMajor,
                                           int versionMinor,
                                           int versionPatch)
{
    return QStringLiteral("%1-%2-%3.%4.%5-OTA.bin")
        .arg(projectName.trimmed())
        .arg(customerName.trimmed())
        .arg(versionMajor)
        .arg(versionMinor)
        .arg(versionPatch);
}

QString PacketAppCore::absoluteOutputDir(const QString &outputDir)
{
    const QString path = outputDir.trimmed().isEmpty() ? QStringLiteral("out/packet_app/") : outputDir.trimmed();
    QDir dir(path);
    if (dir.isAbsolute()) {
        return dir.absolutePath();
    }
    return QDir(QDir::currentPath()).absoluteFilePath(path);
}
