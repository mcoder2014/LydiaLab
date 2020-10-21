include($$[STARLAB])
include($$[SURFACEMESH])
include($$PWD/../../depends/eigen.pri)
StarlabTemplate(plugin)

# linux does not have GLU included in += opengl
unix:!mac: LIBS += -lGLU

HEADERS = plugin.h
SOURCES = plugin.cpp
RESOURCES = plugin.qrc
OTHER_FILES += cloud.png
