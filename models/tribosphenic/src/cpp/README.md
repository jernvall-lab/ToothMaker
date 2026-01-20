# Tooth Development Simulation - C++ Port

This C++ implementation is a port of the original Fortran 90 code (`humppa_translate.f90`) that simulates tooth development using reaction-diffusion and mechanical models.

Translated with Claude Opus 4.5.

---

## Files

| File | Lines | Description |
|------|-------|-------------|
| `tooth_model.hpp` | 278 | Header with class declarations and constants |
| `tooth_model_core.cpp` | 361 | Constructor, initialization, grid setup |
| `tooth_model_geometry.cpp` | 236 | Cell margin/shape calculations |
| `tooth_model_diffusion.cpp` | 296 | Reaction-diffusion equations |
| `tooth_model_mechanics.cpp` | 475 | Mechanical forces (growth, buoyancy, repulsion) |
| `tooth_model_division.cpp` | 843 | Cell division algorithm |
| `tooth_model_iteration.cpp` | 36 | Main iteration loop |
| `file_io.cpp` | 663 | File I/O operations (humppa + ToothMaker formats) |
| `main.cpp` | 104 | Command-line entry point |
| `Makefile` | 70 | Build system |

---

## Architecture

The original Fortran code had two modules and a main program:

| Fortran | C++ | Purpose |
|---------|-----|---------|
| `module coreop2d` | `class ToothModel` | Main simulation model |
| `module esclec` | `class FileIO` | File I/O for parameters and morphology |
| `program tresdac` | `int main()` | Command-line entry point |

---

## Variable Name Translations

### Cell Geometry

| Fortran | C++ | Description |
|---------|-----|-------------|
| `malla` | `cellPositions` | Cell node positions (x, y, z) |
| `vei` | `neighbors` | Neighbor indices per cell |
| `marge` | `cellMargins` | Internode/margin positions |
| `knots` | `knotMarkers` | Knot markers (1=knot, 0=not) |
| `nveins` | `neighborCount` | Number of neighbors per cell |
| `hmalla` | `positionDeltas` | Position changes per iteration |

### Quantities/Concentrations

| Fortran | C++ | Description |
|---------|-----|-------------|
| `q3d` | `quantities3D` | 3D quantities [cell][z][type]: act, inh, fgf, ect, p |
| `q2d` | `quantities2D` | 2D quantities [cell][type] |
| `difq3d` | `diffusionCoeffs3D` | Diffusion coefficients (3D) |
| `difq2d` | `diffusionCoeffs2D` | Diffusion coefficients (2D) |

### Model Parameters

| Fortran | C++ | GUI Param | Description |
|---------|-----|-----------|-------------|
| `ud` | `diffThresholdSet` | Set | Growth factor threshold |
| `us` | `diffThresholdInt` | Int | Initial inhibitor threshold |
| `tacre` | `epithelialGrowthRate` | Egr | Epithelial proliferation rate |
| `tahor` | `mesenchymalGrowthRate` | Mgr | Mesenchymal proliferation rate |
| `umgr` | `basalMesenchymalRate` | uMgr | Basal Mgr (Sec-independent) |
| `acac` | `activatorAutoActivation` | Act | Activator auto-activation |
| `acec` | `ectodinRate` | Not2 | Ectodin rate (hidden) |
| `acaca` | `not3Rate` | Not3 | Not3 rate (hidden) |
| `ihac` | `activatorInhibition` | Inh | Inhibition of activator |
| `ih` | `growthFactorSecretion` | Sec | Growth factor secretion rate |
| `mu` | `degradationRate` | Deg | Protein degradation rate |
| `ina` | `initialActivator` | Ina | Initial activator concentration |
| `elas` | `stiffness` | Rep | Young's modulus / stiffness |
| `crema` | `neighborTraction` | Adh | Traction between neighbors |
| `radibi` | `nucleusTraction` | Ntr | Border-to-nucleus traction |
| `tazmax` | `sharpnessMax` | Dgr | Sharpness maxima |
| `tadi` | `borderDistance` | Swi | Border definition distance |
| `tadif` | `borderWidth` | Dff | **Differentiation rate** (see note below) |
| `difq2d(2)` | `diffusionCoeffs2D[1]` | Boy | **Buoyancy strength** (see note below) |
| `bip` | `biasPosterior` | Pbi | Posterior bias |
| `bia` | `biasAnterior` | Abi | Anterior bias |
| `bil` | `biasLingual` | Lbi | Lingual bias |
| `bib` | `biasBuccal` | Bbi | Buccal bias |
| `radibii` | `biasCenterRadius` | Bwi* | AP bias center radius (see note below) |
| `fac` | `biasFactor` | Bgr | Bias factor |

#### Parameter Naming Confusion (IMPORTANT)

The Fortran variable names are historically misleading and do not reflect actual functionality:

1. **Boy (Buoyancy)**: The GUI parameter "Boy" controls `difq2d(2)` / `diffusionCoeffs2D[1]`.
   Despite being named as a "diffusion coefficient", this variable is used in `stelate()` /
   `calculateBuoyancy()` to control the stellate reticulum / buoyancy effect.

2. **Dff (Differentiation)**: The GUI parameter "Dff" controls `tadif` / `borderWidth`.
   Despite being named "border width" in the Fortran code, this variable is used in
   `diferenciacio()` / `updateDifferentiation()` to control differentiation rate.

3. **Bwi (Border Width)***: The GUI parameter "Bwi" at XML position 28 actually controls
   `radibii` / `biasCenterRadius` (the radius where AP bias is applied), NOT border width.
   This appears to be a historical mapping error in the XML interface definition.
   The actual "border width" functionality is controlled by `tadi` / `borderDistance` (Swi).

### Cell Counts

| Fortran | C++ | Description |
|---------|-----|-------------|
| `ncels` | `numCells` | Number of actual cells |
| `ncals` | `numCellsTotal` | Total cells (including virtual boundary) |
| `ncz` | `numZLevels` | Number of z-depth levels |
| `ncils` | `numBorderCells` | Number of border cells (see note below) |
| `radi` | `radius` | Grid radius |
| `temps` | `timeStep` | Current time step |

**Note on `numBorderCells`**: Fortran initializes `ncils = (radi-1)*6 + 1`, while C++ uses `numBorderCells = (radius-1)*6` (without the +1). This is an intentional difference for 0-based indexing semantics. As a result, C++ `numBorderCells` will always be 1 less than Fortran `ncils`. This does not affect the simulation results - the cell connectivity and positions remain identical.

---

## Function Name Translations

### Initialization

| Fortran | C++ | Description |
|---------|-----|-------------|
| `ciinicial` | `initializeDefaults` | Set default initial conditions |
| `ci` | `reinitialize` | Reinitialize model |
| `dime` | `allocateAndInit` | Allocate and initialize matrices |
| `redime` | `reallocate` | Reallocate matrices |
| `referci` | `reset` | Reset and reinitialize |
| `refercid` | `deallocateAll` | Deallocate all memory |
| `posar` | `placeCell` | Position cell in initial grid |
| `initact` | `initializeActivator` | Set initial activator concentration |

### Simulation

| Fortran | C++ | Description |
|---------|-----|-------------|
| `iteracio` | `runIteration` | Main iteration loop |
| `reaccio_difusio` | `reactionDiffusion` | Reaction-diffusion calculations |
| `diferenciacio` | `updateDifferentiation` | Update differentiation values |
| `calculmarges` | `calculateMargins` | Calculate cell margins/shapes |
| `empu` | `calculateGrowthPushing` | Calculate growth-induced pushing |
| `stelate` | `calculateBuoyancy` | Calculate buoyancy/stellate effect |
| `pushing` | `calculateNeighborRepulsion` | Repulsion between neighbors |
| `pushingnovei` | `checkNonNeighborRepulsion` | Repulsion for non-neighbors |
| `biaixbl` | `applyBuccalLingualBias` | Apply BMP4 bias at borders |
| `promig` | `calculateNucleusTraction` | Nucleus traction by borders |
| `actualitza` | `updatePositions` | Update cell positions |
| `afegircel` | `addNewCells` | Add cells where division occurs |
| `perextrems` | `markBorderCells` | Mark new border cells |
| `controlz` | `increaseZDepth` | Increase z-depth |

### File I/O

| Fortran | C++ | Description |
|---------|-----|-------------|
| `guardapara` | `saveParameters` | Save parameters |
| `guardaforma` | `saveMorphology` | Save cell positions |
| `guardacon` | `saveConcentrations` | Save 3D quantities |
| `guardaex` | `saveExtraData` | Save 2D quantities |
| `guardaknots` | `saveKnots` | Save knot markers |
| `guardaveins` | `saveNeighbors` | Save neighbor data |
| `guardamarges` | `saveMargins` | Save margin data |
| `guardaveinsoff` | `saveAsOFF` | Save as OFF mesh format |
| `llegirparatxt` | `readParametersText` | Read parameters (humppa format) |
| - | `readParametersToothMaker` | Read parameters (ToothMaker format) |
| - | `isToothMakerFormat` | Auto-detect parameter file format |
| - | `getParameterIndex` | Map parameter names to indices |
| `escriuparatxt` | `writeParametersText` | Write parameters (text) |
| `llegirpara` | `readParametersBinary` | Read parameters (binary) |
| `llegirforma` | `readMorphology` | Read cell positions |
| `llegirex` | `readExtraData` | Read extra data |
| `llegirknots` | `readKnots` | Read knot markers |
| `llegirveins` | `readNeighbors` | Read neighbor data |
| `agafarparap` | `storeParameters` | Copy model params to storage |
| `posarparap` | `loadParameters` | Copy storage params to model |
| `llegir` | `readDataFile` | Main read function |
| `llegirinicial` | `readInitialParameters` | Read initial parameters |
| `mat` | `calculateMaterial` | Calculate material values |
| `get_rainbow` | `getColorMapping` | Map value to color |

---

## Constants

| Fortran | C++ | Value | Description |
|---------|-----|-------|-------------|
| `nvmax` | `MAX_NEIGHBORS` | 30 | Maximum neighbors per cell |
| `ng` | `NUM_3D_QUANTITIES` | 5 | Number of 3D quantity types |
| `ngg` | `NUM_2D_QUANTITIES` | 4 | Number of 2D quantity types |
| `la` | `BASE_DISTANCE` | 1.0 | Original inter-node distance |
| `dmax` | `MAX_DIVISION_DIST` | 2.0 | Distance triggering division |
| `delta` | `TIME_DELTA` | 0.05 | Time step size (Fortran: 0.005D1) |
| `pii` | `PI` | 3.14159... | Pi constant |

---

## Key Translation Notes

### 1. Index Conversion (Fortran 1-based to C++ 0-based)

The most critical aspect of this translation is the conversion from Fortran's 1-based indexing to C++'s 0-based indexing.

**Key conventions:**
- Internal arrays use 0-based indexing
- `neighbors` array stores **1-based indices** to maintain file compatibility
- When accessing a neighbor: `int neighborIdx = neighbors[i][j] - 1;`
- Boundary cell index: `numCellsTotal` (1-based) = virtual boundary sentinel
- File I/O preserves 1-based format for compatibility with existing .dad files

**Example pattern used throughout:**
```cpp
for (int j = 0; j < MAX_NEIGHBORS; j++) {
    int k = neighbors[i][j];      // 1-based from array
    if (k != 0 && k <= numCells) {
        k--;                       // Convert to 0-based for array access
        // Use cellPositions[k], quantities3D[k], etc.
    }
}
```

### 2. Parameter File Reading Index

The original Fortran reads parameters starting at array index 3:
```fortran
do i=3,32
  read (2,*,END=666,ERR=777)  a,cara ; parap(i,map)=a
end do
```
The C++ port matches this exactly. Parameter positions (especially `radius` at index 26) must align correctly for file compatibility.

### 3. Parameter Array Index Overlap

In `storeParameters()` and `loadParameters()`, the original Fortran code has overlapping array indices:
- Index 11 is used for both `not3Rate` and `numCellsTotal`
- Index assignments are not sequential

This behavior is preserved for compatibility but may indicate a bug in the original code.

### 4. Goto Statement Conversion

The original Fortran heavily uses `goto` for control flow. These have been converted to:
- `break` and `continue` statements
- Labeled statement blocks with `goto next_label;` where necessary
- Helper flag variables

### 5. Fortran Scientific Notation

Fortran uses `0.044D1` format which equals `0.44` (the D1 means ×10^1). Several magic numbers in the diffusion code use this:
- `0.044D1` = 0.44 (boundary sink term coefficient)
- `0.005D1` = 0.05 (TIME_DELTA)
- `0.05D1` = 0.5 (triangle area factor)

### 6. OFF File Output

The C++ `saveAsOFF()` function produces different output than the original Fortran `guardaveinsoff()`. The original Fortran-derived implementation had two problems:

1. **Missing polygons**: The original algorithm left holes in the mesh surface
2. **Non-oriented triangles**: Triangles had inconsistent winding order, causing rendering artifacts (back-face culling issues, incorrect lighting)

The C++ version uses the `dad_to_polygons` algorithm instead, which fixes these issues. As a consequence, **each triangle is written twice with both orientations**. This doubles the polygon count but ensures correct rendering regardless of viewer orientation, since recovering correctly oriented triangles from the original data structure is non-trivial.

#### Color Mapping Change

The C++ `getColorMapping()` outputs colors that match what `dad_to_polygons` produces, rather than the original Fortran colors:

| Cell State | Original Fortran | C++ / dad_to_polygons |
|------------|------------------|----------------------|
| Undifferentiated | Gray (0.6, 0.6, 0.6) | Gray (0.5, 0.5, 0.5) |
| Differentiated | Red/orange gradient | White (1.0, 1.0, 1.0) |
| Knots | Yellow (1.0, 1.0, 0.0) | Yellow (1.0, 1.0, 0.0) |

The original Fortran used alpha values to encode differentiation state (alpha 0.5 for differentiated, 0.8 for undifferentiated, 1.0 for knots), which `dad_to_polygons` and `binaryhandler` would detect and recolor. The C++ version outputs the final colors directly with alpha 1.0 for all cells, eliminating the need for post-processing by `dad_to_polygons`.

### 7. DAD File Import (Not Implemented)

The original Fortran code has a `llegir` subroutine that reads complete `.dad` files to restore simulation state (parameters, cell positions, neighbors, knots). This was designed for GUI use - loading saved simulations for visualization or continuing a run.

The C++ port includes translated versions of these functions (`readDataFile()`, `readMorphology()`, `readNeighbors()`, `readKnots()`, `readExtraData()`, `readParametersBinary()`), but they are **not called** from the CLI and have **not been tested**. The CLI only reads initial parameters and runs simulations from scratch.

If DAD file import is needed in the future, these functions would require:
- Testing with actual `.dad` files
- Array bounds validation (see Potential Issues below)
- Proper error handling

---

## Potential Issues in Current C++ Code

### Critical Issues

1. **File read array bounds** (`file_io.cpp`): `readMorphology()` reads `numCells` from the file, then uses it to index `cellPositions` without checking array capacity. A corrupted or malicious file specifying large `numCells` causes out-of-bounds memory access and undefined behavior. **Note:** This is part of the unimplemented DAD file import feature (see section 7) and is not currently reachable from the CLI.

2. **Neighbor array bounds** (`tooth_model_division.cpp`): The `addNewCells()` function has complex neighbor list manipulation. While some overflow conditions trigger `panicFlag`, not all insertion paths are protected. Rapid cell division with unusual geometry could write beyond `MAX_NEIGHBORS`.

3. **Array resize during iteration** (`tooth_model_division.cpp`): When cells divide, arrays are resized with `std::move()`. Any cached indices or pointers become invalid. The code correctly recomputes indices, but this pattern is error-prone for future modifications.

### Moderate Issues

1. **Floating-point precision in neighbor detection** (`tooth_model_core.cpp:318-319`): Neighbor matching uses `std::round(1000000 * position)` for comparison. This may miss matches or create false positives near the precision boundary (around 1e-6).

2. **Hard-coded magic numbers**: Many coefficients like `0.44` (boundary sink), `0.85` (margin scale), `1.4` (proximity threshold) are scattered throughout without clear documentation of their physical meaning or derivation.

3. **Goto statements preserved** (`tooth_model_geometry.cpp`, `tooth_model_diffusion.cpp`): While functional, the remaining `goto` statements make control flow harder to reason about and could mask bugs during future maintenance.

### Minor Issues

1. **Memory allocation pattern**: Arrays are resized incrementally rather than reserved with estimated capacity. For simulations with many divisions, this causes repeated reallocations and memory fragmentation.

2. **Redundant array clearing**: Several functions clear arrays that were just allocated with default values (e.g., `fill(0.0)` after `resize(n, {0.0, ...})`). Minor performance overhead but indicates code that could be simplified.

---

## Removed Unused Code

The following dead code was identified and removed during cleanup:

### Global Variable
- `arrowKeyFunction` - declared but never read

### ToothModel Class Members
- `velocityDeltas` - set but never read
- `vizPosX`, `vizPosY`, `vizPosZ` - GUI visualization arrays, never used in CLI
- `rdDeltaQ2D` - allocated but never used for computation
- `amplitude`, `minGrowthConcentration` - set but never read
- `inverseStiffness`, `maxCellCapacity` - computed/declared but never used
- `focusCell` - set but never read
- `showLines`, `showRender`, `showMargins`, `showVectors` - GUI flags
- `showVectorX`, `showVectorK`, `showExtended`, `showNormals` - GUI flags
- `colorMode`, `displayMode`, `displayModeAlt`, `subMenuId` - GUI state
- `currentLevel`, `levelOffset` - GUI state

### FileIO Class Members
- `inputFormFile`, `inputNeighborFile`, `inputParamFile` - set but never read
- `outputFormFile`, `outputNeighborFile`, `outputParamFile` - set but never read
- `fileStatus`, `totalSnapshots`, `errorStatus` - initialized but never used
- `passFlag2` - dead code branch (condition never true)
- `maxSnapshots` - set but never read
- `argSteps`, `argParam`, `argPercent` - never used

### Removed Functions
- `readParameters()` - unused initialization function
- `sortByMagnitude()` - unused sorting helper
- `enforceNeighborSymmetry()` - unused debug function
- `checkSymmetry()` - unused debug function

### Removed Constants
- `MIN_VELOCITY` - defined but never referenced

---

## Improvement Suggestions

### Low Effort (Quick wins)

1. **Validate file data before use** (`file_io.cpp`)
   - Check `if (numCells > cellPositions.size()) { resize or error }`
   - Prevents crashes from corrupted input files
   - ~10 lines of code

2. **Replace magic numbers with named constants**
   - Define `BOUNDARY_SINK_COEFF = 0.44`, `PROXIMITY_THRESHOLD = 1.4`, etc.
   - Add to constants section in header
   - Improves readability and maintainability

3. **Add debug assertions for array bounds**
   - Use `assert(index < size)` in debug builds
   - Catches indexing errors during development
   - Zero runtime cost in release builds

### Medium Effort (Targeted improvements)

1. **Use spatial hashing for non-neighbor repulsion**
   - Replace O(n²) pairwise check with grid-based spatial hash
   - Cells only check neighbors in nearby grid cells
   - ~100 lines, potential further speedup for very large simulations
   - Note: `checkNonNeighborRepulsion()` has already been optimized with symmetric
     early-exit checks, hash set for neighbor lookup, squared distance comparison,
     and direct force accumulation (see C++ Optimizations section below)

2. **Pre-allocate arrays with reserve()**
   - Estimate maximum cell count: `reserve(initialCells * 4)`
   - Reduces memory fragmentation and reallocation overhead
   - ~20 lines across initialization functions

3. **Add input validation and error handling**
   - Validate parameter ranges in `loadParameters()`
   - Check file format version in `readDataFile()`
   - Return error codes instead of silent failures
   - ~50-100 lines

### High Effort (Architectural changes)

1. **Parallelize with OpenMP**
   - Most loops in mechanics and diffusion are embarrassingly parallel
   - Add `#pragma omp parallel for` to main computation loops
   - Requires careful handling of shared state in `positionDeltas`
   - ~200 lines, potential 4-8x speedup on multi-core

2. **Separate simulation state from I/O**
   - Create `SimulationState` class for pure data
   - Make `ToothModel` stateless computation engine
   - `FileIO` operates on state objects
   - Enables cleaner testing, serialization, and checkpointing
   - Major refactor, ~500+ lines

3. **Implement adaptive time stepping**
   - Monitor maximum position/concentration change per step
   - Reduce `TIME_DELTA` when changes are large (numerical stability)
   - Increase `TIME_DELTA` when changes are small (performance)
   - Requires convergence analysis, ~300 lines

---

## C++ Optimizations vs. Fortran

The C++ port includes several performance optimizations not present in the original Fortran code:

### `checkNonNeighborRepulsion()` (tooth_model_mechanics.cpp)

This function checks if non-neighboring cells get too close and applies repulsion. It's the most expensive function in the simulation loop. The following optimizations were applied:

1. **Symmetric early-exit checks**: The original Fortran only checked positive direction (`ux > 1.4`), missing early exit opportunities when cells are far apart in the negative direction. Changed to `ux > 1.4 || ux < -1.4` to skip ~50% more distant pairs.

2. **Hash set for neighbor lookup**: Replaced O(30) linear scan through neighbor array with O(1) `std::unordered_set` lookup for each cell pair.

3. **Squared distance comparison**: Compare `dSq < 1.96` before computing `sqrt()`. Only compute the square root when actually needed for force calculation.

4. **Direct force accumulation**: Eliminated intermediate `pushForce` array. Forces are accumulated directly into `sumX/sumY/sumZ`, improving cache locality and removing allocation overhead.

5. **Named constants**: Replaced magic number `1.4` with `distThreshold` and `distThresholdSq` for clarity.

These optimizations maintain numerical compatibility with the Fortran output (within floating-point tolerance) while improving performance.

---

## Building

```bash
make            # Optimized build
make debug      # Debug build with symbols
make clean      # Remove build artifacts
```

Or manually:
```bash
g++ -std=c++17 -O3 -march=native -flto -ffast-math -o humppa_cpp *.cpp
```

---

## Usage

```bash
./humppa_cpp <input_file> <output_file> <iterations> <steps>
```

**Example:**
```bash
./humppa_cpp params.txt output 5000 1
```

This runs 1 step of 5000 iterations, producing:
- `5000_output_.dad` - Full simulation data
- `5000_output_.off` - Mesh in OFF format (for visualization)
- `5000_output_.txt` - Human-readable parameters

### Parameter File Formats

The program auto-detects the input format:

**Humppa format** (original):
```
0.0255 Egr
200 Mgr
1 Rep
...
```

**ToothMaker format** (GUI export):
```
model==Tribosphenic tooth
Egr==0.0255
Mgr==200
Rep==1
...
```

---

## Testing

Run the unit test suite from the parent directory:
```bash
cd ..
make test
```

This runs 3 tests (with Test 3 having two sub-parts):
1. **C++ with humppa format** - Compares output against reference
2. **C++ with ToothMaker format** - Compares output against reference
3. **Fortran vs C++ cross-validation**:
   - 3a: Connectivity comparison (exact match)
   - 3b: Cell shapes comparison (tolerance 0.001)

Test files are in `../test/`:
- `mpar_no_umgr.txt` - Test parameters (humppa format)
- `toothmaker_no_umgr.txt` - Test parameters (ToothMaker format)
- `reference/` - Reference output files for comparison

See `../README.md` for more details.
