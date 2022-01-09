QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += resources_big

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Canvas.cpp \
    dlg_brushsettings.cpp \
    dlg_sensitivity.cpp \
    dlg_setcanvassettings.cpp \
    dlg_shapes.cpp \
    dlg_textsettings.cpp \
    dlg_tools.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Canvas.h \
    dlg_brushsettings.h \
    dlg_sensitivity.h \
    dlg_setcanvassettings.h \
    dlg_shapes.h \
    dlg_textsettings.h \
    dlg_tools.h \
    mainwindow.h \
    tools.h

FORMS += \
    dlg_brushsettings.ui \
    dlg_sensitivity.ui \
    dlg_setcanvassettings.ui \
    dlg_shapes.ui \
    dlg_textsettings.ui \
    dlg_tools.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
