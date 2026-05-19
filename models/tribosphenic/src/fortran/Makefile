# Makefile for humppa Fortran tooth simulation
# Builds the original Fortran version of the tooth development model

FC = gfortran
# -ffp-contract=off disables FMA fusion for consistent output across architectures.
# Without this, gfortran defaults to -ffp-contract=fast which fuses on ARM (where FMA
# is always available) but not on x86_64 (unless -mfma is also set), causing divergence.
FFLAGS = -O2 -ffp-contract=off -static-libgfortran -static-libgcc

# x86_64 needs -mfma to enable FMA3 hardware instructions
UNAME_M := $(shell uname -m 2>/dev/null)
ifeq ($(UNAME_M),x86_64)
  FFLAGS += -mfma
endif

# Windows/MinGW: fully static link so the binary doesn't need libquadmath-0.dll,
# libwinpthread-1.dll, libgcc_s_seh-1.dll alongside it. -static-libgfortran/-libgcc
# alone don't cover libquadmath and libwinpthread.
# OS=Windows_NT is set by cmd.exe but stripped by MSYS2 bash, so also check uname.
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
  FFLAGS += -static
else ifneq (,$(findstring MINGW,$(UNAME_S)))
  FFLAGS += -static
else ifneq (,$(findstring MSYS,$(UNAME_S)))
  FFLAGS += -static
endif

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
