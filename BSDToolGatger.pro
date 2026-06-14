QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17


# qDebug 打印是否输出
# DEFINES += QT_NO_DEBUG_OUTPUT


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    tool/RingBuff/ringbuffer.cpp \
    tool/SystemLog/logoutput.cpp \
    tool/ConfigBase/configbase.cpp \ 
    \
    view/MainPage/controltabwidget.cpp \
    view/MainPage/mainlogwidget.cpp \
    view/MainPage/maintitlebar.cpp \
    view/MainPage/widget.cpp

HEADERS += \
    tool/SafeQueue/safequeue.h \
    tool/RingBuff/ringbuffer.h \
    tool/SystemLog/logoutput.h \
    tool/ConfigBase/configbase.h \ 
    tool/alltoolfun.h \
    \
    view/MainPage/controltabwidget.h \
    view/MainPage/mainlogwidget.h \
    view/MainPage/maintitlebar.h \
    view/MainPage/widget.h

FORMS += \
    view/MainPage/ui/mainlogwidget.ui \
    view/MainPage/ui/controltabwidget.ui \
    view/MainPage/ui/maintitlebar.ui \
    view/MainPage/ui/widget.ui \ 


INCLUDEPATH += \
    view/MainPage \
    tool


RESOURCES += \
    res.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


