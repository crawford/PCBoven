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

unix:INCLUDEPATH  += /usr/include/libusb-1.0
INCLUDEPATH += $$PWD

win32:LIBS += -llibusb-1.0
unix:LIBS  += -lusb-1.0

DESTDIR     = build
OBJECTS_DIR = build
MOC_DIR     = build
UI_DIR      = build
