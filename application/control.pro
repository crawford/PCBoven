QT      += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = control
TEMPLATE = app

SOURCES += src/main.cpp \
           src/controlpanel.cpp \
           src/ovenmanager.cpp \
           src/reflowprofile.cpp \
           src/reflowgraphwidget.cpp

HEADERS += src/controlpanel.h \
           src/ovenmanager.h \
           src/reflowprofile.h \
           src/reflowgraphwidget.h

FORMS   += ui/controlpanel.ui

INCLUDEPATH = ../driver/src

DESTDIR     = build
OBJECTS_DIR = build
MOC_DIR     = build
UI_DIR      = build
