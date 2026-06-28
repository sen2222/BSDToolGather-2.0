#include "packetappwidget.h"

#include "alltoolfun.h"
#include "packetappcore.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>






PacketAppWidget::PacketAppWidget(QWidget *parent)
    : QWidget(parent)
    , m_projectNameEdit(nullptr)
    , m_customerNameEdit(nullptr)
    , m_versionMajor(nullptr)
    , m_versionMinor(nullptr)
    , m_versionPatch(nullptr)
    , m_alignCombo(nullptr)
    , m_packButton(nullptr)
{
    m_outputDir = CONFIG_READ_STRING(PACKET_APP_CONFIG_SECTION, PACKET_APP_OUTPUT_PATH_KEY, QStringLiteral("out/packet_app/"));
    m_partitionChoices = {
        {PacketAppCore::PT_BOOT, QStringLiteral("BOOT")},
        {PacketAppCore::PT_TAG, QStringLiteral("TAG")},
        {PacketAppCore::PT_KERNEL, QStringLiteral("KERNEL")},
        {PacketAppCore::PT_ROOTFS, QStringLiteral("ROOTFS")},
        {PacketAppCore::PT_RECOVERY, QStringLiteral("RECOVERY")},
        {PacketAppCore::PT_SYSTEM, QStringLiteral("SYSTEM")},
        {PacketAppCore::PT_CONFIG, QStringLiteral("CONFIG")},
        {PacketAppCore::PT_MODEL, QStringLiteral("MODEL")},
        {PacketAppCore::PT_DATA, QStringLiteral("DATA")}
    };
    setupUi();
}

void PacketAppWidget::setupUi()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    QGroupBox *basicGroup = new QGroupBox(QStringLiteral("打包信息"), this);
    basicGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout *basicLayout = new QGridLayout(basicGroup);
    basicLayout->setContentsMargins(12, 12, 12, 12);
    basicLayout->setHorizontalSpacing(8);
    basicLayout->setVerticalSpacing(8);

    m_projectNameEdit = new QLineEdit(basicGroup);
    m_projectNameEdit->setPlaceholderText(QStringLiteral("项目名称"));
    m_customerNameEdit = new QLineEdit(basicGroup);
    m_customerNameEdit->setPlaceholderText(QStringLiteral("客户名称"));

    m_versionMajor = new QSpinBox(basicGroup);
    m_versionMinor = new QSpinBox(basicGroup);
    m_versionPatch = new QSpinBox(basicGroup);
    for (QSpinBox *spinBox : {m_versionMajor, m_versionMinor, m_versionPatch}) {
        spinBox->setRange(0, 255);
        spinBox->setFixedWidth(72);
    }
    m_versionMajor->setValue(0);
    m_versionMinor->setValue(0);
    m_versionPatch->setValue(0);

    QHBoxLayout *versionLayout = new QHBoxLayout();
    versionLayout->setContentsMargins(0, 0, 0, 0);
    versionLayout->setSpacing(4);
    versionLayout->addWidget(m_versionMajor);
    versionLayout->addWidget(new QLabel(QStringLiteral("."), basicGroup));
    versionLayout->addWidget(m_versionMinor);
    versionLayout->addWidget(new QLabel(QStringLiteral("."), basicGroup));
    versionLayout->addWidget(m_versionPatch);
    versionLayout->addStretch();

    m_alignCombo = new QComboBox(basicGroup);
    m_alignCombo->addItem(QStringLiteral("不对齐"), 0);
    m_alignCombo->addItem(QStringLiteral("8 字节对齐"), 8);
    m_alignCombo->addItem(QStringLiteral("16 字节对齐"), 16);
    m_alignCombo->addItem(QStringLiteral("32 字节对齐"), 32);
    m_alignCombo->setFixedWidth(130);

    m_packButton = new QPushButton(QStringLiteral("开始打包"), basicGroup);
    m_packButton->setMinimumWidth(100);

    basicLayout->addWidget(new QLabel(QStringLiteral("项目名称"), basicGroup), 0, 0);
    basicLayout->addWidget(m_projectNameEdit, 0, 1);
    basicLayout->addWidget(new QLabel(QStringLiteral("客户名称"), basicGroup), 0, 2);
    basicLayout->addWidget(m_customerNameEdit, 0, 3);
    basicLayout->addWidget(new QLabel(QStringLiteral("固件版本"), basicGroup), 1, 0);
    basicLayout->addLayout(versionLayout, 1, 1);
    basicLayout->addWidget(new QLabel(QStringLiteral("输出对齐"), basicGroup), 1, 2);
    basicLayout->addWidget(m_alignCombo, 1, 3);
    basicLayout->addWidget(m_packButton, 1, 4);
    basicLayout->setColumnStretch(1, 1);
    basicLayout->setColumnStretch(3, 1);
    basicLayout->setColumnStretch(4, 0);

    QGroupBox *partitionGroup = new QGroupBox(QStringLiteral("分区固件"), this);
    partitionGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *partitionLayout = new QVBoxLayout(partitionGroup);
    partitionLayout->setContentsMargins(12, 12, 12, 12);

    QTableWidget *table = new QTableWidget(PARTITION_ROW_COUNT, 5, partitionGroup);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->setHorizontalHeaderLabels({
        QStringLiteral("固件文件"),
        QStringLiteral("操作"),
        QStringLiteral("分区"),
        QStringLiteral("选择"),
        QStringLiteral("大小")
    });
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->setMinimumHeight(0);

    m_partitionRows.clear();
    for (int row = 0; row < PARTITION_ROW_COUNT; ++row) {
        QLineEdit *pathEdit = new QLineEdit(table);
        pathEdit->setReadOnly(true);
        pathEdit->setPlaceholderText(QStringLiteral("请选择固件文件"));
        table->setCellWidget(row, 0, pathEdit);

        QPushButton *browseButton = new QPushButton(QStringLiteral("..."), table);
        browseButton->setFixedWidth(30);
        table->setCellWidget(row, 1, browseButton);

        QComboBox *partitionCombo = new QComboBox(table);
        fillPartitionCombo(partitionCombo);
        if (!m_partitionChoices.isEmpty()) {
            partitionCombo->setCurrentIndex(row % m_partitionChoices.size());
        }
        table->setCellWidget(row, 2, partitionCombo);

        QCheckBox *checkBox = new QCheckBox(table);
        QWidget *checkBoxWrapper = new QWidget(table);
        QHBoxLayout *checkBoxLayout = new QHBoxLayout(checkBoxWrapper);
        checkBoxLayout->setContentsMargins(0, 0, 0, 0);
        checkBoxLayout->addWidget(checkBox, 0, Qt::AlignCenter);
        table->setCellWidget(row, 3, checkBoxWrapper);

        QLabel *sizeLabel = new QLabel(QStringLiteral("-"), table);
        sizeLabel->setMinimumWidth(78);
        table->setCellWidget(row, 4, sizeLabel);

        m_partitionRows.push_back({
            checkBox,
            pathEdit,
            browseButton,
            partitionCombo,
            sizeLabel
        });

        connect(browseButton, &QPushButton::clicked, this, [this, row]() {
            browsePartitionFile(row);
        });
    }

    partitionLayout->addWidget(table, 1);

    rootLayout->addWidget(partitionGroup, 1);
    rootLayout->addWidget(basicGroup);
    rootLayout->setStretchFactor(partitionGroup, 1);
    rootLayout->setStretchFactor(basicGroup, 0);

    connect(m_packButton, &QPushButton::clicked, this, &PacketAppWidget::packOtaPackage);
}

void PacketAppWidget::browsePartitionFile(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= m_partitionRows.size()) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择固件文件"),
        QString(),
        QStringLiteral("固件文件 (*.bin *.img *.jzlzma *.cpio);;所有文件 (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    m_partitionRows[rowIndex].pathEdit->setText(filePath);
    m_partitionRows[rowIndex].checkBox->setChecked(true);
    updatePartitionSize(rowIndex);
}

void PacketAppWidget::packOtaPackage()
{
    PacketAppCore::PackageOptions options;
    options.projectName = m_projectNameEdit->text();
    options.customerName = m_customerNameEdit->text();
    options.versionMajor = m_versionMajor->value();
    options.versionMinor = m_versionMinor->value();
    options.versionPatch = m_versionPatch->value();
    options.alignment = selectedAlignment();
    options.outputDir = m_outputDir;

    int order = 1;
    for (const PartitionRow &row : std::as_const(m_partitionRows)) {
        if (!row.checkBox->isChecked()) {
            continue;
        }

        const QString path = row.pathEdit->text().trimmed();
        if (path.isEmpty()) {
            BSD_LOG(LOG_WARN, QStringLiteral("OTA 打包失败：已勾选的分区存在未选择固件文件\n"));
            return;
        }

        options.partitions.push_back({
            static_cast<quint32>(row.partitionCombo->currentData().toInt()),
            row.partitionCombo->currentText(),
            path,
            order++
        });
    }

    setControlsEnabled(false);
    const PacketAppCore::PackageResult result = PacketAppCore::pack(options);
    setControlsEnabled(true);

    if (!result.success) {
        BSD_LOG(LOG_ERRO, QStringLiteral("OTA 打包失败：%1\n").arg(result.errorMessage));
        return;
    }

    for (const QString &message : result.messages) {
        BSD_LOG(LOG_INFO, message + QStringLiteral("\n"));
    }
}

void PacketAppWidget::updatePartitionSize(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= m_partitionRows.size()) {
        return;
    }

    const QFileInfo fileInfo(m_partitionRows[rowIndex].pathEdit->text());
    m_partitionRows[rowIndex].sizeLabel->setText(fileInfo.exists() ? formatFileSize(fileInfo.size()) : QStringLiteral("-"));
}

void PacketAppWidget::fillPartitionCombo(QComboBox *comboBox) const
{
    for (const PartitionChoice &choice : m_partitionChoices) {
        comboBox->addItem(choice.name, choice.type);
    }
}

QString PacketAppWidget::formatFileSize(qint64 size) const
{
    if (size < 1024) {
        return QStringLiteral("%1 B").arg(size);
    }
    if (size < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(QString::number(size / 1024.0, 'f', 2));
    }
    return QStringLiteral("%1 MB").arg(QString::number(size / 1024.0 / 1024.0, 'f', 2));
}

int PacketAppWidget::selectedAlignment() const
{
    return m_alignCombo ? m_alignCombo->currentData().toInt() : 0;
}

void PacketAppWidget::setControlsEnabled(bool enabled)
{
    if (m_projectNameEdit) {
        m_projectNameEdit->setEnabled(enabled);
    }
    if (m_customerNameEdit) {
        m_customerNameEdit->setEnabled(enabled);
    }
    if (m_versionMajor) {
        m_versionMajor->setEnabled(enabled);
    }
    if (m_versionMinor) {
        m_versionMinor->setEnabled(enabled);
    }
    if (m_versionPatch) {
        m_versionPatch->setEnabled(enabled);
    }
    if (m_alignCombo) {
        m_alignCombo->setEnabled(enabled);
    }
    if (m_packButton) {
        m_packButton->setEnabled(enabled);
    }

    for (const PartitionRow &row : std::as_const(m_partitionRows)) {
        row.checkBox->setEnabled(enabled);
        row.pathEdit->setEnabled(enabled);
        row.browseButton->setEnabled(enabled);
        row.partitionCombo->setEnabled(enabled);
    }
}
