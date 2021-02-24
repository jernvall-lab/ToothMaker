#
# Compiles humppa with gfortran and updates the libquadmath search location to
# the binary execution folder. libquadmath is included in ../bin/
#
# Assumes gfortran finds libquadmath in the default MacPorts installation path.
#

gfortran -O2 -static-libgfortran -static-libgcc humppa_translate.f90

install_name_tool a.out -change /opt/local/lib/libgcc/libquadmath.0.dylib @executable_path/libquadmath.0.dylib

