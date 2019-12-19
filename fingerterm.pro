QT = core gui qml quick

CONFIG += link_pkgconfig
linux-oe-g++ {
    LIBS += -lqsgepaper
        isEmpty(DEFAULT_FONT) {
                DEFAULT_FONT = NotoMono
        }
}

enable-feedback {
    QT += feedback
    DEFINES += HAVE_FEEDBACK
}

enable-nemonotifications {
    PKGCONFIG += nemonotifications-qt5
}
DEFINES += DEFAULT_FONTFAMILY=\\\"$$DEFAULT_FONT\\\"

TEMPLATE = app
TARGET = fingerterm
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lutil

# Input
HEADERS += \
    ptyiface.h \
    terminal.h \
    textrender.h \
    version.h \
    util.h \
    keyloader.h

SOURCES += \
    main.cpp \
    terminal.cpp \
    textrender.cpp \
    ptyiface.cpp \
    util.cpp \
    keyloader.cpp

RESOURCES += \
    resources.qrc

target.path = /usr/bin
INSTALLS += target

