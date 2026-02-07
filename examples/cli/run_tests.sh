#!/bin/bash
# run_tests.sh - Unit tests for ToothMaker command-line parameter scanning
#
# Runs parameter scans for two models and compares output against reference:
#   - Triconodont (humppa_2010 Fortran binary)
#   - Tribosphenic (humppa_cpp_64bit C++ binary)
#
# Per model, 6 tests are run:
#   1. job_parameters.txt    (exact match)
#   2. top_cusp_angles.txt   (sorted comparison)
#   3. local_maxima.txt      (tolerance comparison, handles Y sign ambiguity)
#   4. cuspA_baseline.txt    (tolerance comparison, handles Y sign ambiguity)
#   5. data/ directory       (subdirectory names, file counts, extensions)
#   6. screenshots/          (exact filename listing)
#
# Usage: ./run_tests.sh [path-to-ToothMaker-binary]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REF_DIR="$SCRIPT_DIR/reference"
TOOTHMAKER_ROOT="$SCRIPT_DIR/../.."

# --- Parse arguments ---
SKIP_TRICONODONT=0
BIN=""
for arg in "$@"; do
    case "$arg" in
        --skip-triconodont) SKIP_TRICONODONT=1 ;;
        *) BIN="$arg" ;;
    esac
done

# --- Find ToothMaker binary ---
if [ -z "$BIN" ]; then
    if [ -x "$TOOTHMAKER_ROOT/build/interface/ToothMaker.app/Contents/MacOS/ToothMaker" ]; then
        BIN="$TOOTHMAKER_ROOT/build/interface/ToothMaker.app/Contents/MacOS/ToothMaker"
    elif [ -x "$TOOTHMAKER_ROOT/build/interface/ToothMaker" ]; then
        BIN="$TOOTHMAKER_ROOT/build/interface/ToothMaker"
    fi
fi

if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "ERROR: ToothMaker binary not found."
    echo ""
    echo "Build ToothMaker first, or provide the binary path:"
    echo "  ./run_tests.sh /path/to/ToothMaker"
    exit 1
fi

# Resolve to absolute path (script cd's during tests).
BIN="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")"

echo "Binary: $BIN"

# --- Linux: set DISPLAY for offscreen rendering ---
if [ "$(uname)" = "Linux" ] && [ -z "$DISPLAY" ]; then
    export DISPLAY=":0"
    echo "DISPLAY set to $DISPLAY (Linux offscreen rendering)"
fi

# --- Setup ---
TEST_TMP="$SCRIPT_DIR/tmp_test"
rm -rf "$TEST_TMP"
mkdir -p "$TEST_TMP"
trap "cd '$SCRIPT_DIR' && rm -rf '$TEST_TMP'" EXIT

PASS=0
FAIL=0
TOLERANCE=0.001

pass() {
    echo "  PASS${1:+ ($1)}"
    PASS=$((PASS + 1))
}

fail() {
    echo "  FAIL${1:+: $1}"
    FAIL=$((FAIL + 1))
}

# Filter out .DS_Store from directory listings
list_files() {
    ls -1 "$1" | grep -v '\.DS_Store' | LC_ALL=C sort
}

# Count files in directory (excluding .DS_Store)
count_files() {
    ls -1 "$1" | grep -v '\.DS_Store' | wc -l | tr -d ' '
}

# Get file extension distribution (count per extension, sorted)
ext_distribution() {
    ls -1 "$1" | grep -v '\.DS_Store' | sed 's/.*\.//' | LC_ALL=C sort | uniq -c | LC_ALL=C sort
}

# =========================================================================
# run_model_tests MODEL_NAME PARAM_FILE REF_SUBDIR
#   Runs a scan and compares output against reference/$REF_SUBDIR/.
# =========================================================================
run_model_tests() {
    local model_name="$1"
    local param_file="$2"
    local ref_subdir="$3"
    local ref="$REF_DIR/$ref_subdir"
    local test_num="$4"
    local niter="$5"

    echo ""
    echo "--- $model_name ---"
    echo ""

    # Prepare working directory for this model.
    local work="$TEST_TMP/$ref_subdir"
    rm -rf "$work"
    mkdir -p "$work"
    cp "$SCRIPT_DIR/$param_file" "$work/"
    cp "$SCRIPT_DIR/scanlist.txt" "$work/"

    echo "Running parameter scan (6 combinations, $niter iterations)..."
    cd "$work"
    "$BIN" --niter "$niter" --param "$param_file" --scan scanlist.txt
    echo ""

    # Test 1: job_parameters.txt
    echo "TEST ${test_num}.1: job_parameters.txt"
    if [ ! -f job_parameters.txt ]; then
        fail "file not found"
    elif diff job_parameters.txt "$ref/job_parameters.txt" >/dev/null 2>&1; then
        pass
    else
        fail "content mismatch"
    fi

    # Test 2: top_cusp_angles.txt (sorted)
    echo "TEST ${test_num}.2: top_cusp_angles.txt"
    if [ ! -f top_cusp_angles.txt ]; then
        fail "file not found"
    else
        tail -n +2 top_cusp_angles.txt | LC_ALL=C sort > "$work/_tca_test.txt"
        tail -n +2 "$ref/top_cusp_angles.txt" | LC_ALL=C sort > "$work/_tca_ref.txt"
        if diff "$work/_tca_test.txt" "$work/_tca_ref.txt" >/dev/null 2>&1; then
            pass
        else
            fail "content mismatch"
        fi
    fi

    # Test 3: local_maxima.txt (tolerance, abs Y for sign ambiguity)
    echo "TEST ${test_num}.3: local_maxima.txt"
    if [ ! -f local_maxima.txt ]; then
        fail "file not found"
    else
        tail -n +2 local_maxima.txt | LC_ALL=C sort > "$work/_lm_test.txt"
        tail -n +2 "$ref/local_maxima.txt" | LC_ALL=C sort > "$work/_lm_ref.txt"

        local test_lines=$(wc -l < "$work/_lm_test.txt" | tr -d ' ')
        local ref_lines=$(wc -l < "$work/_lm_ref.txt" | tr -d ' ')

        if [ "$test_lines" != "$ref_lines" ]; then
            fail "line count: $test_lines vs $ref_lines"
        elif paste "$work/_lm_test.txt" "$work/_lm_ref.txt" | awk -v tol="$TOLERANCE" '
            {
                if ($1 != $5) { exit 1 }
                dx = $2 - $6; if (dx < 0) dx = -dx
                ay1 = $3; if (ay1 < 0) ay1 = -ay1
                ay2 = $7; if (ay2 < 0) ay2 = -ay2
                dy = ay1 - ay2; if (dy < 0) dy = -dy
                dz = $4 - $8; if (dz < 0) dz = -dz
                if (dx > tol || dy > tol || dz > tol) exit 1
            }'; then
            pass "tol=$TOLERANCE, abs(Y)"
        else
            fail "values exceed tolerance ($TOLERANCE)"
        fi
    fi

    # Test 4: cuspA_baseline.txt (tolerance, abs Y for sign ambiguity)
    echo "TEST ${test_num}.4: cuspA_baseline.txt"
    if [ ! -f cuspA_baseline.txt ]; then
        fail "file not found"
    else
        tail -n +2 cuspA_baseline.txt | LC_ALL=C sort > "$work/_cb_test.txt"
        tail -n +2 "$ref/cuspA_baseline.txt" | LC_ALL=C sort > "$work/_cb_ref.txt"

        local test_lines=$(wc -l < "$work/_cb_test.txt" | tr -d ' ')
        local ref_lines=$(wc -l < "$work/_cb_ref.txt" | tr -d ' ')

        if [ "$test_lines" != "$ref_lines" ]; then
            fail "line count: $test_lines vs $ref_lines"
        elif paste "$work/_cb_test.txt" "$work/_cb_ref.txt" | awk -v tol="$TOLERANCE" '
            {
                if ($1 != $5) { exit 1 }
                dx = $2 - $6; if (dx < 0) dx = -dx
                ay1 = $3; if (ay1 < 0) ay1 = -ay1
                ay2 = $7; if (ay2 < 0) ay2 = -ay2
                dy = ay1 - ay2; if (dy < 0) dy = -dy
                dz = $4 - $8; if (dz < 0) dz = -dz
                if (dx > tol || dy > tol || dz > tol) exit 1
            }'; then
            pass "tol=$TOLERANCE, abs(Y)"
        else
            fail "values exceed tolerance ($TOLERANCE)"
        fi
    fi

    # Test 5: data/ directory structure
    echo "TEST ${test_num}.5: data/ directory structure"
    if [ ! -d data ]; then
        fail "data/ directory not found"
    else
        local ref_dirs=$(list_files "$ref/data/")
        local test_dirs=$(list_files data/)

        if [ "$ref_dirs" != "$test_dirs" ]; then
            fail "subdirectory names differ"
        else
            local all_ok=1
            for dir in $ref_dirs; do
                local ref_count=$(count_files "$ref/data/$dir/")
                local test_count=$(count_files "data/$dir/")
                if [ "$ref_count" != "$test_count" ]; then
                    fail "data/$dir/: expected $ref_count files, got $test_count"
                    all_ok=0
                    break
                fi

                local ref_exts=$(ext_distribution "$ref/data/$dir/")
                local test_exts=$(ext_distribution "data/$dir/")
                if [ "$ref_exts" != "$test_exts" ]; then
                    fail "data/$dir/: file type distribution differs"
                    all_ok=0
                    break
                fi
            done
            if [ "$all_ok" -eq 1 ]; then
                pass "6 subdirs, file counts and types match"
            fi
        fi
    fi

    # Test 6: screenshots/ file listing
    echo "TEST ${test_num}.6: screenshots/ file listing"
    if [ ! -d screenshots ]; then
        fail "screenshots/ directory not found"
    else
        local ref_files=$(list_files "$ref/screenshots/")
        local test_files=$(list_files screenshots/)
        if [ "$ref_files" = "$test_files" ]; then
            pass
        else
            fail "file listing differs"
        fi
    fi
}

# =========================================================================
echo "========================================"
echo "ToothMaker command-line scanning tests"
echo "========================================"

if [ "$SKIP_TRICONODONT" -eq 1 ]; then
    echo ""
    echo "--- Triconodont (Fortran): SKIPPED (--skip-triconodont) ---"
else
    run_model_tests "Triconodont (Fortran)" seal_triconodont.txt triconodont_9k 1 9000
fi
run_model_tests "Tribosphenic (C++)" seal_tribosphenic.txt tribosphenic_10k 2 10000

echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "========================================"
exit $FAIL
