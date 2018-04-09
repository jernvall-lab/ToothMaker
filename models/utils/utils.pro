TEMPLATE = subdirs

equals(OSX, "10.6") {
    include(../../gcc-macports.pri)
} else {
    mac: include(../../clang-macports.pri)
}

SUBDIRS = dad_to_polygons \
          no_empty_lines \
          top_cusp_angle
          # main_cusp_baseline

CONFIG += ordered

