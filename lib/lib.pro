include(../common.pri)
TARGET = meegoemail
TEMPLATE = lib

CONFIG += link_pkgconfig \
    mobility

PKGCONFIG += qmfmessageserver \
    qmfclient	\
    libedataserver-1.2	\
    camel-1.2 \
    gconf-2.0

OBJECTS_DIR = .obj
MOC_DIR = .moc

SOURCES += \
    emailaccountlistmodel.cpp \
    emailmessagelistmodel.cpp \
    folderlistmodel.cpp	      \
    dbustypes.cpp	      \
    e-gdbus-emailsession-proxy.cpp \
    e-gdbus-emailstore-proxy.cpp   \
    e-gdbus-emailfolder-proxy.cpp  

INSTALL_HEADERS += \
    emailaccountlistmodel.h \
    emailmessagelistmodel.h \
    folderlistmodel.h

HEADERS += \
    $$INSTALL_HEADERS	\
    dbustypes.h             \
    e-gdbus-emailsession-proxy.h \
    e-gdbus-emailstore-proxy.h   \
    e-gdbus-emailfolder-proxy.h


target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

headers.files += $$INSTALL_HEADERS
headers.path += $$INSTALL_ROOT/usr/include/meegoemail
INSTALLS += headers

pkgconfig.files += meegoemail.pc
pkgconfig.path += $$[QT_INSTALL_LIBS]/pkgconfig
INSTALLS += pkgconfig
