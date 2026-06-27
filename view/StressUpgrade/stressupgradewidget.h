#ifndef STRESSUPGRADEWIDGET_H
#define STRESSUPGRADEWIDGET_H

#include "SerialPort/serialportquery.h"
#include "stressupgradeworker.h"
#include "usbvidpidrules.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTimer;

class StressUpgradeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StressUpgradeWidget(QWidget *parent = nullptr);
    ~StressUpgradeWidget() override;

private:
    void setupUi();
    void refreshPorts(bool checkCurrent = false);
    void updateSelectedPortStatus();
    bool checkSelectedPort(bool writeLog);
    void browseFirmware(QLineEdit *targetEdit);
    void startOrStop();
    void startUpgrade();
    void stopUpgrade();
    void setControlsEnabled(bool enabled);
    SerialPort::Config currentSerialConfig(const QString &portName) const;
    QString ruleFilePath() const;

    QComboBox *m_portCombo;
    QPushButton *m_detectButton;
    QLabel *m_portStatusLabel;
    QComboBox *m_baudCombo;
    QComboBox *m_dataBitsCombo;
    QComboBox *m_parityCombo;
    QComboBox *m_stopBitsCombo;
    QLineEdit *m_firmwareAEdit;
    QLineEdit *m_firmwareBEdit;
    QPushButton *m_firmwareAButton;
    QPushButton *m_firmwareBButton;
    QSpinBox *m_countSpin;
    QPushButton *m_startButton;
    QProgressBar *m_progressBar;
    QLabel *m_stateLabel;
    QLabel *m_statsLabel;
    QTimer *m_portRefreshTimer;

    QList<SerialPort> m_ports;
    QString m_portSnapshot;
    UsbVidPidRules m_rules;
    StressUpgradeWorker *m_worker;
    bool m_hasCheckedPort;
};

#endif // STRESSUPGRADEWIDGET_H
