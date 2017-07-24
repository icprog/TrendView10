#-------------------------------------------------
#
# Project created by QtCreator 2015-11-30T11:30:03
#
#-------------------------------------------------

QT       += core gui network printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = TrendView10
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp \
    datareceiver.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    autostopthread.h \
    datareceiver.h

FORMS    += mainwindow.ui

RESOURCES += \
    res.qrc

#add ico to windows application
RC_FILE = myapp.rc

OTHER_FILES += \
    myapp.rc
