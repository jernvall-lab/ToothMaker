# Makefile for humppa_cpp tooth simulation
# Builds the modular C++ version of the tooth development model

CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -ffast-math -funroll-loops -DNDEBUG
LDFLAGS = -flto

# Debug build flags (use: make debug)
DEBUG_FLAGS = -std=c++17 -g -O0 -Wall -Wextra -DDEBUG

# Source files
SOURCES = main.cpp \
          tooth_model_core.cpp \
          tooth_model_geometry.cpp \
          tooth_model_diffusion.cpp \
          tooth_model_mechanics.cpp \
          tooth_model_division.cpp \
          tooth_model_iteration.cpp \
          file_io.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Header dependencies
HEADERS = tooth_model.hpp

# Target executable (output to bin/ directory)
TARGET = ../../bin/humppa_cpp_64bit

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Compile
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Debug build
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: LDFLAGS =
debug: clean $(TARGET)

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run unit tests (compare output against reference files)
test: $(TARGET)
	@../test/run_tests.sh

# Regenerate reference files (use when intentionally changing output)
test-update-reference: $(TARGET)
	@echo "Regenerating reference files..."
	@mkdir -p ../test/reference
	@cd ../test && rm -rf tmp_ref && mkdir tmp_ref && cd tmp_ref && \
		../../../bin/humppa_cpp_64bit ../mpar_1767210438.txt output.dad 900 10 >/dev/null && \
		cp 9000_output.dad_.dad ../reference/9000_humppa.dad && \
		../../../bin/humppa_cpp_64bit ../toothmaker_1767210438.txt output.dad 900 10 >/dev/null && \
		cp 9000_output.dad_.dad ../reference/9000_toothmaker.dad && \
		cd .. && rm -rf tmp_ref
	@echo "Reference files updated."

# Install target (binary already built to bin/, kept for compatibility)
install: $(TARGET)
	@echo "Binary installed to ../../bin/humppa_cpp_64bit"

.PHONY: all clean debug test test-update-reference install
