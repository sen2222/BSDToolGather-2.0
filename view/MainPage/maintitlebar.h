#ifndef MAINTITLEBAR_H
#define MAINTITLEBAR_H

#include <QWidget>
#include <QTimer>
#include "alltoolfun.h"


#define CONFIG_SECTION_MAIN_TITLE_BAR   "MainTitleBar"
#define CONFIG_KEY_TIME_SHOW            "TimeShow"

namespace Ui {
class MainTitleBar;
}

class MainTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit MainTitleBar(QWidget *parent = nullptr);
    ~MainTitleBar();

private slots:
    void on_btnTimeShow_clicked();

private:
    void updateSystemTime();
    void timeShowSwitch(int showStatus = false);
    
    Ui::MainTitleBar *ui;
    QTimer *m_timer;
    int timeShowStatus;
};

#endif // MAINTITLEBAR_H
