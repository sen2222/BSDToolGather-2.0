#ifndef MAINLOGWIDGET_H
#define MAINLOGWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTextCharFormat>
#include "SystemLog/logoutput.h"

#define MAX_LOG_LINE_COUNT 2000


namespace Ui {
class MainLogWidget;
}

class MainLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainLogWidget(QWidget *parent = nullptr);
    ~MainLogWidget();

private:
    void appendLogEntry(void);

    Ui::MainLogWidget *ui;
    QTimer* m_logTimer; // 定时器，用于定时更新日志
};

#endif // MAINLOGWIDGET_H
