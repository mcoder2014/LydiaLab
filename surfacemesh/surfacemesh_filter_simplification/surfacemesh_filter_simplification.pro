include($$[STARLAB])
include($$[SURFACEMESH])
include($$PWD/../../depends/eigen.pri)
StarlabTemplate(plugin)

HEADERS += surfacemesh_filter_simplification.h decimater.h
SOURCES += surfacemesh_filter_simplification.cpp
