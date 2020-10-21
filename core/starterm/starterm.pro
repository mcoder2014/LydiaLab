include($$PWD/../starlab.prf)
include($$PWD/../../depends/eigen.pri)
StarlabTemplate(console)

TARGET = starterm

HEADERS += \
    QCommandLine.h \
    CmdLineParser.h

SOURCES += main.cpp \
    QCommandLine.cpp \
    CmdLineParser.cpp
