#ifndef CONTROLTABWIDGET_H
#define CONTROLTABWIDGET_H

#include <QWidget>

namespace Ui {
class ControlTabWidget;
}

class ControlTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlTabWidget(QWidget *parent = nullptr);
    ~ControlTabWidget();

private:
    Ui::ControlTabWidget *ui;
};

#endif // CONTROLTABWIDGET_H
