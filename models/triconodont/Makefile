# Makefile for triconodont tooth model (2010)
# Builds Fortran version into bin/

FC = gfortran
FFLAGS = -O2 -static-libgfortran -static-libgcc -Jsrc

# Windows/MinGW: fully static link so the binary doesn't need libquadmath-0.dll,
# libwinpthread-1.dll, libgcc_s_seh-1.dll alongside it.
# OS=Windows_NT is set by cmd.exe but stripped by MSYS2 bash, so also check uname.
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
  FFLAGS += -static
else ifneq (,$(findstring MINGW,$(UNAME_S)))
  FFLAGS += -static
else ifneq (,$(findstring MSYS,$(UNAME_S)))
  FFLAGS += -static
endif

# Source and target
SOURCE = src/humppa.f90

# Platform-specific binary name
ifeq ($(OS),Windows_NT)
    TARGET = bin/humppa_2010.exe
else
    TARGET = bin/humppa_2010
endif

.PHONY: all clean

# Default target
all: $(TARGET)

$(TARGET): $(SOURCE)
	@mkdir -p bin
	$(FC) $(FFLAGS) -o $@ $<
ifeq ($(shell uname),Darwin)
	# macOS: Copy libquadmath to bin/ and fix binary to use local copy
	@QUADMATH_PATH=$$(otool -L $@ | grep libquadmath | awk '{print $$1}'); \
	if [ -n "$$QUADMATH_PATH" ] && [ -f "$$QUADMATH_PATH" ]; then \
		cp -f "$$QUADMATH_PATH" bin/libquadmath.0.dylib; \
		install_name_tool -change "$$QUADMATH_PATH" @executable_path/libquadmath.0.dylib $@; \
		echo "Bundled libquadmath.0.dylib"; \
	fi
endif

clean:
	rm -f $(TARGET) bin/libquadmath.0.dylib src/*.mod
