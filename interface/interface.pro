#-------------------------------------------------
#
# Project created by QtCreator 2011-02-02T10:06:56
#
#-------------------------------------------------

QT += core gui widgets

# Qt6 moved QOpenGLWidget to a separate module
greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

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
    src/utils/readxml.h \
    src/misc/loader.h \
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

VERSION = $$system(git rev-list --count HEAD)
BUILD_NUM = '\\"$${VERSION}\\"'
DEFINES += MMAKER_BUILD=\"$${BUILD_NUM}\"

# All -arch * flags need to be removed for non-Apple compilers in OS X:
QMAKE_CFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
QMAKE_CXXFLAGS_X86_64  -= -arch x86_64 -Xarch_x86_64
QMAKE_LFLAGS -= -arch x86_64 -Xarch_x86_64
QMAKE_LFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64

CONFIG += c++17

!win32-msvc* {
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE += -O3
}

QMAKE_FLAGS +=
unix:!macx: QMAKE_LFLAGS += -rdynamic

unix:!macx: LIBS += -lX11
# Windows: Compile GLEW from source (statically linked)
win32 {
    SOURCES += ../ext/GLEW/glew.c
    DEFINES += GLEW_STATIC
    # Suppress windef.h min()/max() macros so std::min/std::max compile.
    DEFINES += NOMINMAX
    LIBS += -lopengl32 -luser32 -lgdi32
    INCLUDEPATH += ../ext/GLEW
}

# On macOS, we need OpenGL framework but not the deprecated AGL framework
# AGL was removed in newer macOS SDKs
macx {
    # Qt5 supports macOS 10.13+, Qt6 requires 10.15+
    greaterThan(QT_MAJOR_VERSION, 5): QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
    else: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.13

    # Override Qt's OpenGL framework settings to exclude AGL
    QMAKE_LIBS_OPENGL = -framework OpenGL
    QMAKE_INCDIR_OPENGL = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/OpenGL.framework/Headers
    LIBS += -framework OpenGL
}

equals(OSX, "10.6") {
    include(../gcc-macports.pri)
} else {
    mac: include(../clang-macports.pri)
}
