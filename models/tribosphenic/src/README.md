# Tribosphenic Tooth Model - Source Code

This directory contains the source code for the tribosphenic model.

## Directory Structure

```
src/
├── cpp/          # C++ implementation (primary)
├── fortran/      # Original Fortran 90 implementation
├── test/         # Unit tests and reference files
├── Makefile      # Parent makefile for building both versions
└── README.md     # This file
```

### cpp/

C++ translation of the original Fortran model. This is the primary implementation used for:
- WebAssembly compilation (webtooth)
- Future cross-platform builds
- Easier maintenance and debugging

See `cpp/README.md` for detailed documentation on the C++ code and parameter mappings.

### fortran/

Original Fortran 90 model code (`humppa_translate.f90`). Used for cross-validation against the C++ port. The Fortran binary requires input files to be in the current working directory.

### test/

Unit test framework with reference outputs for regression testing.

**Contents:**
- `run_tests.sh` - Main test script
- `mpar_no_umgr.txt` - Test parameters (humppa format)
- `toothmaker_no_umgr.txt` - Test parameters (ToothMaker format)
- `reference/` - Reference output files for comparison

## Building

### Build everything (C++ and Fortran)

```bash
make
```

### Build C++ only

```bash
make cpp
```

### Build Fortran only

Requires gfortran:

```bash
make fortran
```

### Clean build artifacts

```bash
make clean
```

## Running Tests

```bash
make test
```

This runs 4 tests:
1. **C++ with humppa format** - Compares output against reference
2. **C++ with ToothMaker format** - Compares output against reference
3. **Fortran vs C++ connectivity** - Verifies identical cell connectivity
4. **Fortran vs C++ cell shapes** - Verifies geometry within tolerance (0.001)

## Running Simulations

### C++ binary

```bash
./cpp/humppa_cpp <parameter_file> <output_name> <iterations> <output_interval>
```

Example:
```bash
./cpp/humppa_cpp test/mpar_no_umgr.txt output.dad 6000 1
```

This produces:
- `6000_output.dad_.dad` - Full simulation data
- `6000_output.dad_.off` - 3D mesh for visualization
- `6000_output.dad_.txt` - Parameter summary

### Fortran binary

```bash
cd <directory_with_param_file>
../fortran/humppa <param_file> <output_name> <iterations> <output_interval>
```

Note: The Fortran binary requires the parameter file to be in the current working directory.

## Parameter File Formats

The C++ implementation auto-detects the parameter file format:

### Humppa format (original)
```
0.0255 Egr
200 Mgr
1 Rep
...
```

### ToothMaker format
```
model==Tribosphenic tooth
Egr==0.0255
Mgr==200
Rep==1
...
```
