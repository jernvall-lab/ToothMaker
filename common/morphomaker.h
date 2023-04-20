#pragma once

#define MMAKER_VERSION "0.7.1"
// #define MMAKER_VERSION "0.6.4dev (build: " MMAKER_BUILD ")"

// Defines program name; controls output file names etc.
#define PROGRAM_NAME "ToothMaker"

// Default model index. Indexing is determined by the order in which the models
// are loaded, which is alphabetical.
#define DEFAULT_MODEL 0

// Debug mode. If 1, then preserves object files in tmp folder etc.
#define DEBUG_MODE 0
#define PRESERVE_MODEL_TEMP 0

// Maximum number of CPU cores internal odels can use:
#if defined(__APPLE__) || defined(LINUX) || defined(linux)
#define DEFAULT_CORES 3
#else
#define DEFAULT_CORES 1
#endif

// Decimal precision for parameter buttons.
#define PARAM_PREC 12
// Maximum value allowed for a parameter button
#define PARAM_MAX 99999

// Maximum number of iterations allowed by the interface.
#define MAX_ITER 900000

// If 1, then automatically imports example parameters during start.
#define AUTO_IMPORT_EXAMPLES 1

// Multipliers for scroll zooming.
#define ZOOM_MAX_MULTIP 3.0
#define ZOOM_MIN_MULTIP 0.02
// Scrolling wheel sensitivy. Larger value means less sensitive.
#define WHEEL_SENSITIVITY 4000.0

// Maximum number of vertices associated with each polygon.
#define MAX_POLYGON_SIZE 5

// Show mesh by default (1).
#define SHOW_MESH 1

// Interval for updating the visuals in milliseconds.
#define UPDATE_INTERVAL 4

// Size of square main window objects (parameters widget, glwidget) in pixels.
// 495 pixels is ideal when aiming for a total window width of 1024 pixels,
// allowing for 10px center marginal + 12px borders on each side of the windows.
#define SQUARE_WIN_SIZE 495

// Size of tooth history, i.e. number of ToothLife to be held in memory.
#define MAX_HISTORY_SIZE 10

// Default threshold for concentrations in model view.
#define DEFAULT_VIEW_THRESH 0.5

// Sub-folders for parameter scanning outputs.
#define DATA_SAVE_DIR   "data"
#define SSHOT_SAVE_DIR  "screenshots"

// Name for the file to store parameter scanning info.
#define SCAN_LIST "job_parameters.txt"

// Interface window width at start.
#define MAIN_WINDOW_WIDTH 1024

// Initial interface window height, platform-specific to fix aligment issues.
#if defined(__APPLE__)
#define MAIN_WINDOW_HEIGHT 650
#elif defined(LINUX) || defined(linux)
#define MAIN_WINDOW_HEIGHT 670
#else
#define MAIN_WINDOW_HEIGHT 660
#endif

// Resource folder.
#if defined(__linux__)
#define RESOURCES "Resources/"
#elif defined(__APPLE__)
#define RESOURCES "../Resources/"
#else
#define RESOURCES "../Resources/"
#endif

// Binaries folder under the resources folder.
#if defined(__linux__) || defined(__APPLE__)
#define BIN_STYLE "./bin/"
#else
#define BIN_STYLE "bin\\"
#endif

// QLineEdit() padding. Platform-specific to fix overlapping issues.
#if defined(__APPLE__)
#define FIELD_PADDING 5
#else
#define FIELD_PADDING 3
#endif

// Supported render modes.
// NOTE: Only RENDER_MESH and RENDER_PIXEL should be used.
// RENDER_HUMPPA is a legacy option.
#define RENDER_MESH 3
#define RENDER_PIXEL 1
#define RENDER_HUMPPA 0

