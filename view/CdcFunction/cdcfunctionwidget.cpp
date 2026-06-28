#include "cdcfunctionwidget.h"

#include "alltoolfun.h"
#include "cdccommandcore.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSerialPort>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>

CdcFunctionWidget::CdcFunctionWidget(QWidget *parent)
    : QWidget(parent)
    , m_portCombo(nullptr)
    , m_refreshButton(nullptr)
    , m_openButton(nullptr)
    , m_closeButton(nullptr)
    , m_baudCombo(nullptr)
    , m_dataBitsCombo(nullptr)
    , m_stopBitsCombo(nullptr)
    , m_parityCombo(nullptr)
    , m_timeStampEdit(nullptr)
    , m_localTimestampButton(nullptr)
    , m_mjpegShowCheck(nullptr)
    , m_mjpegXSpin(nullptr)
    , m_mjpegYSpin(nullptr)
    , m_h264ShowCheck(nullptr)
    , m_h264XSpin(nullptr)
    , m_h264YSpin(nullptr)
    , m_sendButton(nullptr)
{
    setupUi();
    refreshPorts();
    updateSerialControls();
}

void CdcFunctionWidget::setupUi()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(12);

    QWidget *functionArea = new QWidget(this);
    QVBoxLayout *functionLayout = new QVBoxLayout(functionArea);
    functionLayout->setContentsMargins(0, 0, 0, 0);
    functionLayout->setSpacing(10);

    QGroupBox *timeGroup = new QGroupBox(QStringLiteral("修改时间显示设置(ABH)"), functionArea);
    QGridLayout *timeLayout = new QGridLayout(timeGroup);
    timeLayout->setContentsMargins(12, 12, 12, 12);
    timeLayout->setHorizontalSpacing(8);
    timeLayout->setVerticalSpacing(8);

    m_timeStampEdit = new QLineEdit(timeGroup);
    m_timeStampEdit->setPlaceholderText(QStringLiteral("0 表示不修改时间"));
    m_timeStampEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("\\d{1,10}")), m_timeStampEdit));
    m_localTimestampButton = new QPushButton(QStringLiteral("本地时间戳"), timeGroup);

    m_mjpegShowCheck = new QCheckBox(QStringLiteral("显示"), timeGroup);
    m_mjpegXSpin = new QSpinBox(timeGroup);
    m_mjpegYSpin = new QSpinBox(timeGroup);
    m_h264ShowCheck = new QCheckBox(QStringLiteral("显示"), timeGroup);
    m_h264XSpin = new QSpinBox(timeGroup);
    m_h264YSpin = new QSpinBox(timeGroup);
    for (QSpinBox *spinBox : {m_mjpegXSpin, m_mjpegYSpin, m_h264XSpin, m_h264YSpin}) {
        spinBox->setRange(0, 65535);
        spinBox->setFixedWidth(110);
    }

    m_sendButton = new QPushButton(QStringLiteral("发送"), timeGroup);
    m_sendButton->setMinimumWidth(120);

    timeLayout->addWidget(new QLabel(QStringLiteral("时间戳"), timeGroup), 0, 0);
    timeLayout->addWidget(m_timeStampEdit, 0, 1, 1, 3);
    timeLayout->addWidget(m_localTimestampButton, 0, 4, 1, 2);
    timeLayout->addWidget(new QLabel(QStringLiteral("mjpeg"), timeGroup), 1, 0);
    timeLayout->addWidget(m_mjpegShowCheck, 1, 1);
    timeLayout->addWidget(new QLabel(QStringLiteral("X"), timeGroup), 1, 2);
    timeLayout->addWidget(m_mjpegXSpin, 1, 3);
    timeLayout->addWidget(new QLabel(QStringLiteral("Y"), timeGroup), 1, 4);
    timeLayout->addWidget(m_mjpegYSpin, 1, 5);
    timeLayout->addWidget(new QLabel(QStringLiteral("H265"), timeGroup), 2, 0);
    timeLayout->addWidget(m_h264ShowCheck, 2, 1);
    timeLayout->addWidget(new QLabel(QStringLiteral("X"), timeGroup), 2, 2);
    timeLayout->addWidget(m_h264XSpin, 2, 3);
    timeLayout->addWidget(new QLabel(QStringLiteral("Y"), timeGroup), 2, 4);
    timeLayout->addWidget(m_h264YSpin, 2, 5);
    timeLayout->addWidget(m_sendButton, 3, 0, 1, 6, Qt::AlignRight);
    timeLayout->setColumnStretch(1, 1);

    functionLayout->addWidget(timeGroup);
    functionLayout->addStretch();

    QGroupBox *serialGroup = new QGroupBox(QStringLiteral("串口设置"), this);
    serialGroup->setFixedWidth(260);
    QGridLayout *serialLayout = new QGridLayout(serialGroup);
    serialLayout->setContentsMargins(12, 12, 12, 12);
    serialLayout->setHorizontalSpacing(8);
    serialLayout->setVerticalSpacing(8);

    m_portCombo = new QComboBox(serialGroup);
    m_baudCombo = new QComboBox(serialGroup);
    m_baudCombo->setEditable(true);
    for (int baud : {115200, 230400, 460800, 921600, 1500000, 2000000}) {
        m_baudCombo->addItem(QString::number(baud), baud);
    }
    m_baudCombo->setCurrentText(QStringLiteral("230400"));

    m_dataBitsCombo = new QComboBox(serialGroup);
    m_dataBitsCombo->addItem(QStringLiteral("5"), static_cast<int>(QSerialPort::Data5));
    m_dataBitsCombo->addItem(QStringLiteral("6"), static_cast<int>(QSerialPort::Data6));
    m_dataBitsCombo->addItem(QStringLiteral("7"), static_cast<int>(QSerialPort::Data7));
    m_dataBitsCombo->addItem(QStringLiteral("8"), static_cast<int>(QSerialPort::Data8));
    m_dataBitsCombo->setCurrentIndex(3);

    m_stopBitsCombo = new QComboBox(serialGroup);
    m_stopBitsCombo->addItem(QStringLiteral("1"), static_cast<int>(QSerialPort::OneStop));
    m_stopBitsCombo->addItem(QStringLiteral("1.5"), static_cast<int>(QSerialPort::OneAndHalfStop));
    m_stopBitsCombo->addItem(QStringLiteral("2"), static_cast<int>(QSerialPort::TwoStop));

    m_parityCombo = new QComboBox(serialGroup);
    m_parityCombo->addItem(QStringLiteral("none"), static_cast<int>(QSerialPort::NoParity));
    m_parityCombo->addItem(QStringLiteral("even"), static_cast<int>(QSerialPort::EvenParity));
    m_parityCombo->addItem(QStringLiteral("odd"), static_cast<int>(QSerialPort::OddParity));
    m_parityCombo->addItem(QStringLiteral("space"), static_cast<int>(QSerialPort::SpaceParity));
    m_parityCombo->addItem(QStringLiteral("mark"), static_cast<int>(QSerialPort::MarkParity));

    m_refreshButton = new QPushButton(QStringLiteral("刷新"), serialGroup);
    m_openButton = new QPushButton(QStringLiteral("Open"), serialGroup);
    m_closeButton = new QPushButton(QStringLiteral("Close"), serialGroup);

    QWidget *buttonRow = new QWidget(serialGroup);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(6);
    buttonLayout->addWidget(m_openButton);
    buttonLayout->addWidget(m_closeButton);

    serialLayout->addWidget(new QLabel(QStringLiteral("端口"), serialGroup), 0, 0);
    serialLayout->addWidget(m_portCombo, 0, 1);
    serialLayout->addWidget(new QLabel(QStringLiteral("波特率"), serialGroup), 1, 0);
    serialLayout->addWidget(m_baudCombo, 1, 1);
    serialLayout->addWidget(new QLabel(QStringLiteral("数据位"), serialGroup), 2, 0);
    serialLayout->addWidget(m_dataBitsCombo, 2, 1);
    serialLayout->addWidget(new QLabel(QStringLiteral("停止位"), serialGroup), 3, 0);
    serialLayout->addWidget(m_stopBitsCombo, 3, 1);
    serialLayout->addWidget(new QLabel(QStringLiteral("校验位"), serialGroup), 4, 0);
    serialLayout->addWidget(m_parityCombo, 4, 1);
    serialLayout->addWidget(m_refreshButton, 5, 0, 1, 2);
    serialLayout->addWidget(buttonRow, 6, 0, 1, 2);
    serialLayout->setColumnStretch(1, 1);

    rootLayout->addWidget(functionArea, 1);
    rootLayout->addWidget(serialGroup, 0, Qt::AlignTop);

    connect(m_refreshButton, &QPushButton::clicked, this, &CdcFunctionWidget::refreshPorts);
    connect(m_openButton, &QPushButton::clicked, this, &CdcFunctionWidget::openSerialPort);
    connect(m_closeButton, &QPushButton::clicked, this, &CdcFunctionWidget::closeSerialPort);
    connect(m_localTimestampButton, &QPushButton::clicked, this, &CdcFunctionWidget::fillLocalTimestamp);
    connect(m_sendButton, &QPushButton::clicked, this, &CdcFunctionWidget::sendTimeOsdCommand);
    connect(m_portCombo, &QComboBox::currentIndexChanged, this, [this]() {
        updateSerialControls();
    });
}

void CdcFunctionWidget::refreshPorts()
{
    if (m_serialPort.isOpen()) {
        BSD_LOG(LOG_WARN, QStringLiteral("CDC串口已打开，请关闭后再刷新\n"));
        return;
    }

    const QString oldPort = m_portCombo->currentData().toString();
    m_ports = SerialPortQuery::availablePorts();

    const QSignalBlocker blocker(m_portCombo);
    m_portCombo->clear();
    int selectIndex = -1;
    for (int i = 0; i < m_ports.size(); ++i) {
        const SerialPort::Info &info = m_ports.at(i).info();
        const QString name = info.friendlyName.isEmpty()
            ? info.portName
            : QStringLiteral("%1 - %2").arg(info.portName, info.friendlyName);
        m_portCombo->addItem(name, info.portName);
        if (info.portName.compare(oldPort, Qt::CaseInsensitive) == 0) {
            selectIndex = i;
        }
    }
    if (selectIndex >= 0) {
        m_portCombo->setCurrentIndex(selectIndex);
    }

    BSD_LOG(LOG_INFO, QStringLiteral("CDC串口刷新完成，数量：%1\n").arg(m_ports.size()));
    updateSerialControls();
}

void CdcFunctionWidget::openSerialPort()
{
    if (m_serialPort.isOpen()) {
        return;
    }

    if (m_portCombo->currentIndex() < 0) {
        BSD_LOG(LOG_ERRO, QStringLiteral("CDC串口打开失败：请选择串口\n"));
        updateSerialControls();
        return;
    }

    const SerialPort::Config config = currentSerialConfig();
    if (!m_serialPort.open(config)) {
        BSD_LOG(LOG_ERRO, QStringLiteral("CDC串口打开失败：%1\n").arg(m_serialPort.errorString()));
        updateSerialControls();
        return;
    }

    BSD_LOG(LOG_INFO, QStringLiteral("CDC串口已打开：%1，波特率：%2\n").arg(config.portName).arg(config.baudRate));
    updateSerialControls();
}

void CdcFunctionWidget::closeSerialPort()
{
    if (!m_serialPort.isOpen()) {
        updateSerialControls();
        return;
    }

    const QString portName = m_serialPort.portName();
    m_serialPort.close();
    BSD_LOG(LOG_INFO, QStringLiteral("CDC串口已关闭：%1\n").arg(portName));
    updateSerialControls();
}

void CdcFunctionWidget::fillLocalTimestamp()
{
    m_timeStampEdit->setText(QString::number(QDateTime::currentSecsSinceEpoch()));
}

void CdcFunctionWidget::sendTimeOsdCommand()
{
    if (!m_serialPort.isOpen()) {
        BSD_LOG(LOG_ERRO, QStringLiteral("CDC指令发送失败：请先打开串口\n"));
        updateSerialControls();
        return;
    }

    quint32 timeStamp = 0;
    if (!readUInt32(m_timeStampEdit, &timeStamp, QStringLiteral("时间戳"))) {
        return;
    }

    CdcCommandCore::TimeOsdOptions options;
    options.timeStamp = timeStamp;
    options.mjpegShowFlag = m_mjpegShowCheck->isChecked() ? 1 : 0;
    options.mjpegShowX = static_cast<quint16>(m_mjpegXSpin->value());
    options.mjpegShowY = static_cast<quint16>(m_mjpegYSpin->value());
    options.h264ShowFlag = m_h264ShowCheck->isChecked() ? 1 : 0;
    options.h264ShowX = static_cast<quint16>(m_h264XSpin->value());
    options.h264ShowY = static_cast<quint16>(m_h264YSpin->value());

    setControlsEnabled(false);
    const CdcCommandCore::CommandResult result = CdcCommandCore::sendTimeOsdCommand(&m_serialPort, options);
    setControlsEnabled(true);

    if (!result.success) {
        BSD_LOG(LOG_ERRO, QStringLiteral("CDC修改时间显示设置失败：%1\n").arg(result.errorMessage));
        BSD_LOG(LOG_INFO, QStringLiteral("CDC发送数据：%1\n").arg(bytesToHex(result.request)));
        return;
    }

    BSD_LOG(LOG_INFO, QStringLiteral("CDC修改时间显示设置已发送：%1\n").arg(bytesToHex(result.request)));
    if (result.response.isEmpty()) {
        BSD_LOG(LOG_WARN, QStringLiteral("CDC修改时间显示设置未收到返回数据\n"));
    } else {
        BSD_LOG(LOG_INFO, QStringLiteral("CDC返回数据：%1\n").arg(bytesToHex(result.response)));
    }
}

SerialPort::Config CdcFunctionWidget::currentSerialConfig() const
{
    SerialPort::Config config;
    config.portName = m_portCombo->currentData().toString();
    config.baudRate = m_baudCombo->currentText().toInt();
    if (config.baudRate <= 0) {
        config.baudRate = 115200;
    }
    config.dataBits = static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentData().toInt());
    config.parity = static_cast<QSerialPort::Parity>(m_parityCombo->currentData().toInt());
    config.stopBits = static_cast<QSerialPort::StopBits>(m_stopBitsCombo->currentData().toInt());
    config.flowControl = QSerialPort::NoFlowControl;
    return config;
}

QString CdcFunctionWidget::bytesToHex(const QByteArray &data) const
{
    return QString::fromLatin1(data.toHex(' ').toUpper());
}

bool CdcFunctionWidget::readUInt32(const QLineEdit *edit, quint32 *value, const QString &fieldName) const
{
    const QString text = edit->text().trimmed();
    bool ok = false;
    const quint64 parsed = text.isEmpty() ? 0 : text.toULongLong(&ok, 10);
    if (!text.isEmpty() && (!ok || parsed > 0xFFFFFFFFULL)) {
        BSD_LOG(LOG_ERRO, QStringLiteral("CDC指令参数错误：%1 需要在 0~4294967295 之间\n").arg(fieldName));
        return false;
    }

    *value = static_cast<quint32>(parsed);
    return true;
}

void CdcFunctionWidget::setControlsEnabled(bool enabled)
{
    m_timeStampEdit->setEnabled(enabled);
    m_localTimestampButton->setEnabled(enabled);
    m_mjpegShowCheck->setEnabled(enabled);
    m_mjpegXSpin->setEnabled(enabled);
    m_mjpegYSpin->setEnabled(enabled);
    m_h264ShowCheck->setEnabled(enabled);
    m_h264XSpin->setEnabled(enabled);
    m_h264YSpin->setEnabled(enabled);

    if (!enabled) {
        m_portCombo->setEnabled(false);
        m_refreshButton->setEnabled(false);
        m_openButton->setEnabled(false);
        m_closeButton->setEnabled(false);
        m_baudCombo->setEnabled(false);
        m_dataBitsCombo->setEnabled(false);
        m_stopBitsCombo->setEnabled(false);
        m_parityCombo->setEnabled(false);
        m_sendButton->setEnabled(false);
        return;
    }

    updateSerialControls();
}

void CdcFunctionWidget::updateSerialControls()
{
    const bool opened = m_serialPort.isOpen();
    const bool hasPort = m_portCombo->currentIndex() >= 0;

    m_portCombo->setEnabled(!opened);
    m_refreshButton->setEnabled(!opened);
    m_openButton->setEnabled(!opened && hasPort);
    m_closeButton->setEnabled(opened);
    m_baudCombo->setEnabled(!opened);
    m_dataBitsCombo->setEnabled(!opened);
    m_stopBitsCombo->setEnabled(!opened);
    m_parityCombo->setEnabled(!opened);
    m_sendButton->setEnabled(opened);
}
