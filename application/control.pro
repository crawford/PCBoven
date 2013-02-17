QT      += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = control
TEMPLATE = app


SOURCES += src/main.cpp \
           src/controlpanel.cpp

HEADERS += src/controlpanel.h

FORMS   += ui/controlpanel.ui

DESTDIR     = build
OBJECTS_DIR = build
MOC_DIR     = build
UI_DIR      = build
