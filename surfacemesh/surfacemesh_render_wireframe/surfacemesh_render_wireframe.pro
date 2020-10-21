include($$[STARLAB])
include($$[SURFACEMESH])
include($$PWD/../../depends/eigen.pri)
StarlabTemplate(plugin)

DEFINES += TODO_WIREFRAME_VBUFFER

HEADERS = \
    plugin.h
SOURCES = \
    plugin.cpp
RESOURCES = \
    plugin.qrc

OTHER_FILES += \
    wireframe.png
