# Makefile for humppa Fortran tooth simulation
# Builds the original Fortran version of the tooth development model

FC = gfortran
FFLAGS = -O2 -static-libgfortran -static-libgcc

# Source and target (output to bin/ directory)
SOURCE = humppa_translate.f90
TARGET = ../../bin/humppa_fortran_64bit
BINDIR = ../../bin

# Default target
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(FC) $(FFLAGS) -o $@ $<
ifeq ($(shell uname),Darwin)
	# macOS: Copy libquadmath to bin/ and fix binary to use local copy
	@QUADMATH_PATH=$$(otool -L $@ | grep libquadmath | awk '{print $$1}'); \
	if [ -n "$$QUADMATH_PATH" ] && [ -f "$$QUADMATH_PATH" ]; then \
		cp -f "$$QUADMATH_PATH" $(BINDIR)/libquadmath.0.dylib; \
		install_name_tool -change "$$QUADMATH_PATH" @executable_path/libquadmath.0.dylib $@; \
		echo "Bundled libquadmath.0.dylib"; \
	fi
endif

clean:
	rm -f $(TARGET) $(BINDIR)/libquadmath.0.dylib *.mod

.PHONY: all clean
