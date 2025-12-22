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

# Target executable
TARGET = humppa_cpp

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

# Run a test simulation
test: $(TARGET)
	@echo "Running test simulation..."
	./$(TARGET) ../test/mpar_test.txt test_output 100 3

# Install (copy to parent directory)
install: $(TARGET)
	cp $(TARGET) ../

.PHONY: all clean debug test install
