# Makefile for humppa Fortran tooth simulation
# Builds the original Fortran version of the tooth development model

FC = gfortran
FFLAGS = -O2 -static-libgfortran -static-libgcc

# Source and target (output to bin/ directory)
SOURCE = humppa_translate.f90
TARGET = ../../bin/humppa_fortran_64bit

# Default target
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(FC) $(FFLAGS) -o $@ $<
ifeq ($(shell uname),Darwin)
	# macOS: Fix libquadmath path to look in executable directory
	-install_name_tool $@ -change /opt/local/lib/libgcc/libquadmath.0.dylib @executable_path/libquadmath.0.dylib 2>/dev/null || true
endif

clean:
	rm -f $(TARGET) *.mod

.PHONY: all clean
