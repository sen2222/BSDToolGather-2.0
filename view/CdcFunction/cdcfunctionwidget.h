#ifndef CDCFUNCTIONWIDGET_H
#define CDCFUNCTIONWIDGET_H

#include "SerialPort/serialportquery.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

class CdcFunctionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CdcFunctionWidget(QWidget *parent = nullptr);

private:
    void setupUi();
    void refreshPorts();
    void openSerialPort();
    void closeSerialPort();
    void fillLocalTimestamp();
    void sendTimeOsdCommand();
    SerialPort::Config currentSerialConfig() const;
    QString bytesToHex(const QByteArray &data) const;
    bool readUInt32(const QLineEdit *edit, quint32 *value, const QString &fieldName) const;
    void setControlsEnabled(bool enabled);
    void updateSerialControls();

    QComboBox *m_portCombo;
    QPushButton *m_refreshButton;
    QPushButton *m_openButton;
    QPushButton *m_closeButton;
    QComboBox *m_baudCombo;
    QComboBox *m_dataBitsCombo;
    QComboBox *m_stopBitsCombo;
    QComboBox *m_parityCombo;
    QLineEdit *m_timeStampEdit;
    QPushButton *m_localTimestampButton;
    QCheckBox *m_mjpegShowCheck;
    QSpinBox *m_mjpegXSpin;
    QSpinBox *m_mjpegYSpin;
    QCheckBox *m_h264ShowCheck;
    QSpinBox *m_h264XSpin;
    QSpinBox *m_h264YSpin;
    QPushButton *m_sendButton;
    QList<SerialPort> m_ports;
    SerialPort m_serialPort;
};

#endif // CDCFUNCTIONWIDGET_H
