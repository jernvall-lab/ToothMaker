# Tribosphenic Tooth Model - Source Code

This directory contains the source code for the tribosphenic model.

## Directory Structure

```
tribosphenic/
├── Makefile      # Parent makefile for building both versions
├── bin/          # Output binaries
│   ├── humppa_cpp_64bit      # C++ binary
│   └── humppa_fortran_64bit  # Fortran binary
├── data/         # Interface XMLs and default parameters
└── src/
    ├── cpp/      # C++ implementation (primary)
    ├── fortran/  # Original Fortran 90 implementation
    ├── test/     # Unit tests and reference files
    └── README.md # This file
```

### cpp/

C++ translation of the original Fortran model. This is the primary implementation used for:
- WebAssembly compilation (webtooth)
- Future cross-platform builds
- Easier maintenance and debugging

See `cpp/README.md` for detailed documentation on the C++ code and parameter mappings.

### fortran/

Original Fortran 90 model code (`humppa_translate.f90`). Used for cross-validation against the C++ port. The Fortran binary stores its command-line arguments in `character*30` variables, so the input file path must be at most 30 characters long (longer paths are silently truncated and the run starts with all parameters at zero) — in practice, copy the parameter file into the working directory. It also writes a scratch file named `kk` there.

### test/

Unit test framework with reference outputs for regression testing.

**Contents:**
- `run_tests.sh` - Main test script
- `mpar_no_umgr.txt` - Test parameters (humppa format)
- `toothmaker_no_umgr.txt` - Test parameters (ToothMaker format)
- `reference/` - Reference output files for comparison

## Building

All build commands are run from the `tribosphenic/` directory (parent of `src/`).

### Build everything (C++ and Fortran)

```bash
cd models/tribosphenic
make
```

Outputs:
- `bin/humppa_cpp_64bit` - C++ binary
- `bin/humppa_fortran_64bit` - Fortran binary

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

From the `tribosphenic/` directory:

```bash
make test
```

This runs 4 tests:
1. **C++ with humppa format** - 6000 iterations, compared against a stored reference (tolerance 0.001)
2. **C++ with ToothMaker format** - same, ToothMaker-format parameter file
3. **Fortran vs C++, exact** - 10000 iterations; every value in the `.dad` must match the Fortran's
4. **Fortran vs C++, exact, all parameters non-zero** - 4 steps of 2000 iterations; steps 2 and 4 compared

Tests 3-4 require `bin/humppa_fortran_64bit` (`make fortran`) and are skipped without it.

## Running Simulations

### C++ binary

```bash
bin/humppa_cpp_64bit <parameter_file> <output_name> <iterations> <output_interval>
```

Example (from `tribosphenic/`):
```bash
bin/humppa_cpp_64bit src/test/mpar_no_umgr.txt output 6000 1
```

This produces:
- `6000_output_.dad` - Full simulation data
- `6000_output_.off` - 3D mesh for visualization
- `6000_output_.txt` - Parameter summary

### Fortran binary

```bash
cd <directory_with_param_file>
/path/to/bin/humppa_fortran_64bit <param_file> <output_name> <iterations> <output_interval>
```

Note: The Fortran binary truncates its arguments to 30 characters, so run it from the directory
holding the parameter file and pass the bare file name (see `fortran/` above).

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
