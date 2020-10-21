include($$PWD/../../starlab.prf)
include($$PWD/../../../depends/eigen.pri)
StarlabTemplate(plugin)

HEADERS += gui_filter.h \
    FilterDockWidget.h
SOURCES += gui_filter.cpp \
    FilterDockWidget.cpp

FORMS += \
    FilterDockWidget.ui




