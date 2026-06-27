#include "stressupgradewidget.h"

#include "alltoolfun.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSerialPort>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

static QString formatStressDuration(qint64 durationMs)
{
    qint64 totalSeconds = (durationMs + 500) / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1.%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

StressUpgradeWidget::StressUpgradeWidget(QWidget *parent)
    : QWidget(parent)
    , m_portCombo(nullptr)
    , m_detectButton(nullptr)
    , m_portStatusLabel(nullptr)
    , m_baudCombo(nullptr)
    , m_dataBitsCombo(nullptr)
    , m_parityCombo(nullptr)
    , m_stopBitsCombo(nullptr)
    , m_firmwareAEdit(nullptr)
    , m_firmwareBEdit(nullptr)
    , m_firmwareAButton(nullptr)
    , m_firmwareBButton(nullptr)
    , m_countSpin(nullptr)
    , m_startButton(nullptr)
    , m_progressBar(nullptr)
    , m_stateLabel(nullptr)
    , m_statsLabel(nullptr)
    , m_portRefreshTimer(nullptr)
    , m_worker(nullptr)
    , m_hasCheckedPort(false)
{
    setupUi();
    QString errorMessage;
    if (!m_rules.load(ruleFilePath(), &errorMessage)) {
        BSD_LOG(LOG_WARN, errorMessage + QStringLiteral("\n"));
    }
    refreshPorts(false);
}

StressUpgradeWidget::~StressUpgradeWidget()
{
    if (m_worker) {
        m_worker->requestStop();
        m_worker->wait(3000);
    }
}

void StressUpgradeWidget::setupUi()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    QGroupBox *portGroup = new QGroupBox(QStringLiteral("串口设置"), this);
    portGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QGridLayout *portLayout = new QGridLayout(portGroup);
    portLayout->setContentsMargins(12, 12, 12, 12);
    portLayout->setHorizontalSpacing(6);
    portLayout->setVerticalSpacing(8);

    QGroupBox *configGroup = new QGroupBox(QStringLiteral("配置"), this);
    configGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);
    configLayout->setContentsMargins(12, 12, 12, 12);
    configLayout->setSpacing(8);

    m_portCombo = new QComboBox(portGroup);
    m_portCombo->setMinimumWidth(118);
    m_detectButton = new QPushButton(QStringLiteral("检测串口"), portGroup);
    m_portStatusLabel = new QLabel(QStringLiteral("未检测"), portGroup);
    m_portStatusLabel->setWordWrap(true);
    m_portStatusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_baudCombo = new QComboBox(portGroup);
    m_baudCombo->setEditable(true);
    for (int baud : {230400, 2000000, 1500000, 921600, 460800, 115200}) {
        m_baudCombo->addItem(QString::number(baud), baud);
    }

    m_dataBitsCombo = new QComboBox(portGroup);
    m_dataBitsCombo->addItem(QStringLiteral("8"), QSerialPort::Data8);
    m_dataBitsCombo->addItem(QStringLiteral("7"), QSerialPort::Data7);

    m_parityCombo = new QComboBox(portGroup);
    m_parityCombo->addItem(QStringLiteral("none"), QSerialPort::NoParity);
    m_parityCombo->addItem(QStringLiteral("even"), QSerialPort::EvenParity);
    m_parityCombo->addItem(QStringLiteral("odd"), QSerialPort::OddParity);

    m_stopBitsCombo = new QComboBox(portGroup);
    m_stopBitsCombo->addItem(QStringLiteral("1"), QSerialPort::OneStop);
    m_stopBitsCombo->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    portLayout->addWidget(new QLabel(QStringLiteral("端口"), portGroup), 0, 0);
    portLayout->addWidget(m_portCombo, 0, 1);
    portLayout->addWidget(new QLabel(QStringLiteral("波特率"), portGroup), 1, 0);
    portLayout->addWidget(m_baudCombo, 1, 1);
    portLayout->addWidget(new QLabel(QStringLiteral("数据位"), portGroup), 2, 0);
    portLayout->addWidget(m_dataBitsCombo, 2, 1);
    portLayout->addWidget(new QLabel(QStringLiteral("停止位"), portGroup), 3, 0);
    portLayout->addWidget(m_stopBitsCombo, 3, 1);
    portLayout->addWidget(new QLabel(QStringLiteral("校验位"), portGroup), 4, 0);
    portLayout->addWidget(m_parityCombo, 4, 1);
    portLayout->addWidget(m_detectButton, 5, 0, 1, 2);
    portLayout->addWidget(m_portStatusLabel, 6, 0, 1, 2);
    portLayout->setRowStretch(7, 1);

    QGroupBox *firmwareGroup = new QGroupBox(QStringLiteral("压测固件"), configGroup);
    firmwareGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout *firmwareLayout = new QGridLayout(firmwareGroup);
    firmwareLayout->setContentsMargins(12, 12, 12, 12);
    firmwareLayout->setHorizontalSpacing(8);
    firmwareLayout->setVerticalSpacing(8);

    m_firmwareAEdit = new QLineEdit(firmwareGroup);
    m_firmwareAEdit->setReadOnly(true);
    m_firmwareAEdit->setPlaceholderText(QStringLiteral("请选择第一个固件"));
    m_firmwareBEdit = new QLineEdit(firmwareGroup);
    m_firmwareBEdit->setReadOnly(true);
    m_firmwareBEdit->setPlaceholderText(QStringLiteral("请选择第二个固件"));
    m_firmwareAButton = new QPushButton(QStringLiteral("..."), firmwareGroup);
    m_firmwareBButton = new QPushButton(QStringLiteral("..."), firmwareGroup);
    m_firmwareAButton->setFixedWidth(34);
    m_firmwareBButton->setFixedWidth(34);

    m_countSpin = new QSpinBox(firmwareGroup);
    m_countSpin->setRange(1, 100000);
    m_countSpin->setValue(2);
    m_startButton = new QPushButton(QStringLiteral("开始压测"), firmwareGroup);
    m_startButton->setMinimumWidth(100);

    firmwareLayout->addWidget(new QLabel(QStringLiteral("固件A"), firmwareGroup), 0, 0);
    firmwareLayout->addWidget(m_firmwareAEdit, 0, 1);
    firmwareLayout->addWidget(m_firmwareAButton, 0, 2);
    firmwareLayout->addWidget(new QLabel(QStringLiteral("固件B"), firmwareGroup), 1, 0);
    firmwareLayout->addWidget(m_firmwareBEdit, 1, 1);
    firmwareLayout->addWidget(m_firmwareBButton, 1, 2);
    firmwareLayout->addWidget(new QLabel(QStringLiteral("升级次数"), firmwareGroup), 2, 0);
    firmwareLayout->addWidget(m_countSpin, 2, 1);
    firmwareLayout->addWidget(m_startButton, 2, 2);
    firmwareLayout->setColumnStretch(1, 1);

    configLayout->addWidget(firmwareGroup);

    QGroupBox *statusGroup = new QGroupBox(QStringLiteral("进度"), this);
    statusGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    m_stateLabel = new QLabel(QStringLiteral("-"), statusGroup);
    m_statsLabel = new QLabel(QStringLiteral("总数:0 成功:0 失败:0 总耗时:0.00 分钟 平均耗时:0.00 分钟 最近耗时:0.00 分钟"), statusGroup);
    m_statsLabel->setWordWrap(true);
    m_progressBar = new QProgressBar(statusGroup);
    m_progressBar->setRange(0, 100);
    statusLayout->addWidget(m_stateLabel);
    statusLayout->addWidget(m_statsLabel);
    statusLayout->addWidget(m_progressBar);
    statusLayout->addStretch();

    leftLayout->addWidget(configGroup);
    leftLayout->addWidget(statusGroup);
    leftLayout->addStretch();

    rootLayout->addLayout(leftLayout, 1);
    rootLayout->addWidget(portGroup);

    m_portRefreshTimer = new QTimer(this);
    m_portRefreshTimer->setInterval(1000);
    connect(m_portRefreshTimer, &QTimer::timeout, this, [this]() { refreshPorts(false); });
    m_portRefreshTimer->start();

    connect(m_detectButton, &QPushButton::clicked, this, [this]() { refreshPorts(true); });
    connect(m_portCombo, &QComboBox::currentIndexChanged, this, &StressUpgradeWidget::updateSelectedPortStatus);
    connect(m_firmwareAButton, &QPushButton::clicked, this, [this]() { browseFirmware(m_firmwareAEdit); });
    connect(m_firmwareBButton, &QPushButton::clicked, this, [this]() { browseFirmware(m_firmwareBEdit); });
    connect(m_startButton, &QPushButton::clicked, this, &StressUpgradeWidget::startOrStop);
}

void StressUpgradeWidget::refreshPorts(bool checkCurrent)
{
    const int oldIndex = m_portCombo->currentIndex();
    QString oldPortName;
    QString oldPhysicalPortKey;
    if (oldIndex >= 0 && oldIndex < m_ports.size()) {
        oldPortName = m_ports.at(oldIndex).info().portName;
        oldPhysicalPortKey = m_ports.at(oldIndex).info().physicalPortKey;
    }

    const QList<SerialPort> ports = SerialPortQuery::availablePorts();
    const QString snapshot = SerialPortQuery::snapshot(ports);
    if (!checkCurrent && snapshot == m_portSnapshot) {
        return;
    }

    m_ports = ports;
    m_portSnapshot = snapshot;
    const QSignalBlocker blocker(m_portCombo);
    m_portCombo->clear();
    for (const SerialPort &port : m_ports) {
        const SerialPort::Info &info = port.info();
        const QString name = info.friendlyName.isEmpty()
            ? info.portName
            : QStringLiteral("%1 - %2").arg(info.portName, info.friendlyName);
        m_portCombo->addItem(name);
    }

    int newIndex = -1;
    for (int i = 0; i < m_ports.size(); ++i) {
        const SerialPort::Info &info = m_ports.at(i).info();
        if (!oldPhysicalPortKey.isEmpty()
            && info.physicalPortKey.compare(oldPhysicalPortKey, Qt::CaseInsensitive) == 0) {
            newIndex = i;
            break;
        }
        if (newIndex < 0 && !oldPortName.isEmpty()
            && info.portName.compare(oldPortName, Qt::CaseInsensitive) == 0) {
            newIndex = i;
        }
    }
    if (newIndex < 0 && !m_ports.isEmpty()) {
        newIndex = qBound(0, oldIndex, m_ports.size() - 1);
    }
    if (newIndex >= 0) {
        m_portCombo->setCurrentIndex(newIndex);
    }

    if (m_ports.isEmpty()) {
        m_portStatusLabel->setText(QStringLiteral("未检测到串口"));
        return;
    }

    if (checkCurrent) {
        checkSelectedPort(true);
    } else if (m_hasCheckedPort) {
        checkSelectedPort(false);
    } else {
        m_portStatusLabel->setText(QStringLiteral("未检测"));
    }
}

void StressUpgradeWidget::updateSelectedPortStatus()
{
    checkSelectedPort(true);
}

bool StressUpgradeWidget::checkSelectedPort(bool writeLog)
{
    if (writeLog) {
        m_hasCheckedPort = true;
    }

    const int index = m_portCombo->currentIndex();
    if (index < 0 || index >= m_ports.size()) {
        m_portStatusLabel->setText(QStringLiteral("未选择串口"));
        return false;
    }

    const SerialPort::Info info = m_ports.at(index).info();
    if (m_rules.isAllowed(info)) {
        m_portStatusLabel->setText(QStringLiteral("支持：%1\nVID:%2 PID:%3")
            .arg(m_rules.describe(info), info.vid, info.pid));
        if (writeLog) {
            BSD_LOG(LOG_INFO, QStringLiteral("检测串口通过：%1，VID:%2 PID:%3，物理口:%4\n")
                .arg(m_rules.describe(info), info.vid, info.pid, info.physicalPortKey));
        }
        return true;
    }

    m_portStatusLabel->setText(QStringLiteral("不支持\nVID:%1 PID:%2").arg(info.vid, info.pid));
    if (writeLog) {
        BSD_LOG(LOG_WARN, QStringLiteral("检测串口不支持：%1，VID:%2 PID:%3\n")
            .arg(info.portName, info.vid, info.pid));
    }
    return false;
}

void StressUpgradeWidget::browseFirmware(QLineEdit *targetEdit)
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择升级固件"),
        QString(),
        QStringLiteral("固件文件 (*.bin);;所有文件 (*.*)"));
    if (!filePath.isEmpty()) {
        targetEdit->setText(filePath);
    }
}

void StressUpgradeWidget::startOrStop()
{
    if (m_worker) {
        stopUpgrade();
        return;
    }
    startUpgrade();
}

void StressUpgradeWidget::startUpgrade()
{
    const int index = m_portCombo->currentIndex();
    if (index < 0 || index >= m_ports.size()) {
        BSD_LOG(LOG_ERRO, QStringLiteral("压测升级失败：请选择 COM 口\n"));
        return;
    }
    if (!checkSelectedPort(true)) {
        BSD_LOG(LOG_ERRO, QStringLiteral("压测升级失败：当前 COM 口 VID/PID 不在规则内\n"));
        return;
    }
    if (m_firmwareAEdit->text().isEmpty() || m_firmwareBEdit->text().isEmpty()) {
        BSD_LOG(LOG_ERRO, QStringLiteral("压测升级失败：请选择两个固件\n"));
        return;
    }
    bool baudOk = false;
    const int baudRate = m_baudCombo->currentText().trimmed().toInt(&baudOk);
    if (!baudOk || baudRate <= 0) {
        BSD_LOG(LOG_ERRO, QStringLiteral("压测升级失败：请输入有效波特率\n"));
        return;
    }

    StressUpgradeWorker::Options options;
    options.selectedPortInfo = m_ports.at(index).info();
    options.serialConfig = currentSerialConfig(options.selectedPortInfo.portName);
    options.firmwareAPath = m_firmwareAEdit->text();
    options.firmwareBPath = m_firmwareBEdit->text();
    options.upgradeCount = m_countSpin->value();
    options.ruleFilePath = ruleFilePath();

    m_worker = new StressUpgradeWorker(options, this);
    qRegisterMetaType<StressUpgradeWorker::State>("StressUpgradeWorker::State");
    qRegisterMetaType<StressUpgradeWorker::Statistics>("StressUpgradeWorker::Statistics");

    connect(m_worker, &StressUpgradeWorker::stateChanged, this, [this](StressUpgradeWorker::State, const QString &text) {
        m_stateLabel->setText(text);
        BSD_LOG(LOG_INFO, QStringLiteral("压测升级：%1\n").arg(text));
    });
    connect(m_worker, &StressUpgradeWorker::progressChanged, this, [this](int cycleIndex, int progress) {
        m_progressBar->setValue(progress);
        m_stateLabel->setText(QStringLiteral("第 %1 次升级，进度 %2%").arg(cycleIndex).arg(progress));
    });
    connect(m_worker, &StressUpgradeWorker::statisticsChanged, this, [this](const StressUpgradeWorker::Statistics &statistics) {
        const qint64 average = statistics.finished > 0 ? statistics.totalDurationMs / statistics.finished : 0;
        m_statsLabel->setText(QStringLiteral("总数:%1 成功:%2 失败:%3 总耗时:%4 分钟 平均耗时:%5 分钟 最近耗时:%6 分钟")
            .arg(statistics.total)
            .arg(statistics.success)
            .arg(statistics.failed)
            .arg(formatStressDuration(statistics.totalDurationMs))
            .arg(formatStressDuration(average))
            .arg(formatStressDuration(statistics.lastDurationMs)));
    });
    connect(m_worker, &StressUpgradeWorker::logMessage, this, [](const QString &message) {
        BSD_LOG(LOG_INFO, message + QStringLiteral("\n"));
    });
    connect(m_worker, &QThread::finished, this, [this]() {
        m_worker->deleteLater();
        m_worker = nullptr;
        m_startButton->setText(QStringLiteral("开始压测"));
        setControlsEnabled(true);
        if (m_portRefreshTimer && !m_portRefreshTimer->isActive()) {
            m_portRefreshTimer->start();
        }
        refreshPorts(false);
    });

    setControlsEnabled(false);
    if (m_portRefreshTimer) {
        m_portRefreshTimer->stop();
    }
    m_startButton->setEnabled(true);
    m_startButton->setText(QStringLiteral("停止压测"));
    m_stateLabel->setText(QStringLiteral("-"));
    m_statsLabel->setText(QStringLiteral("总数:%1 成功:0 失败:0 总耗时:0.00 分钟 平均耗时:0.00 分钟 最近耗时:0.00 分钟")
        .arg(m_countSpin->value()));
    m_progressBar->setValue(0);
    m_worker->start();
}

void StressUpgradeWidget::stopUpgrade()
{
    if (m_worker) {
        BSD_LOG(LOG_INFO, QStringLiteral("正在停止压测升级\n"));
        m_worker->requestStop();
        m_startButton->setEnabled(false);
        m_startButton->setText(QStringLiteral("正在停止"));
    }
}

void StressUpgradeWidget::setControlsEnabled(bool enabled)
{
    m_portCombo->setEnabled(enabled);
    m_detectButton->setEnabled(enabled);
    m_baudCombo->setEnabled(enabled);
    m_dataBitsCombo->setEnabled(enabled);
    m_parityCombo->setEnabled(enabled);
    m_stopBitsCombo->setEnabled(enabled);
    m_firmwareAEdit->setEnabled(enabled);
    m_firmwareBEdit->setEnabled(enabled);
    m_firmwareAButton->setEnabled(enabled);
    m_firmwareBButton->setEnabled(enabled);
    m_countSpin->setEnabled(enabled);
    m_startButton->setEnabled(true);
}

SerialPort::Config StressUpgradeWidget::currentSerialConfig(const QString &portName) const
{
    SerialPort::Config config;
    config.portName = portName;
    config.baudRate = m_baudCombo->currentText().toInt();
    if (config.baudRate <= 0) {
        config.baudRate = m_baudCombo->currentData().toInt();
    }
    config.dataBits = static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentData().toInt());
    config.parity = static_cast<QSerialPort::Parity>(m_parityCombo->currentData().toInt());
    config.stopBits = static_cast<QSerialPort::StopBits>(m_stopBitsCombo->currentData().toInt());
    config.flowControl = QSerialPort::NoFlowControl;
    return config;
}

QString StressUpgradeWidget::ruleFilePath() const
{
    const QString currentPath = QDir::currentPath() + QStringLiteral("/config/UpgradeUsbRules.json");
    if (QFileInfo::exists(currentPath)) {
        return currentPath;
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config/UpgradeUsbRules.json"));
}
