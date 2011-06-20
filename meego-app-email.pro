include(common.pri)
TEMPLATE = subdirs 
CONFIG += ordered
SUBDIRS += lib plugin content

qmlfiles.files += *.qml settings/
qmlfiles.path += $$INSTALL_ROOT/usr/share/$$TARGET

desktop.files += meego-app-email.desktop
desktop.path += $$INSTALL_ROOT/usr/share/applications/

setting_desktop.files += email-settings.desktop
setting_desktop.path += $$INSTALL_ROOT/usr/share/meego-ux-settings/apps/

schemas.files += meego-app-email.schemas
schemas.path +=$$INSTALL_ROOT/etc/gconf/schemas/

INSTALLS += qmlfiles desktop setting_desktop schemas

TRANSLATIONS += *.qml settings/*.qml settings/*.js
PROJECT_NAME = meego-app-email

dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION} &&
dist.commands += git clone . $${PROJECT_NAME}-$${VERSION} &&
dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION}/.git &&
dist.commands += rm -f $${PROJECT_NAME}-$${VERSION}/.gitignore &&
dist.commands += mkdir -p $${PROJECT_NAME}-$${VERSION}/ts &&
dist.commands += lupdate $${TRANSLATIONS} -ts $${PROJECT_NAME}-$${VERSION}/ts/$${PROJECT_NAME}.ts &&
dist.commands += tar jcpvf $${PROJECT_NAME}-$${VERSION}.tar.bz2 $${PROJECT_NAME}-$${VERSION} &&
dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION}
QMAKE_EXTRA_TARGETS += dist
