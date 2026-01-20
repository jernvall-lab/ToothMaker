# Makefile for triconodont tooth model (2010)
# Builds Fortran version into bin/

FC = gfortran
FFLAGS = -O2 -static-libgfortran -static-libgcc -Jsrc

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
