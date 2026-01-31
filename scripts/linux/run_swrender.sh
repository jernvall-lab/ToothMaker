#!/bin/sh

# Force using software rendering.
# This option doesn't have any effect if running nvidia binary drivers.
QT_AUTO_SCREEN_SCALE_FACTOR=1 LIBGL_ALWAYS_SOFTWARE=1 ./ToothMaker
