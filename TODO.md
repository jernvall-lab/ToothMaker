# ToothMaker Code Improvement TODO

This document tracks code improvements, bug fixes, and modernization tasks for the ToothMaker codebase.

---

## 1. Cross-Platform Compatibility (Windows)

### 1.1 Build Configuration (`interface/interface.pro`)
- [x] **Missing GLEW library linking for Windows**: **DONE** - Compiling GLEW from source (`ext/GLEW/glew.c`) with `GLEW_STATIC`. Added `-luser32 -lgdi32` for Windows API functions. Platform-specific compiler flags for MSVC.

### 1.2 OpenGL Context Creation (`interface/src/renderer/glcore.cpp`)
- [x] **Windows GL context creation**: **DONE** - Implemented using WGL with GLEW initialization for CLI/batch mode.

### 1.3 Path Separators
- [x] **Hardcoded path separators in `binaryhandler.cpp`**: **DONE** - Replaced preprocessor-based `path_style` with `QDir::toNativeSeparators(QDir("../bin").filePath(...))`. Removed 8 lines of platform-specific code.

### 1.4 OpenGL Headers
- [x] **Missing Windows OpenGL includes**: **DONE** - Added Windows GLEW includes to glcore.h and glengine.h.

### 1.5 Locale Handling (`interface/src/utils/readparameters.cpp`)
- [ ] **Linux-only locale setting**: The `setlocale()` calls at lines 38–39, 61–62, 85–86, 149–150 are wrapped in `#if defined(__linux__)`. Should apply on Windows too for consistent decimal separator handling (and arguably on macOS as well).

---

## 2. Deprecated Qt APIs

### 2.1-2.4 Core Qt Deprecations
- [x] **QGLWidget → QOpenGLWidget**: **DONE**
- [x] **QGLFormat → QSurfaceFormat**: **DONE**
- [x] **QDesktopWidget → QScreen**: **DONE**
- [x] **QWheelEvent::delta() → angleDelta()**: **DONE**

### 2.5 Old-style Signal/Slot Connections
- [x] **Use of `SIGNAL()` and `SLOT()` macros**: **DONE** - All 41 old-style connections converted to function-pointer syntax across 7 files. QSignalMapper replaced with lambdas in parameterwindow.cpp. `qOverload<>` used for overloaded signals (QComboBox, QSpinBox, QProcess). Constructor defaults updated from `=0`/`=NULL` to `=nullptr`. Removed unused `QSignalMapper` include.

### 2.6 QProcess::error Signal (`interface/src/misc/binaryhandler.cpp`)
- [x] **`QProcess::error()` signal is deprecated**: **DONE** - Using `errorOccurred()` (Qt 5.6+).

### 2.7-2.9 Other Deprecations
- [x] **QProcess::start(QString)**: **DONE** - Using `splitCommand()` + `start(program, args)`.
- [x] **sprintf**: **DONE** - All calls replaced with `QString::arg()` and `QDir::filePath()`.
- [x] **std::bind2nd**: **DONE** - Replaced with lambda.
- [x] **`Qt::CTRL + Qt::Key_X`**: **DONE** - Changed to `Qt::CTRL | Qt::Key_X` (works on both Qt5 and Qt6).

### 2.10 Qt6-only Deprecations (requires dropping Qt5 support)
- [ ] **`QCheckBox::stateChanged` → `checkStateChanged`** (deprecated Qt 6.9): 5 uses — `scanwindow.cpp:53,60,66`, `parameterwindow.cpp:130`, `controlpanel.cpp:246`. New signal passes `Qt::CheckState` instead of `int`.
- [ ] **`QMouseEvent::x()`/`y()` → `position().x()`/`.y()`** (deprecated Qt 6.0): 6 uses in `glwidget.cpp:163-164, 201-202, 224-225` (mouse handlers).
- [ ] **`QMenu::addAction(text, obj, slot, shortcut)` argument order** (deprecated Qt 6.4): 6 uses in `hampu.cpp:1200, 1206, 1208, 1210, 1212, 1218`. Qt6 wants `addAction(text, shortcut, obj, slot)`.
- [ ] **`QImage::mirrored()` → `flipped()`** (deprecated Qt 6.13): 2 uses — `glwidget.cpp:473`, `glengine.cpp:143`.

---

## 3. Bug Fixes

### 3.1 Command Line Argument Parsing (`interface/src/main.cpp`)
- [x] **Array bounds not checked**: **DONE** - Added `i+1 >= argc` guard before accessing `argv[i+1]`. Flag-only args (`--export-images`) handled separately.

### 3.2 Duplicate Header in Project File
- [x] **`glcore.h` listed twice in HEADERS**: **DONE** - Removed duplicate from `interface/interface.pro`.

### 3.3 Magic Number Key Codes
- [x] **Hardcoded key codes**: **DONE** - Replaced with `Qt::Key_Left`, `Qt::Key_Right`, `Qt::Key_Up`, `Qt::Key_Down`, `Qt::Key_5`, `Qt::Key_C` in `glwidget.cpp` and `hampu.cpp`.

### 3.4 Model Run ID Collision
- [x] **Run ID collision fix**: **DONE** - Now uses millisecond timestamp. Always 10 digits, cycles every ~13.3 days.

### 3.5-3.9 Completed Fixes
- [x] **Uninitialized `allowRotations`**: **DONE**
- [x] **Memory leaks in `cmdappcore.cpp`**: **DONE** - Stack allocation for QDir.
- [x] **Buffer overflow risk with sprintf**: **DONE** - All sprintf replaced with QString.
- [x] **NULL/nullptr inconsistency**: **DONE** - Standardized to nullptr.

### 3.10 Parameters Constructor Dead Code (`common/parameters.cpp`)
- [x] **Constructor creates unused `parameter`**: **DONE** (2026-05-09) — verified no caller ever passes a names vector (all use default-construct or the copy constructor), so the loop was doubly dead. Removed the `names` parameter entirely from `Parameters::Parameters()` and its declaration in `parameters.h`.

### 3.11 Memory Leak in `ParameterWindow::paintEvent` (`interface/src/gui/parameterwindow.cpp:254`)
- [x] **Heap-allocated `QDir` leaked on every repaint**: **DONE** (2026-05-09) — switched to stack allocation and `dir.filePath(...)` (also avoids the hardcoded `"/"` separator).

### 3.12 Broken / hidden CLI flags (`interface/src/main.cpp:17`)
- [ ] **`--step` and `--export-images` documented as broken**: Comment in `printHelp()` says these are "currently broken; hiding them from the help." Either fix and re-expose them or remove the dead code paths from `main.cpp`.

---

## 4. Code Quality (Optional/Cosmetic)

These are stylistic improvements with low priority. Address opportunistically.

### 4.1 Mixed Naming Conventions
- [ ] Inconsistent method naming: camelCase (`getProgress`), snake_case (`setSignals_`), PascalCase (`Panel_ViewMode`).

### 4.2 Debug Output Inconsistency
- [ ] Mixed use of `fprintf(stderr, ...)`, `std::cerr`, `std::cout`, and `qDebug()`. Consider standardizing on Qt's logging framework.

### 4.3 Error Handling
- [ ] Inconsistent error return values: `-1`, `1`, and `0` used for errors in different places.

### 4.4 Hard-coded Strings
- [ ] UI strings not internationalized. Consider `tr()` macro if i18n needed in future.

### 4.5 Commented-Out Code
- [ ] Dead code blocks in `glcore.cpp` (33 lines of commented OSMesa code at lines 257–289, plus a stray `glBindFragDataLocation` at line 626) and `binaryhandler.cpp`. Remove or document.

---

## 5. Memory Management

### 5.1 Raw Pointer Usage with `malloc`/`free`
- [ ] **C-style memory allocation in OpenGL code** (`glcore.cpp:270, 709, 711, 749, 751`): 5 `malloc`/`free` calls for image buffers. Could use `std::vector<GLubyte>` for RAII.

### 5.2 Raw Pointer Ownership
- [ ] **Unclear ownership semantics**: Many classes store raw pointers (`Model*`, `Parameters*`) without ownership documentation. Consider smart pointers where appropriate.

### 5.3 Virtual Destructor
- [x] **`Model` class**: **DONE** — destructor is `virtual` (`common/model.h:59`). Verified during 2026-05-09 audit.

---

## 6. OpenGL Modernization (Future)

These are intentional design decisions for compatibility, not bugs. Document here for future consideration.

### 6.1 Legacy OpenGL
- [ ] `gl_legacy.cpp` uses immediate mode (`glBegin`/`glEnd`). Works with compatibility profile; modern OpenGL would require VBOs.

### 6.2 Fixed Function Pipeline
- [ ] `glShadeModel`, `glLightfv`, etc. are deprecated. Work only in compatibility profile.

### 6.3 GLSL Shader Version
- [ ] Shaders use GLSL 1.20 (compatibility). Core profile would need GLSL 3.30+.

### 6.4 Apple VAO Macros
- [ ] `glcore.h` redefines `glBindVertexArray`/`glGenVertexArrays` to Apple extensions. May need updates for newer macOS.

### 6.5 Apple OpenGL Deprecation
- [ ] **All OpenGL APIs deprecated on macOS since 10.14.** Produces ~78 warnings with Qt6 builds. Can silence with `GL_SILENCE_DEPRECATION` define, but Apple may eventually remove OpenGL entirely. Long-term options: Metal via Qt's RHI backend, or Vulkan via MoltenVK. The webtooth (WebGL/Three.js) port sidesteps this for cross-platform use.

See `docs/rendering_options.md` for detailed migration options.

---

## 7. Thread Safety

### 7.1 ToothLife Mutex
- [x] **Incomplete mutex coverage**: **DONE** - Added lock to `getLifeSize()`.

### 7.2 Progress Updates
- [x] **`currentIter` race**: **DONE** (2026-05-09) — changed `currentIter` to `std::atomic<int>` in `common/model.h` and updated `getProgress()` to use explicit `.load()`. Worker-thread writes (`binaryhandler.cpp:632, 688`) and GUI-thread reads now synchronize through atomic ops.

---

## 8. Build System

### 8.1 C++ Standard
- [x] **C++17**: **DONE** - Upgraded from C++11. Also added conditional Qt6 `openglwidgets` module.

### 8.2 Windows Configuration
- [ ] **No Windows `.pri` file**: Has `gcc-macports.pri` and `clang-macports.pri` but no MSVC equivalent.

### 8.3 Version Information
- [ ] **Git-based version number**: Uses `git rev-list --count HEAD` which may not work without git in PATH.

---

## 9. Documentation

### 9.1 Outdated References
- [ ] Comments and user-facing strings still reference "MorphoMaker":
  - `interface/src/main.cpp:22` — `printHelp()` prints `"** MorphoMaker %s **\n"` (user-visible CLI banner; should be "ToothMaker")
  - `interface/src/utils/readparameters.cpp:7` — comment "Supported file formats: MorphoMaker"
  - `interface/src/misc/binaryhandler.cpp:505` — XML `inputStyle` keyword `"MorphoMaker"`. NOTE: this is a programmatic identifier matched against XML interface files; renaming requires also updating every model's interface XML and is therefore breaking. Either keep for compatibility or coordinate a rename across all `*_interface.xml` files.
  - `morphomaker::` namespace in `common/morphomaker.h` and call sites — internal, low priority.

---

## 10. Testing

### 10.1 Unit Tests
- [x] **C++ model tests**: **DONE** - Unit test framework in `models/tribosphenic/src/test/`. Tests C++ model output against references and cross-validates with Fortran.
- [ ] **GUI/interface tests**: No automated tests for Qt interface. Consider Qt Test framework for critical components.

### 10.2 CLI Integration Tests
- [x] **Command-line parameter scanning tests**: **DONE** - 12 tests (6 per model) in `examples/cli/run_tests.sh`. Integrated into CI. Tests job_parameters, cusp angles, local maxima, cuspA baseline (tolerance-based), data structure, and screenshots.

### 10.3 CI/CD
- [x] **Add GitHub Actions**: **DONE** - Multi-platform builds working (`.github/workflows/build.yml`):
  - Linux x64, Linux ARM, macOS Intel, macOS ARM, Windows: All building successfully
  - Produces downloadable artifacts for each platform

---

## 11. Strategic Decisions

### 11.1 webtooth (In Progress)
- [ ] **WebAssembly browser port**: Working build exists in `webtooth/` using Emscripten + Three.js. Not yet committed. Decisions needed:
  - Commit to main repo or separate?
  - Replace Windows native version with web-only?

### 11.2 C++ Model as Fortran Replacement
- [ ] **Replace 32-bit Fortran binaries**: The C++ port in `models/tribosphenic/src/cpp/` produces identical output. Could eliminate need for pre-built Fortran binaries and 32-bit compatibility issues.

### 11.3 Windows Strategy
- [ ] **Native Windows vs Web-only**: Is Windows native build worth maintaining, or should effort go to web version which works everywhere?

### 11.4 BinaryHandler Restructuring Plan
- [ ] **Plan drafted but not executed**: `docs/binaryhandler_restructuring_plan.md` (2026-01-20 Session 4) proposes splitting `BinaryHandler` into `ProcessController`, `ParserPipeline`, and `OutputReader`. Only `OutputReader` was implemented (`interface/src/misc/binaryhandler.h:45`); the rest of the plan is dormant. Decide: execute, narrow scope, or close.

---

## Priority Recommendations

**High Priority (functionality):**
1. ~~Fix path separator handling (1.3)~~ **DONE**
2. ~~Fix command line argument bounds checking (3.1)~~ **DONE** (2026-04-22)
3. Decide on webtooth commit strategy (11.1)
4. ~~Fix `ParameterWindow::paintEvent` QDir leak (3.11)~~ **DONE** (2026-05-09)
5. Decide what to do with broken `--step` / `--export-images` CLI flags (3.12).

**Medium Priority (stability/quality):**
1. ~~Signal/slot macro modernization (2.5)~~ **DONE**
2. ~~Magic key code constants (3.3)~~ **DONE** (2026-04-22)
3. ~~Thread safety for `currentIter` (7.2)~~ **DONE** (2026-05-09)
4. ~~Remove dead-code constructor loop in `Parameters` (3.10)~~ **DONE** (2026-05-09)
5. Update user-facing "MorphoMaker" → "ToothMaker" in CLI banner (9.1).

**Lower Priority (nice-to-have):**
1. GUI unit tests (10.1)
2. MorphoMaker→ToothMaker references (9.1)
3. Code quality items (4.x)

**Strategic (requires decision):**
1. C++ model replacing Fortran binaries (11.2)
2. ~~Windows native vs web-only strategy (11.3)~~ Windows build now working

---

## Related Documentation

- `docs/rendering_options.md` - Modern OpenGL/rendering alternatives
- `docs/windows_setup.md` - Windows development environment setup
- `CLAUDE.md` - Session log and project context
- `IDEAS.md` - Future concepts and explorations
