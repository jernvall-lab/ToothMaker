# Tooth Development Simulation - C++ Port

This C++ implementation is a port of the original Fortran 90 code (`humppa_translate.f90`) that simulates tooth development using reaction-diffusion and mechanical models.

Translated with Claude Opus 4.5.

---

## Files

| File | Lines | Description |
|------|-------|-------------|
| `tooth_model.hpp` | 267 | Header with class declarations and constants |
| `tooth_model_core.cpp` | 168 | Constructor, initialization, grid setup |
| `tooth_model_geometry.cpp` | 270 | Cell margins (recorded mesh walk), triangle mesh |
| `tooth_model_diffusion.cpp` | 262 | Reaction-diffusion equations |
| `tooth_model_mechanics.cpp` | 520 | Mechanical forces (growth, buoyancy, repulsion) |
| `tooth_model_division.cpp` | 450 | Cell division algorithm |
| `tooth_model_iteration.cpp` | 38 | Main iteration loop |
| `file_io.cpp` | 541 | Parameter input, .dad/.off/.txt output (humppa + ToothMaker formats) |
| `main.cpp` | 128 | Command-line entry point |
| `Makefile` | 109 | Build system |

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
| `nveins` | — | Number of neighbors per cell (maintained but never read; dropped in the C++) |
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
| `bip` | `biasPosterior` | Abi* | Bias on the x < 0 side (see note below) |
| `bia` | `biasAnterior` | Pbi* | Bias on the x > 0 side (see note below) |
| `bil` | `biasLingual` | Bbi* | Activator floor on the y < −Swi side (see note below) |
| `bib` | `biasBuccal` | Lbi* | Activator floor on the y > Swi side (see note below) |
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

4. **Abi/Pbi and Bbi/Lbi***: The four bias variables are named after the Fortran (`bip`, `bia`,
   `bil`, `bib`), but the GUI passes parameters positionally and `Abi` lands in `bip` /
   `biasPosterior`, `Pbi` in `bia` / `biasAnterior`, `Lbi` in `bib` / `biasBuccal` and `Bbi` in
   `bil` / `biasLingual` — the Fortran source says so itself (`NOTE: Abi, Pbi, Bbi, Lbi`). The
   GUI, the Fortran and the C++ therefore agree with each other; only the variable names are
   misleading. The names and the order are kept because existing users and parameter files rely
   on them. Only the name-based reader for ToothMaker-format files disagrees (Known Issues, B1).

### Cell Counts

| Fortran | C++ | Description |
|---------|-----|-------------|
| `ncels` | `numCells` | Number of actual cells |
| `ncals` | `numCellsTotal` | Total cells (including virtual boundary) |
| `ncz` | `NUM_Z_LEVELS` | Number of z-depth levels (constant 4; `controlz` is never called) |
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
| `dime` | `allocateAndInit` | Allocate and initialize matrices |
| `posar` | `placeCell` | Position cell in initial grid |
| `initact` | `initializeActivator` | Set initial activator concentration |

`ci`, `redime`, `referci` and `refercid` (re-initialisation for the Fortran GUI) have no counterpart.

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

`perextrems` (maintains the `mmaa`/`mmap` marker arrays, which nothing reads) and `controlz` (never
called) are not ported.

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
| `agafarparap` | `storeParameters` | Copy model params to storage |
| `posarparap` | `loadParameters` | Copy storage params to model |
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

This behavior is preserved for compatibility. The index-11 overlap is a genuine bug in the original
(see *Known Issues*, F2): after the first output step `Not3` holds the cell count. The other
overlaps are harmless, because the clobbered slots (`difq3d(5)`, `difq2d(1,3,4)`) are never used.

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

### 7. DAD File Import (Not Ported)

The original Fortran has a `llegir` subroutine (with `llegirpara`, `llegirforma`, `llegirex`,
`llegirknots`, `llegirveins`) that reads complete `.dad` files to restore a simulation state for its
GUI. The C++ port carried untested, unreachable translations of these routines until 2026-09, when
they were removed; they indexed arrays with counts read from the file, unchecked. The CLI reads a
parameter file and runs from scratch, and ToothMaker's GUI reads `.dad` files with its own parser
(`common/readdata.cpp`).

---

## Known Issues

Status after a line-by-line audit against `humppa_translate.f90` (2026-09), backed by instrumented
builds, 500 randomised parameter sets and sanitizer/hardened builds.

**Fidelity in one sentence:** since 2026-09-22 (D1 fixed, `-ffp-contract=off` like the Fortran
build) the production binary reproduces the Fortran *bit for bit* — on macOS and on Linux, and the
two platforms agree with each other (unit tests, 2026-09-22) — positions,
margins, concentrations, neighbour lists, knots — through at least 20 000 iterations, including
runs with the hidden channels switched on and 20-step runs, verified value for value against the
shipped `humppa_fortran_64bit`. Both write 17 significant digits but format them differently, so
compare parsed values, as `test/run_tests.sh` does.

### Bugs in the C++ port

- **B1. `Abi`/`Pbi` and `Lbi`/`Bbi` are swapped in the ToothMaker-format reader**
  (`getParameterIndex()`, `file_io.cpp`). The GUI passes parameters positionally in XML order
  (`… Bgr, Abi, Pbi, Lbi, Bbi, Rad …`), which feeds GUI `Abi` into Fortran `bip`, `Pbi` into `bia`,
  `Lbi` into `bib` and `Bbi` into `bil`. The name-based reader follows the (misleading) Fortran
  variable names instead, so a ToothMaker-format file given directly to `humppa_cpp` produces a
  different tooth than the same file run through the GUI. GUI, `examples/cli` and webtooth are
  positional and unaffected. This is also why the two unit-test references differ. *Decision
  2026-09-22: kept for now, with the mapping documented in the code, so that nothing changes for
  existing users.*
- **B2.** *(fixed 2026-09-22)* The `.dad` body was written with 6 decimals — `saveParameters()`
  set `std::fixed << setprecision(6)` for the header and the stream kept it — and the `.off` and
  `.txt` with the default 6 significant digits, against 17 in the Fortran. All three now carry 17
  significant digits (`max_digits10`); only the `5F15.6` parameter header keeps its fixed format,
  as in the Fortran.
- **B3. Argument parsing** (`main.cpp`): a non-numeric iteration or step count throws out of
  `std::stoi` uncaught; a negative step count runs `abs(steps)` steps (Fortran: none).

### Bugs inherited from the Fortran original (present in both)

- **F1.** *(guarded 2026-09-22)* Neighbour lists have a fixed capacity of `MAX_NEIGHBORS` = 30
  (Fortran `nvmax`, a fixed array dimension with no physical meaning; a healthy cell has 6–13).
  In `addNewCells()` the Fortran inserts into a full list without a check: it drops the last
  entry or writes past the array. The C++ now stops instead: `insertBetween` and the new cell's
  own list report over-capacity, `panic()` records the message, and the CLI exits with code 2
  (`Error: cell N would have more than 30 neighbours …`). Only reachable in a degenerate mesh
  (parameter scans far outside the biological range). AddressSanitizer alone does not see the
  original overrun, which stays inside the `std::vector` allocation; libc++'s
  `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG` does.
- **F2. `Not3` is overwritten by the cell count after the first output step.** `storeParameters()`
  puts `not3Rate` and then `numCellsTotal` into index 11, and `loadParameters()`, called after
  every step's output, reads `not3Rate` back from it. With `Not2 ≠ 0` a run split into steps
  therefore differs from the same run in one step, and the `.txt` echo always reports
  `Not3` = cell count. Masked in normal use because `Not2` is hidden and zero.
- **F3.** *(resolved 2026-09-22)* In the Fortran a division "panic" (ring walk longer than
  `nvmax`) abandons that iteration's divisions and the run continues; the offending pair is still
  too far apart next time, so every later iteration repeats the failed attempt and the cell count
  freezes. Seen in ~7% of parameter sets perturbed by up to 2x. The C++ now treats every panic —
  ring walk, list capacity (F1), numerical overflow — as fatal: the run stops with a message and
  exit code 2, leaving the files of the completed steps.
- **F4. (Fortran only.)** `cmarge(i,kk,4:8)=…` in `afegircel` uses the ordinal of the new cell as a
  cell index and corrupts the rest lengths of an unrelated cell. The C++ shifts the new cell's own
  margins instead, i.e. it does *not* reproduce this. Never reached with the stock parameter
  sets; reached in ~3% of otherwise healthy randomised runs.

### Deviations from the Fortran

- **D1.** *(fixed 2026-09-22)* `calculateBuoyancy()` evaluated `ax*(dd*a)` where the Fortran
  `ax=ax*dd*a` evaluates `(ax*dd)*a`. It was the only arithmetic deviation in normally exercised
  code; the C++ now multiplies left to right.
- **D2.** *(resolved 2026-09-22)* The Makefile used `-ffp-contract=on` while the Fortran is built
  with `off`; FMA contraction made the two outputs differ from iteration 2 onward and diverge
  around iteration 6300. The C++ is now built with `-ffp-contract=off` too (about 3% slower).
- **D3.** Non-neighbour range test uses `d² < 1.4*1.4` instead of `sqrt(d²) < 1.4`; the two disagree
  for exactly one representable value of `d²`.
- **D4.** Paths that only execute once the mesh has already degenerated differ in detail: the
  `kl > 100` fallback in `calculateMargins()`, zero-initialised border-cell locals in
  `calculateGrowthPushing()` where the Fortran keeps stale values, NaN guards (`suma > 0`,
  `n > 0`, `dist > 0`) the Fortran lacks, and a duplicate-pair check and index guards in
  `addNewCells()`. None of them executes with the mouse, seal or test parameters.
- **D5.** Output naming (no underscore padding), no `kk` scratch file, header line 7 of the `.dad`
  holds `numBorderCells` (= Fortran `ncils − 1`), OFF content (section 6), number formatting
  (C++ `%g` style versus Fortran list-directed, same 17 significant digits).

### Minor

1. **Duplicated parameter tables.** `getParameterIndex()` and the default `parameterNames` list in
   `readParametersToothMaker()` spell out the parameter order twice; one table in GUI order would
   replace both, and is where B1 would be fixed.
2. **Hard-coded magic numbers**: `0.44` (boundary sink), `1.4` (non-neighbour range), `1e-4`
   (z gap in growth pushing), `-0.3` (unset-angle sentinel) and the `1e-15`/`1e-13`/`1e-8` snap
   thresholds are undocumented.
3. **Goto statements preserved** in `calculateMargins()` (two labels) and `reactionDiffusion()`
   (one).
4. **Memory layout**: `quantities3D` and `rdDeltaQ3D` are vectors of vectors (one heap block per
   cell, although the depth is always `NUM_Z_LEVELS`), and every division event rebuilds and copies
   every per-cell array.
5. **Redundant clearing**: `calculateMargins()` zeroes 30 margin slots per cell per iteration
   although only slot 0 can ever be read while empty.
6. `static` scratch buffers in `checkNonNeighborRepulsion()` are shared between `ToothModel`
   instances; the spatial grid size is an unchecked `int` product of the bounding-box extents.

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

### 2026-09 dead-code removal (3546 → 2543 lines; output byte-identical on 500 parameter sets)

- `markBorderCells()` with `anteriorMarkers`/`posteriorMarkers` and their counts (maintained, never
  read; the Fortran `perextrems`/`mmaa`/`mmap` are dead in the same way)
- `reinitialize()`, `reallocate()`, `reset()`, `deallocateAll()`, `increaseZDepth()`
- `neighborCount` (maintained in six places, never read), `centerCell`, `stepNumber`; `numNewCells`
  became a local
- Margin components `[3]`, `[5]`, `[6]`, `[7]` (written and shifted, never read): a slot is now
  `{x, y, z, restLength}` (`MarginSlot`, `MARGIN_REST`)
- Initialisation temporaries held as members (`currX/currY/newX/newY`, `currentCellIndex`,
  `centerCellIndex`, twelve `sin/cosDir`); `placeCell()` takes them as arguments
- `FileIO`: the `.dad` import path (`readDataFile()` and five readers), the 5000-snapshot
  `parameterHistory`/`positionHistory`/`knotsHistory`/`neighborHistory` (a single 32-entry
  `parameters` array remains), `snapshotIndex`, `passFlag`, `argOutputFile`, `argIterations`, and
  the `cellNeighbors` argument of the save functions
- Duplicated code folded into helpers: `compactNeighborLists()`; `findSlot`, `nextRealNeighbor`,
  `insertBetween` and `setRestLength` in `addNewCells()`; `directionAngle()` in
  `calculateGrowthPushing()`; the interior/border loops of `calculateNucleusTraction()`; the six
  copies of the row loop in `saveParameters()`; the second zeroing of `positionDeltas`

No model parameter was removed: all 30 (`Egr` … `uMgr`) and the 32-slot parameter array are intact.

---

## Improvement Suggestions

Every optimisation below was prototyped and verified to leave all output files byte-identical
(four stock workloads plus 300 randomised parameter sets). Rule for anything that touches
arithmetic: under `-ffp-contract=on` never merge a multiply and an add that were separate
statements (or split ones that shared an expression), and keep every accumulation in its original
order.

### Low Effort (Quick wins)

1. **Fix B1** (four indices in `getParameterIndex()`), after which both parameter formats give
   identical output and unit test 2 can share test 1's reference.
2. **Replace magic numbers with named constants** (values unchanged).
3. **Add a memory-checked build target**: `-fsanitize=address,undefined` plus
   `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG`. The hardening mode bounds-checks
   `std::array`/`std::vector` indexing, which is what exposed F1.

### Medium Effort (Targeted improvements)

1. **Remaining input validation**: `std::stoi` failures, step count <= 0.

### High Effort (Architectural changes)

1. **Take the parameters out of the I/O round trip.** `storeParameters()` → `saveParameters()` →
   `loadParameters()` after every step exists only to mimic the Fortran `parap` array, and is where
   F2 comes from. Parse once into the model, write the `.dad` header and `.txt` echo from the
   model, and `FileIO` shrinks to a handful of free functions.
2. **Grow per-cell arrays in place at division** instead of rebuilding and copying all of them
   (~5% at 2000 cells; touches the most delicate routine, so low priority).

### Considered and rejected

- *OpenMP*: an iteration is 20–100 µs of work at typical sizes, force sums must keep their order,
  and the division step is inherently serial — threading overhead would dominate below ~10⁴ cells.
- *Adaptive time stepping*: changes the model; results would no longer correspond to the published
  one.
- *`reserve()` on the per-cell arrays*: they are rebuilt, not grown, so it would not help.
- *Skipping the (normally all-zero) fourth quantity channel*: no gain; the fixed-width loops
  vectorise better than a run-time bound.

---

## C++ Optimizations vs. Fortran

The C++ is the same algorithm as the Fortran with the same floating-point operations in the same
order; the optimisations below only avoid work whose result is already known. Each was committed
separately (2026-09-22) and verified byte-identical to the previous step and exact against the
Fortran. Gains are CPU cycles on the stock parameter sets, relative to the step before:

| Step | What | Gain |
|------|------|------|
| Spatial grid (2026-02) | `checkNonNeighborRepulsion()`: cells binned into 1.4-wide bins, only the 27 surrounding bins examined; contributors summed in ascending index order like the Fortran | 2x overall |
| Margin reset | `calculateMargins()` resets only slot 0 of a cell whose slot 0 is empty, the only empty slot ever read | 1.05x |
| Walk cache | the margin mesh walk depends only on the neighbour lists, so it is recorded as index lists when the topology changes and re-summed in the recorded order each iteration | 1.19x |
| Candidate lists | per cell, the non-neighbours within 1.4 + 0.2; rebuilt only on a topology change or once a cell has moved more than 0.45 * 0.2, so the grid is built only at rebuilds | 1.59x (1.65x at 2000 cells) |
| Fixed-depth blocks | `quantities3D` and the reaction-diffusion deltas are one contiguous block per cell | 1.04x |
| Compact weights | `reactionDiffusion()` computes edge lengths and areas into local arrays over the occupied slots | 1.11x |
| Slot bound | every neighbour loop runs to the cell's last occupied slot instead of all 30 | 1.34x |

Together 2.5x at 170-530 cells and 3.1x at 2100 cells. Rules for anything further: never reorder
a sum or a chain of multiplications, keep every quantisation (`round`, `trunc`) where it is, and
verify byte-identity against the previous build (`docs/humppa_audit_2026-09-21/harness/bench.py`
does the stock sets, `stress_cmp.py` random sets). With `-ffp-contract=off` there is no FMA to
worry about; the order of operations must stay the Fortran's.

---

## Building

```bash
make            # Optimized build
make debug      # Debug build with symbols
make clean      # Remove build artifacts
```

Or manually, with the same flags the Makefile uses (add `-mfma` on x86_64):
```bash
clang++ -std=c++17 -O3 -funroll-loops -ffp-contract=off -DNDEBUG -flto -o humppa_cpp *.cpp
```

Do **not** add `-ffast-math`, `-march=native` or switch compilers casually: the simulation is
chaotic, and anything that changes floating-point rounding (reassociation, allowing FMA
contraction, GCC instead of Clang) changes the output, breaks the bit-identity with the Fortran
and invalidates the reference files.

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

This runs 4 tests:
1. **C++ with humppa format** - 6000 iterations, against the C++'s own stored output
2. **C++ with ToothMaker format** - same, ToothMaker-format parameter file
3. **Fortran vs C++** - 10000 iterations, against the Fortran binary run live
4. **Fortran vs C++, all parameters non-zero** - 4 steps of 2000 iterations with the hidden
   Not2/Not3/Not4 channel, Ina, uMgr and Swi all live; steps 2 and 4 are compared

All four are exact: every value in the `.dad` is parsed and must be identical (the model is
chaotic, so any rounding difference either is zero or grows to ~1e-4 within a few thousand
iterations; a tolerance would only hide that). Tests 3-4 are skipped when `humppa_fortran_64bit`
is not built.

Test files are in `../test/`:
- `mpar_no_umgr.txt` - Test parameters (humppa format)
- `toothmaker_no_umgr.txt` - Test parameters (ToothMaker format)
- `all_nonzero.txt` - Every parameter non-zero (humppa format)
- `reference/` - Reference output files for tests 1-2

See `../README.md` for more details.
