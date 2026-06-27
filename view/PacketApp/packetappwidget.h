#ifndef PACKETAPPWIDGET_H
#define PACKETAPPWIDGET_H

#include <QVector>
#include <QWidget>



#define PACKET_APP_CONFIG_SECTION      "PacketApp"
#define PACKET_APP_OUTPUT_PATH_KEY     "outputPath"
#define PARTITION_ROW_COUNT            9

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class PacketAppWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PacketAppWidget(QWidget *parent = nullptr);

private:
    struct PartitionChoice
    {
        int type;
        QString name;
    };

    struct PartitionRow
    {
        QCheckBox *checkBox;
        QLineEdit *pathEdit;
        QPushButton *browseButton;
        QComboBox *partitionCombo;
        QLabel *sizeLabel;
    };

    void setupUi();
    void browsePartitionFile(int rowIndex);
    void packOtaPackage();
    void updatePartitionSize(int rowIndex);
    void fillPartitionCombo(QComboBox *comboBox) const;
    QString formatFileSize(qint64 size) const;
    int selectedAlignment() const;
    void setControlsEnabled(bool enabled);

    QVector<PartitionRow> m_partitionRows;
    QVector<PartitionChoice> m_partitionChoices;
    QLineEdit *m_projectNameEdit;
    QLineEdit *m_customerNameEdit;
    QSpinBox *m_versionMajor;
    QSpinBox *m_versionMinor;
    QSpinBox *m_versionPatch;
    QComboBox *m_alignCombo;
    QPushButton *m_packButton;
    QString m_outputDir;
};

#endif // PACKETAPPWIDGET_H
