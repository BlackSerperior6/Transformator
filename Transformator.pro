QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += WIN32_LEAN_AND_MEAN

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    abstractport.cpp \
    connectionedit.cpp \
    main.cpp \
    mainwindow.cpp \
    portsconnection.cpp \
    serialport.cpp \
    tcpclientconnection.cpp \
    tcpport.cpp \
    threadpool.cpp \
    utils.cpp

HEADERS += \
    PortType.h \
    abstractport.h \
    connectionedit.h \
    mainwindow.h \
    portsconnection.h \
    serialport.h \
    tcpclientconnection.h \
    tcpport.h \
    tcpstatuscode.h \
    threadpool.h

FORMS += \
    connectionedit.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
