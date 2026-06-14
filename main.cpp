#include "widget.h"

#include <QApplication>
#include <QString>
#include "alltoolfun.h"


#define CONFIG_SECTION_SYSTEM_INFO  "SystemInfo"
#define CONFIG_KEY_EMAIL            "Email"
#define CONFIG_KEY_VERSION          "Version"   

int main(int argc, char *argv[])
{
    // BSD_LOG(LOG_INFO, QString(">作者: %1 \n").arg("sent"));
    BSD_LOG(LOG_INFO, QString(">邮箱: %1\n").arg(CONFIG_READ_STRING(CONFIG_SECTION_SYSTEM_INFO, CONFIG_KEY_EMAIL, "")));
    BSD_LOG(LOG_INFO, QString(">版本号: ToolGether-BSD-%1\n").arg(CONFIG_READ_STRING(CONFIG_SECTION_SYSTEM_INFO, CONFIG_KEY_VERSION, "")));
    BSD_LOG(LOG_INFO,  QString("--------------------------------------------------------------------------------------------------\n"));

    QApplication a(argc, argv);
    Widget w;
    w.setWindowTitle("BSDToolGather");
    w.setWindowIcon(QIcon(":/res/test.ico"));
    w.show();
    return a.exec();
}
