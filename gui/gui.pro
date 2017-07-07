#-------------------------------------------------
#
# Project created by QtCreator 2017-05-11T13:38:06
#
#-------------------------------------------------

QT        += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET    = collaboriso
TEMPLATE  = app


SOURCES   += main.cpp \
             cutils.cpp \
             dialogs/boot.cpp \
             dialogs/about.cpp \
             dialogs/device.cpp \
             dialogs/progress.cpp \
             dialogs/settings.cpp \
             dialogs/collaboriso.cpp

HEADERS   += cutils.h \
             dialogs/boot.h \
             dialogs/about.h \
             dialogs/device.h \
             dialogs/progress.h \
             dialogs/settings.h \
             dialogs/collaboriso.h

RESOURCES += collaboriso.qrc
