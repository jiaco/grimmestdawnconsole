#-------------------------------------------------
#
# Project created by QtCreator 2013-05-15T10:28:18
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = grimmestdawnconsole
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += ZLIB_WINAPI

INCLUDEPATH += C:\Qt\Qt5.0.2\5.0.2\Src\qtbase\src\3rdparty\zlib
#LIBS += -lzlibwap

SOURCES += main.cpp \
    utility.cpp \
    arz.cpp

HEADERS += \
    utility.h \
    arz.h
