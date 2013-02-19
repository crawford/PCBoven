QT      += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = control
TEMPLATE = app


SOURCES += src/main.cpp \
           src/controlpanel.cpp \
           src/ovenmanager.cpp \
           src/udevmonitor.cpp \
           src/ioctlworker.cpp

HEADERS += src/controlpanel.h \
           src/ovenmanager.h \
           src/udevmonitor.h \
           src/ioctlworker.h

FORMS   += ui/controlpanel.ui

DESTDIR     = build
OBJECTS_DIR = build
MOC_DIR     = build
UI_DIR      = build
