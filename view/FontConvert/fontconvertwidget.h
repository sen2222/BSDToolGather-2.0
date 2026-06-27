#ifndef FONTCONVERTWIDGET_H
#define FONTCONVERTWIDGET_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

class FontConvertWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FontConvertWidget(QWidget *parent = nullptr);

private:
    void setupUi();
    void browseFontFile();
    void convertFont();
    QString outputFilePath() const;
    void setControlsEnabled(bool enabled);

    QLineEdit *m_fontPathEdit;
    QPushButton *m_fontBrowseButton;
    QLineEdit *m_outputNameEdit;
    QComboBox *m_formatCombo;
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QSpinBox *m_fontSizeSpin;
    QLineEdit *m_suffixEdit;
    QLineEdit *m_charsEdit;
    QPushButton *m_convertButton;
    QString m_outputDir;
};

#endif // FONTCONVERTWIDGET_H
