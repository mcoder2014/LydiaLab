include($$[STARLAB])
include($$[SURFACEMESH])
include($$PWD/../../depends/eigen.pri)
StarlabTemplate(plugin)

HEADERS += surfacemesh_filter_laplacian_smoothing.h
SOURCES += surfacemesh_filter_laplacian_smoothing.cpp
