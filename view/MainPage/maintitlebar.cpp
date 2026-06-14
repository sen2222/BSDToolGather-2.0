#include "maintitlebar.h"
#include "ui_maintitlebar.h"

MainTitleBar::MainTitleBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainTitleBar)
{
    ui->setupUi(this);
    m_timer = new QTimer(this);
    timeShowStatus = CONFIG_READ_INT(CONFIG_SECTION_MAIN_TITLE_BAR, CONFIG_KEY_TIME_SHOW, 0);
    
    timeShowSwitch(timeShowStatus);
    connect(m_timer, &QTimer::timeout, this, &MainTitleBar::updateSystemTime);

}

MainTitleBar::~MainTitleBar()
{
    delete ui;
}


void MainTitleBar::timeShowSwitch(int showStatus)
{
    if (showStatus)
    {
        ui->labelTimeShow->setVisible(true);
        ui->btnTimeShow->setText("关闭时间显示");
        m_timer->start(1000);
        updateSystemTime();
    }
    else
    {
        ui->labelTimeShow->setVisible(false);
        ui->btnTimeShow->setText("开启时间显示");
        m_timer->stop();
    }
    timeShowStatus = showStatus;    
    CONFIG_WRITE_INT(CONFIG_SECTION_MAIN_TITLE_BAR, CONFIG_KEY_TIME_SHOW, timeShowStatus);
}

void MainTitleBar::updateSystemTime()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString timeStr = currentTime.toString("yyyy-MM-dd hh:mm:ss");
    
    ui->labelTimeShow->setText(timeStr);
}
void MainTitleBar::on_btnTimeShow_clicked()
{
    timeShowSwitch(!timeShowStatus);
}

