include(../common.pri)
TARGET = emailplugin
TEMPLATE = lib

CONFIG += plugin link_pkgconfig

PKGCONFIG += meego-ux-content qmfclient libedataserver-1.2

# use pkg-config paths for include in both g++ and moc
INCLUDEPATH += $$system(pkg-config --cflags meego-ux-content \
    | tr \' \' \'\\n\' | grep ^-I | cut -d 'I' -f 2-)

INCLUDEPATH += ../lib
LIBS += -L../lib -lmeegoemail

OBJECTS_DIR = .obj
MOC_DIR = .moc

SOURCES += \
    emailfeedmodel.cpp \
    emailplugin.cpp \
    emailservicemodel.cpp

HEADERS += \
    emailfeedmodel.h \
    emailplugin.h \
    emailservicemodel.h

target.path = $$[QT_INSTALL_PLUGINS]/MeeGo/Content
INSTALLS += target
