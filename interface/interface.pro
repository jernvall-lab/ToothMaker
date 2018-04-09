#-------------------------------------------------
#
# Project created by QtCreator 2011-02-02T10:06:56
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = ToothMaker
TEMPLATE = app

SOURCES += \
    src/gui/parameterwindow.cpp \
    src/gui/scanwindow.cpp \
    src/misc/binaryhandler.cpp \
    src/main.cpp \
    src/gui/hampu.cpp \
    src/gui/glwidget.cpp \
    src/gui/controlpanel.cpp \
    src/misc/scanlist.cpp \
    src/cli/cmdappcore.cpp \
    src/cli/glengine.cpp \
    src/utils/writeparameters.cpp \
    src/utils/readparameters.cpp \
    src/utils/writedata.cpp \
    src/utils/readxml.cpp \
    src/misc/loader.cpp \
    src/renderer/glcore.cpp \
    ../common/model.cpp \
    ../common/parameters.cpp \
    ../common/colormap.cpp \
    ../common/readdata.cpp \
    src/renderer/gl_modern.cpp \
    src/renderer/gl_legacy.cpp

HEADERS += \
    src/gui/parameterwindow.h \
    src/gui/scanwindow.h \
    src/misc/binaryhandler.h \
    src/gui/hampu.h \
    src/gui/glwidget.h \
    src/gui/controlpanel.h \
    src/misc/scanlist.h \
    src/cli/cmdappcore.h \
    src/cli/glengine.h \
    src/renderer/glcore.h \
    src/utils/writeparameters.h \
    src/utils/readparameters.h \
    src/utils/writedata.h \
    src/utils/readxml.h \
    src/misc/loader.h \
    src/renderer/glcore.h \
    ../common/model.h \
    ../common/mesh.h \
    ../common/parameters.h \
    ../common/tooth.h \
    ../common/toothlife.h \
    ../common/morphomaker.h \
    ../common/colormap.h \
    ../common/readdata.h \
    src/renderer/gl_modern.h \
    src/renderer/gl_legacy.h


INCLUDEPATH += src/ \
               ../common/ \
               ../ext/glm \
               /opt/local/include

VERSION = $$system(git rev-list --count HEAD)
BUILD_NUM = '\\"$${VERSION}\\"'
DEFINES += MMAKER_BUILD=\"$${BUILD_NUM}\"

# All -arch * flags need to be removed for non-Apple compilers in OS X:
QMAKE_CFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
QMAKE_CXXFLAGS_X86_64  -= -arch x86_64 -Xarch_x86_64
QMAKE_LFLAGS -= -arch x86_64 -Xarch_x86_64
QMAKE_LFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64

unix:!macx: CONFIG += c++11

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE +=
unix:!win32: QMAKE_CXXFLAGS_RELEASE += -O3 -std=c++11 -fopenmp
QMAKE_CXXFLAGS_DEBUG +=
unix:!win32: QMAKE_CXXFLAGS_DEBUG += -std=c++11

QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE +=
unix:!win32: QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CFLAGS_DEBUG +=
unix:!win32: QMAKE_CFLAGS_DEBUG +=

mac: QMAKE_LFLAGS += -L/opt/local/lib -fopenmp
unix:!macx: QMAKE_LFLAGS += -fopenmp -rdynamic

unix:!macx: LIBS += -lX11
win32: LIBS += -lopengl32

equals(OSX, "10.6") {
    include(../gcc-macports.pri)
} else {
    mac: include(../clang-macports.pri)
}
