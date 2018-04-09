#!/bin/bash

#
# Runs macdeployqt on ToothMaker bundle, followed by fixing the missing libraries. 
# NOTE: Assumes a MacPorts installation with specific file locations!
#
# Usage: sh install_names_toothmaker.sh path/to/ToothMaker.app
#

BUNDLE_PATH=$1
CONT_PATH=$BUNDLE_PATH/Contents
macdeployqt $BUNDLE_PATH

cp /opt/local/lib/libgcc/libgcc_s.1.dylib $CONT_PATH/Frameworks/
cp /opt/local/lib/libgcc/libstdc++.6.dylib $CONT_PATH/Frameworks/
cp /opt/local/lib/libgcc/libgomp.1.dylib $CONT_PATH/Frameworks/

install_name_tool $CONT_PATH/MacOS/ToothMaker -change /opt/local/lib/libgcc/libstdc++.6.dylib @executable_path/../Frameworks/libstdc++.6.dylib
install_name_tool $CONT_PATH/MacOS/ToothMaker -change /opt/local/lib/libgcc/libgcc_s.1.dylib @executable_path/../Frameworks/libgcc_s.1.dylib
install_name_tool $CONT_PATH/MacOS/ToothMaker -change /opt/local/lib/libgcc/libgomp.1.dylib @executable_path/../Frameworks/libgomp.1.dylib

install_name_tool $CONT_PATH/Frameworks/libstdc++.6.dylib -change /opt/local/lib/libgcc/libgcc_s.1.dylib @executable_path/../Frameworks/libgcc_s.1.dylib
install_name_tool $CONT_PATH/Frameworks/libgomp.1.dylib -change /opt/local/lib/libgcc/libgcc_s.1.dylib @executable_path/../Frameworks/libgcc_s.1.dylib
install_name_tool $CONT_PATH/Frameworks/libstdc++.6.dylib -id libstdc++.6.dylib
install_name_tool $CONT_PATH/Frameworks/libgomp.1.dylib -id libgomp.1.dylib
install_name_tool $CONT_PATH/Frameworks/libgcc_s.1.dylib -id libgcc_s.1.dylib

cp Info.plist $CONT_PATH
