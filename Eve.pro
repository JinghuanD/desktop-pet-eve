QT       += core gui network

# Application target name.
TARGET = Eve

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 ssl


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QT += multimedia

SOURCES += \
    eve.cpp \
    gamewin.cpp \
    healthwin.cpp \
    readingcompanion.cpp \
    dresswin.cpp \
    main.cpp \
    musicwin.cpp \
    pawmenuwidget.cpp \
    setwin.cpp \
    timerwin.cpp

HEADERS += \
    eve.h \
    gamewin.h \
    healthwin.h \
    readingcompanion.h \
    dresswin.h \
    musicwin.h \
    pawmenuwidget.h \
    setwin.h \
    timerwin.h

FORMS += \
    eve.ui \
    gamewin.ui \
    dresswin.ui \
    musicwin.ui \
    setwin.ui \
    timerwin.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

# Automatically copy file.ini to the build directory.
filecopy.files = file.ini
filecopy.path = $${OUT_PWD}
INSTALLS += filecopy

win32: LIBS += -lole32 -loleaut32 -lwbemuuid -loleacc -lpsapi -luuid
