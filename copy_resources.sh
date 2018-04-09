#!/bin/bash

#
# Copies resources and models so that ToothMaker can see them.
# Execute this script inside the build folder.
#
# TODO: Add Windows.
#

#
# Platform check.
#
platform=`uname`
RESOURCES=""
BINPATH=""
LIBTYPE=""
if [ "$platform" == "Darwin" ]; then
    RESOURCES='interface/ToothMaker.app/Contents/Resources/'
    BINPATH='mac/32'
    LIBTYPE='.dylib'
fi
if [ "$platform" == "Linux" ]; then
    RESOURCES='interface/Resources/'
    BINPATH='linux/32'
    LIBTYPE='.so'
fi

if [ ! -n $RESOURCES ]; then
    echo "Error: unknown platform '$platform'"
    exit
fi

echo "Detected platform: $platform"
if [ -f "copy_resources.sh" ]; then
    echo "Error: Don't run me in the root folder!"
    exit
fi

#
# Check if given directory exists, abort the script if not.
#
dir_exists ()
{
    if [ ! -d $1 ]; then
	    echo "Error: cannot find directory '$1'. Aborting."
	    exit
    fi
}


dir_exists interface
printf "\n** Copying interface resources.\n"
mkdir -p $RESOURCES'bin/'
cp ../interface/data/images/* $RESOURCES
cp ../interface/src/renderer/*.glsl $RESOURCES

dir_exists models
printf "\n** Copying models.\n"
for d in models/*/; do 
    if [ -d $d ]; then
	    cmd='cp '$d'*'$LIBTYPE' '$RESOURCES'bin/'
	    echo $cmd
	    $cmd
    fi
done


dir_exists ../models
printf "\n** Copying model resources.\n"
for d in ../models/*/; do
    fulld=$d'data'
    if [ -d $fulld ]; then
	    cmd='cp '$d'data/* '$RESOURCES
	    echo $cmd
	    $cmd
    fi
done

dir_exists ../models
printf "\n** Copying binary/script models.\n"
for d in ../models/*/; do
    fulld=$d'bin'
    if [ -d $fulld ]; then
        cmd='cp '$d'bin/* '$RESOURCES'bin/'
        echo $cmd
        $cmd
    fi
done

dir_exists models/utils
printf "\n** Copying model utilities.\n"
for d in models/utils/*/; do
    if [ -d $d ]; then
        # Assume the binary name matches with the folder name.
        fname=`echo $d | cut -d'/' -f 3`
        cmd='cp '$d''$fname' '$RESOURCES'bin/'
        echo $cmd
        $cmd
    fi
done

