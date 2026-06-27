#include "controltabwidget.h"
#include "ui_controltabwidget.h"

#include "fontconvertwidget.h"
#include "packetappwidget.h"
#include "stressupgradewidget.h"

#include <QSizePolicy>
#include <QTabWidget>
#include <QVBoxLayout>

ControlTabWidget::ControlTabWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlTabWidget)
    , m_stressUpgradeTabIndex(-1)
    , m_fontConvertTabIndex(-1)
{
    ui->setupUi(this);
    setupTabs();
}

ControlTabWidget::~ControlTabWidget()
{
    delete ui;
}

void ControlTabWidget::setupTabs()
{
    ui->tabWidget->setTabText(0, QStringLiteral("OTA打包"));
    ui->tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *packetTab = ui->tabWidget->widget(0);
    packetTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *layout = new QVBoxLayout(packetTab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    PacketAppWidget *packetAppWidget = new PacketAppWidget(packetTab);
    packetAppWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(packetAppWidget, 1);

    QWidget *stressUpgradeTab = new QWidget(ui->tabWidget);
    stressUpgradeTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_stressUpgradeTabIndex = ui->tabWidget->addTab(stressUpgradeTab, QStringLiteral("压测升级"));

    QWidget *fontConvertTab = new QWidget(ui->tabWidget);
    fontConvertTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_fontConvertTabIndex = ui->tabWidget->addTab(fontConvertTab, QStringLiteral("字模转换"));

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == m_stressUpgradeTabIndex) {
            ensureStressUpgradeTabLoaded();
        } else if (index == m_fontConvertTabIndex) {
            ensureFontConvertTabLoaded();
        }
    });

    if (ui->tabWidget->currentIndex() == m_stressUpgradeTabIndex) {
        ensureStressUpgradeTabLoaded();
    } else if (ui->tabWidget->currentIndex() == m_fontConvertTabIndex) {
        ensureFontConvertTabLoaded();
    }
}

void ControlTabWidget::ensureStressUpgradeTabLoaded()
{
    QWidget *stressUpgradeTab = ui->tabWidget->widget(m_stressUpgradeTabIndex);
    if (!stressUpgradeTab || stressUpgradeTab->layout()) {
        return;
    }

    QVBoxLayout *layout = new QVBoxLayout(stressUpgradeTab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    StressUpgradeWidget *stressUpgradeWidget = new StressUpgradeWidget(stressUpgradeTab);
    stressUpgradeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(stressUpgradeWidget, 1);
}

void ControlTabWidget::ensureFontConvertTabLoaded()
{
    QWidget *fontConvertTab = ui->tabWidget->widget(m_fontConvertTabIndex);
    if (!fontConvertTab || fontConvertTab->layout()) {
        return;
    }

    QVBoxLayout *layout = new QVBoxLayout(fontConvertTab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    FontConvertWidget *fontConvertWidget = new FontConvertWidget(fontConvertTab);
    fontConvertWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(fontConvertWidget, 1);
}
