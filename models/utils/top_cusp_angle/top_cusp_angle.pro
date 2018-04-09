TEMPLATE = app
CONFIG -= app_bundle
QT -= core gui
DEFINES += COMPILE
INCLUDEPATH += src/

# Remove arguments GCC doesnt recognize.
QMAKE_CXXFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
QMAKE_CFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
QMAKE_LFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 -std=c++11
QMAKE_CXXFLAGS_DEBUG += -std=c++11

equals(OSX, "10.6") {
    include(../../../gcc-macports.pri)
    QMAKE_LFLAGS += -static-libstdc++ -static-libgcc
} else {
    mac: include(../../../clang-macports.pri)
}

INCLUDEPATH +=
HEADERS += src/top_cusp_angle.h
SOURCES += src/top_cusp_angle.cpp
TARGET = top_cusp_angle

