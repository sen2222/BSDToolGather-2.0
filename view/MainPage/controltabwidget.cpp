#include "controltabwidget.h"
#include "ui_controltabwidget.h"

ControlTabWidget::ControlTabWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlTabWidget)
{
    ui->setupUi(this);
}

ControlTabWidget::~ControlTabWidget()
{
    delete ui;
}
