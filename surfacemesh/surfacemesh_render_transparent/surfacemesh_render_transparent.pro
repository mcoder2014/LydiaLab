include($$[STARLAB])
include($$[SURFACEMESH])
include($$PWD/../../depends/eigen.pri)
StarlabTemplate(plugin)

HEADERS = \
    plugin.h
SOURCES = \
    plugin.cpp
RESOURCES = \
    plugin.qrc

OTHER_FILES += \
    transparent_mesh.png
