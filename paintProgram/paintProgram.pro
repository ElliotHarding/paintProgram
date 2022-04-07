QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += resources_big

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    canvas.cpp \
    dlg_blursettings.cpp \
    dlg_brushsettings.cpp \
    dlg_colormultipliers.cpp \
    dlg_effectssliders.cpp \
    dlg_huesaturation.cpp \
    dlg_info.cpp \
    dlg_layers.cpp \
    dlg_message.cpp \
    dlg_sensitivity.cpp \
    dlg_setcanvassettings.cpp \
    dlg_shapes.cpp \
    dlg_sketch.cpp \
    dlg_textsettings.cpp \
    dlg_tools.cpp \
    main.cpp \
    mainwindow.cpp \
    wdg_layerlistitem.cpp

HEADERS += \
    canvas.h \
    canvaslayer.h \
    dlg_blursettings.h \
    dlg_brushsettings.h \
    dlg_colormultipliers.h \
    dlg_effectssliders.h \
    dlg_huesaturation.h \
    dlg_info.h \
    dlg_layers.h \
    dlg_message.h \
    dlg_sensitivity.h \
    dlg_setcanvassettings.h \
    dlg_shapes.h \
    dlg_sketch.h \
    dlg_textsettings.h \
    dlg_tools.h \
    mainwindow.h \
    tools.h \
    wdg_layerlistitem.h

FORMS += \
    dlg_blursettings.ui \
    dlg_brushsettings.ui \
    dlg_colormultipliers.ui \
    dlg_effectssliders.ui \
    dlg_huesaturation.ui \
    dlg_info.ui \
    dlg_layers.ui \
    dlg_message.ui \
    dlg_sensitivity.ui \
    dlg_setcanvassettings.ui \
    dlg_shapes.ui \
    dlg_sketch.ui \
    dlg_textsettings.ui \
    dlg_tools.ui \
    mainwindow.ui \
    wdg_layerlistitem.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
