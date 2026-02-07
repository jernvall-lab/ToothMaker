# Command-Line Parameter Scanning

ToothMaker can run parameter scans from the command line without the GUI.
This produces simulation output, rendered screenshots, and cusp analysis
results for each parameter combination.

## Quick Start

```bash
ToothMaker --niter 9000 --param seal_triconodont.txt --scan scanlist.txt
```

### Arguments

| Argument | Description |
|----------|-------------|
| `--niter N` | Number of simulation iterations |
| `--param FILE` | Model parameters file |
| `--scan FILE` | Scan list defining parameter ranges |
| `--res N` | Image resolution width/height in pixels (default: 495) |

### Input Files

**Parameter file** (e.g. `seal_triconodont.txt`, `seal_tribosphenic.txt`):
Defines the base model and default parameter values. The `model==` line
selects which model binary to run. Parameters use `name==value` format.

Two example parameter files are included:
- `seal_triconodont.txt` -- Triconodont model (Fortran, `humppa_2010`)
- `seal_tribosphenic.txt` -- Tribosphenic model (C++, `humppa_cpp_64bit`)

**Scan list** (`scanlist.txt`): Defines which parameters to vary and their
ranges. Each scanned parameter uses `name==min:step:max` format. All
combinations are generated (full factorial scan).

```
ViewMode==1
orientation==Occlusal,Buccal
Act==0.7:0.4:1.5
Inh==20:6:26
```

This example scans Act (3 values) x Inh (2 values) = 6 combinations.

### Output

| File/Directory | Description |
|----------------|-------------|
| `job_parameters.txt` | Lists all parameter combinations generated |
| `data/XX/` | Simulation output per parameter combination (XX = grid ID) |
| `screenshots/` | Rendered images at requested orientations |
| `top_cusp_angles.txt` | Cusp angle measurements for each combination |
| `local_maxima.txt` | Local maxima coordinates |
| `cuspA_baseline.txt` | Main cusp baseline coordinates |

Grid IDs encode the position in the scan matrix: the first digit is the
index into the first scanned parameter, the second digit is the index into
the second, etc. For example, with Act (3 values) x Inh (2 values):

| ID | Act | Inh |
|----|-----|-----|
| 00 | 0.7 | 20 |
| 10 | 1.1 | 20 |
| 20 | 1.5 | 20 |
| 01 | 0.7 | 26 |
| 11 | 1.1 | 26 |
| 21 | 1.5 | 26 |

## Running Tests

The test script runs parameter scans for both the Triconodont and
Tribosphenic models and compares output against reference data (12 tests
total, 6 per model).

From the build directory:

```bash
make test
```

From this directory:

```bash
make test
```

Or directly:

```bash
./run_tests.sh [path-to-ToothMaker-binary]
```

If no binary path is given, the script looks for it in the default build
location (`../../build/`). The test runs 12 simulations (6 per model, 9000
iterations each) and takes about 30 seconds.
