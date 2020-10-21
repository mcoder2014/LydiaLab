include($$PWD/../../starlab.prf)
include($$PWD/../../../depends/eigen.pri)
StarlabTemplate(plugin)

HEADERS += gui_windows.h \
    layerDialog.h
SOURCES += gui_windows.cpp \
    layerDialog.cpp

FORMS += \
    layerDialog.ui





