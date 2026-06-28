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
    void setupTabs();
    void ensureStressUpgradeTabLoaded();
    void ensureFontConvertTabLoaded();
    void ensureCdcFunctionTabLoaded();

    Ui::ControlTabWidget *ui;
    int m_stressUpgradeTabIndex;
    int m_fontConvertTabIndex;
    int m_cdcFunctionTabIndex;
};

#endif // CONTROLTABWIDGET_H
