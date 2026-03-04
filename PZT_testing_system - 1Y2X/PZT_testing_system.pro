QT       += core gui network charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    udphelper.cpp \
    udpsimulator.cpp

HEADERS += \
    Structs.h \
    mainwindow.h \
    udphelper.h \
    udpsimulator.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


# LIBS += -lvisa64
# INCLUDEPATH += "C:\Program Files (x86)\IVI Foundation\visa\WinNT\TekVISA\Bin"

# VISA 包含头文件
INCLUDEPATH += "C:/Program Files (x86)/IVI Foundation/visa/WinNT/Include"

# VISA 库文件
LIBS += "C:/Program Files (x86)/IVI Foundation/visa/WinNT/Lib_x64/msc/visa64.lib"

RESOURCES += \
    resource.qrc

INCLUDEPATH += $$PWD/UICustom
include($$PWD/UICustom/UICustom.pri)
INCLUDEPATH += $$PWD/GeneralTools
include($$PWD/GeneralTools/GeneralTools.pri)
INCLUDEPATH += $$PWD/CsvModule
include($$PWD/CsvModule/CsvModule.pri)
INCLUDEPATH += $$PWD/MathModule
include($$PWD/MathModule/MathModule.pri)

