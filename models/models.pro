TEMPLATE = subdirs

equals(OSX, "10.6") {
    include(../gcc-macports.pri)
} else {
    mac: include(../clang-macports.pri)
}

SUBDIRS = utils

CONFIG += ordered

# Tribosphenic model uses plain Makefile, not qmake
tribosphenic.commands = $(MAKE) -C $$PWD/tribosphenic
QMAKE_EXTRA_TARGETS += tribosphenic

# Build tribosphenic after utils (as part of default target)
first.depends = $(first) sub-utils-all-ordered tribosphenic
QMAKE_EXTRA_TARGETS += first

