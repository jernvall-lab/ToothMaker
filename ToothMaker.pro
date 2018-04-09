TEMPLATE = subdirs

# To compile in OS X 10.6 we need to use GCC installed via third party means,
# e.g, Macports (Clang doesn't work).
equals(OSX, "10.6") {
    include(gcc-macports.pri)
} else {
    mac: include(clang-macports.pri)
}

SUBDIRS = interface \
          models
          
CONFIG += ordered

unix: resources.commands = ../copy_resources.sh
QMAKE_EXTRA_TARGETS += resources
