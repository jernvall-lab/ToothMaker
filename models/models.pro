TEMPLATE = subdirs

equals(OSX, "10.6") {
    include(../gcc-macports.pri)
} else {
    mac: include(../clang-macports.pri)
}

SUBDIRS = utils \

CONFIG += ordered

