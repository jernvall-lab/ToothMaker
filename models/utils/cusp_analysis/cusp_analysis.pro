TEMPLATE = app
CONFIG -= app_bundle
QT -= core gui
INCLUDEPATH += src/

QMAKE_CXXFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
QMAKE_CFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
QMAKE_LFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 -std=c++17
QMAKE_CXXFLAGS_DEBUG += -std=c++17

equals(OSX, "10.6") {
    include(../../../gcc-macports.pri)
    QMAKE_LFLAGS += -static-libstdc++ -static-libgcc
} else {
    mac: include(../../../clang-macports.pri)
}

SOURCES += src/cusp_analysis.cpp
TARGET = cusp_analysis
