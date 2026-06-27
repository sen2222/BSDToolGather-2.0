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

    Ui::ControlTabWidget *ui;
    int m_stressUpgradeTabIndex;
    int m_fontConvertTabIndex;
};

#endif // CONTROLTABWIDGET_H
