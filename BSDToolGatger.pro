QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17


# qDebug 打印是否输出
# DEFINES += QT_NO_DEBUG_OUTPUT


# Windows SetupAPI / CfgMgr32
win32:LIBS += -lsetupapi -lcfgmgr32


SOURCES += \
    main.cpp \
    application/FontConvert/fontconvertcore.cpp \
    application/PacketApp/packetappcore.cpp \
    application/StressUpgrade/stressupgradeworker.cpp \
    application/StressUpgrade/usbvidpidrules.cpp \
    tool/RingBuff/ringbuffer.cpp \
    tool/SerialPort/serialportquery.cpp \
    tool/SystemLog/logoutput.cpp \
    tool/ConfigBase/configbase.cpp \
    view/FontConvert/fontconvertwidget.cpp \
    view/PacketApp/packetappwidget.cpp \
    view/StressUpgrade/stressupgradewidget.cpp \
    view/MainPage/controltabwidget.cpp \
    view/MainPage/mainlogwidget.cpp \
    view/MainPage/maintitlebar.cpp \
    view/MainPage/widget.cpp


HEADERS += \
    application/FontConvert/fontconvertcore.h \
    application/PacketApp/packetappcore.h \
    application/StressUpgrade/stressupgradeworker.h \
    application/StressUpgrade/usbvidpidrules.h \
    tool/SafeQueue/safequeue.h \
    tool/RingBuff/ringbuffer.h \
    tool/SerialPort/serialportquery.h \
    tool/SystemLog/logoutput.h \
    tool/ConfigBase/configbase.h \
    tool/alltoolfun.h \
    view/FontConvert/fontconvertwidget.h \
    view/PacketApp/packetappwidget.h \
    view/StressUpgrade/stressupgradewidget.h \
    view/MainPage/controltabwidget.h \
    view/MainPage/mainlogwidget.h \
    view/MainPage/maintitlebar.h \
    view/MainPage/widget.h


FORMS += \
    view/MainPage/ui/mainlogwidget.ui \
    view/MainPage/ui/controltabwidget.ui \
    view/MainPage/ui/maintitlebar.ui \
    view/MainPage/ui/widget.ui


INCLUDEPATH += \
    application/FontConvert \
    application/PacketApp \
    application/StressUpgrade \
    view/FontConvert \
    view/PacketApp \
    view/StressUpgrade \
    view/MainPage \
    tool 


RESOURCES += \
    res.qrc

# freetype library
INCLUDEPATH += $$PWD/source/freetype/out/include/freetype2
LIBS += -L$$PWD/source/freetype/out/lib -lfreetype

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
