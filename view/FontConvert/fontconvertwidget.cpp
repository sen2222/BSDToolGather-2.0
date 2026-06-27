#include "fontconvertwidget.h"

#include "alltoolfun.h"
#include "fontconvertcore.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>

#define FONT_CONVERT_CONFIG_SECTION      "FontConvert"
#define FONT_CONVERT_OUTPUT_PATH_KEY     "outputPath"

FontConvertWidget::FontConvertWidget(QWidget *parent)
    : QWidget(parent)
    , m_fontPathEdit(nullptr)
    , m_fontBrowseButton(nullptr)
    , m_outputNameEdit(nullptr)
    , m_formatCombo(nullptr)
    , m_widthSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_fontSizeSpin(nullptr)
    , m_suffixEdit(nullptr)
    , m_charsEdit(nullptr)
    , m_convertButton(nullptr)
{
    m_outputDir = CONFIG_READ_STRING(FONT_CONVERT_CONFIG_SECTION,
                                     FONT_CONVERT_OUTPUT_PATH_KEY,
                                     QStringLiteral("out/font_convert/"));
    setupUi();
}

void FontConvertWidget::setupUi()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    QGroupBox *fileGroup = new QGroupBox(QStringLiteral("文件"), this);
    fileGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout *fileLayout = new QGridLayout(fileGroup);
    fileLayout->setContentsMargins(12, 12, 12, 12);
    fileLayout->setHorizontalSpacing(8);
    fileLayout->setVerticalSpacing(8);

    m_fontPathEdit = new QLineEdit(fileGroup);
    m_fontPathEdit->setReadOnly(true);
    m_fontPathEdit->setPlaceholderText(QStringLiteral("请选择字体文件"));
    m_fontBrowseButton = new QPushButton(QStringLiteral("..."), fileGroup);
    m_fontBrowseButton->setFixedWidth(34);

    m_outputNameEdit = new QLineEdit(fileGroup);
    m_outputNameEdit->setPlaceholderText(QStringLiteral("请输入输出文件名"));

    fileLayout->addWidget(new QLabel(QStringLiteral("字体文件"), fileGroup), 0, 0);
    fileLayout->addWidget(m_fontPathEdit, 0, 1);
    fileLayout->addWidget(m_fontBrowseButton, 0, 2);
    fileLayout->addWidget(new QLabel(QStringLiteral("输出文件名"), fileGroup), 1, 0);
    fileLayout->addWidget(m_outputNameEdit, 1, 1, 1, 2);
    fileLayout->setColumnStretch(1, 1);

    QGroupBox *optionGroup = new QGroupBox(QStringLiteral("转换参数"), this);
    optionGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout *optionLayout = new QGridLayout(optionGroup);
    optionLayout->setContentsMargins(12, 12, 12, 12);
    optionLayout->setHorizontalSpacing(8);
    optionLayout->setVerticalSpacing(8);

    m_formatCombo = new QComboBox(optionGroup);
    m_formatCombo->addItem(QStringLiteral("RGBA1555"), FontConvertCore::RGBA1555);

    m_widthSpin = new QSpinBox(optionGroup);
    m_widthSpin->setRange(1, 4096);
    m_widthSpin->setValue(16);

    m_heightSpin = new QSpinBox(optionGroup);
    m_heightSpin->setRange(1, 4096);
    m_heightSpin->setValue(34);

    m_fontSizeSpin = new QSpinBox(optionGroup);
    m_fontSizeSpin->setRange(1, 4096);
    m_fontSizeSpin->setValue(28);

    m_suffixEdit = new QLineEdit(optionGroup);
    m_suffixEdit->setPlaceholderText(QStringLiteral("例如 _sec，可为空"));

    m_charsEdit = new QLineEdit(optionGroup);
    m_charsEdit->setText(QStringLiteral("0123456789:- "));

    m_convertButton = new QPushButton(QStringLiteral("开始转换"), optionGroup);
    m_convertButton->setMinimumWidth(100);

    optionLayout->addWidget(new QLabel(QStringLiteral("格式"), optionGroup), 0, 0);
    optionLayout->addWidget(m_formatCombo, 0, 1);
    optionLayout->addWidget(new QLabel(QStringLiteral("宽度"), optionGroup), 0, 2);
    optionLayout->addWidget(m_widthSpin, 0, 3);
    optionLayout->addWidget(new QLabel(QStringLiteral("高度"), optionGroup), 0, 4);
    optionLayout->addWidget(m_heightSpin, 0, 5);
    optionLayout->addWidget(new QLabel(QStringLiteral("字号"), optionGroup), 1, 0);
    optionLayout->addWidget(m_fontSizeSpin, 1, 1);
    optionLayout->addWidget(new QLabel(QStringLiteral("数组后缀"), optionGroup), 1, 2);
    optionLayout->addWidget(m_suffixEdit, 1, 3);
    optionLayout->addWidget(new QLabel(QStringLiteral("字符"), optionGroup), 2, 0);
    optionLayout->addWidget(m_charsEdit, 2, 1, 1, 5);
    optionLayout->addWidget(m_convertButton, 3, 5);
    optionLayout->setColumnStretch(3, 1);
    optionLayout->setColumnStretch(5, 1);

    rootLayout->addWidget(fileGroup);
    rootLayout->addWidget(optionGroup);
    rootLayout->addStretch();

    connect(m_fontBrowseButton, &QPushButton::clicked, this, &FontConvertWidget::browseFontFile);
    connect(m_convertButton, &QPushButton::clicked, this, &FontConvertWidget::convertFont);
}

void FontConvertWidget::browseFontFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择字体文件"),
        QString(),
        QStringLiteral("字体文件 (*.ttf *.ttc *.otf);;所有文件 (*.*)"));
    if (!filePath.isEmpty()) {
        m_fontPathEdit->setText(filePath);
    }
}

void FontConvertWidget::convertFont()
{
    FontConvertCore::Options options;
    options.fontPath = m_fontPathEdit->text();
    options.outputPath = outputFilePath();
    options.width = m_widthSpin->value();
    options.height = m_heightSpin->value();
    options.fontSize = m_fontSizeSpin->value();
    options.suffix = m_suffixEdit->text().trimmed();
    options.chars = m_charsEdit->text();
    options.format = static_cast<FontConvertCore::Format>(m_formatCombo->currentData().toInt());

    setControlsEnabled(false);
    const FontConvertCore::Result result = FontConvertCore::convert(options);
    setControlsEnabled(true);

    if (!result.success) {
        BSD_LOG(LOG_ERRO, QStringLiteral("字模转换失败：%1\n").arg(result.errorMessage));
        return;
    }

    for (const QString &message : result.messages) {
        BSD_LOG(LOG_INFO, message + QStringLiteral("\n"));
    }
}

QString FontConvertWidget::outputFilePath() const
{
    QString fileName = m_outputNameEdit->text().trimmed();
    if (fileName.isEmpty()) {
        return QString();
    }

    const QFileInfo fileInfo(QFileInfo(fileName).fileName());
    fileName = fileInfo.completeBaseName();
    if (fileName.isEmpty()) {
        return QString();
    }
    fileName += QStringLiteral(".h");

    const QString outputDir = m_outputDir.trimmed().isEmpty()
        ? QStringLiteral("out/font_convert/")
        : m_outputDir.trimmed();
    QDir dir(outputDir);
    if (dir.isAbsolute()) {
        return dir.filePath(fileName);
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QDir(outputDir).filePath(fileName));
}

void FontConvertWidget::setControlsEnabled(bool enabled)
{
    m_fontPathEdit->setEnabled(enabled);
    m_fontBrowseButton->setEnabled(enabled);
    m_outputNameEdit->setEnabled(enabled);
    m_formatCombo->setEnabled(enabled);
    m_widthSpin->setEnabled(enabled);
    m_heightSpin->setEnabled(enabled);
    m_fontSizeSpin->setEnabled(enabled);
    m_suffixEdit->setEnabled(enabled);
    m_charsEdit->setEnabled(enabled);
    m_convertButton->setEnabled(enabled);
}
