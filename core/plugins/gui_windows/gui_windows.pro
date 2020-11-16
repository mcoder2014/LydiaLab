include($$PWD/../../starlab.prf)
include($$PWD/../../../depends/eigen.pri)
StarlabTemplate(plugin)

HEADERS += \
    LayerGuiPlugin.h \
    layerDialog.h
SOURCES += \
    LayerGuiPlugin.cpp \
    layerDialog.cpp

FORMS += \
    layerDialog.ui





