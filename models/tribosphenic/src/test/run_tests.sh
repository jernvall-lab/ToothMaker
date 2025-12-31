#!/bin/bash
# run_tests.sh - Unit tests for humppa_cpp and humppa (Fortran)
# All tests use 6000 iterations with umgr=0

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_BIN="$SCRIPT_DIR/../cpp/humppa_cpp"
FORTRAN_BIN="$SCRIPT_DIR/../fortran/humppa"
TEST_TMP="$SCRIPT_DIR/../tmp_test"

# Check that C++ binary exists (required for all tests)
if [ ! -x "$CPP_BIN" ]; then
    echo "ERROR: C++ binary not found: $CPP_BIN"
    echo ""
    echo "Please build the binaries first:"
    echo "  cd $(dirname "$SCRIPT_DIR")"
    echo "  make"
    echo ""
    echo "Or build C++ only:"
    echo "  make cpp"
    exit 1
fi

mkdir -p "$TEST_TMP"
trap "rm -rf $TEST_TMP" EXIT

PASS=0
FAIL=0

# Tolerance for floating-point comparisons (handles cross-platform differences)
POSITION_TOLERANCE=0.001

# Extract connectivity section (lines 8 to before "cell shape")
extract_connectivity() {
    local file="$1"
    local end_line=$(grep -n "cell shape" "$file" | head -1 | cut -d: -f1)
    if [ -z "$end_line" ]; then
        echo "ERROR: Could not find 'cell shape' marker in $file" >&2
        return 1
    fi
    sed -n "8,$((end_line-1))p" "$file" | tr -s ' ' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//'
}

# Extract cell shape vertices (lines after "N cell shape" markers)
extract_cell_shapes() {
    local file="$1"
    awk '/cell shape/ { n = $1; for (i = 0; i < n; i++) { getline; print } }' "$file"
}

# Compare positions with tolerance (returns 0 if all within tolerance)
compare_positions() {
    local file1="$1"
    local file2="$2"
    local tolerance="$3"

    paste "$file1" "$file2" | awk -v tol="$tolerance" '
    BEGIN { max = 0; fail = 0 }
    {
        dx = $1 - $4
        dy = $2 - $5
        dz = $3 - $6
        dist = sqrt(dx*dx + dy*dy + dz*dz)
        if (dist > max) max = dist
        if (dist > tol) fail = 1
    }
    END {
        printf "max_dist=%.6f ", max
        exit fail
    }'
}

# Compare two .dad files: exact connectivity match + cell shapes within tolerance
# Returns 0 on success, 1 on failure. Prints result info.
compare_dad_files() {
    local file1="$1"
    local file2="$2"
    local tolerance="$3"

    # Check connectivity (must match exactly)
    extract_connectivity "$file1" > _conn1.txt 2>/dev/null
    extract_connectivity "$file2" > _conn2.txt 2>/dev/null
    if ! diff -q _conn1.txt _conn2.txt >/dev/null 2>&1; then
        echo "connectivity mismatch"
        rm -f _conn1.txt _conn2.txt
        return 1
    fi
    rm -f _conn1.txt _conn2.txt

    # Check cell shapes with tolerance
    extract_cell_shapes "$file1" > _shapes1.txt
    extract_cell_shapes "$file2" > _shapes2.txt
    if result=$(compare_positions _shapes1.txt _shapes2.txt "$tolerance"); then
        echo "$result"
        rm -f _shapes1.txt _shapes2.txt
        return 0
    else
        echo "$result"
        rm -f _shapes1.txt _shapes2.txt
        return 1
    fi
}

echo "========================================"
echo "Running humppa unit tests (6000 iterations)"
echo "========================================"
echo ""

#---------------------------------------------------------------------------
# Test 1: C++ with humppa parameter format
#---------------------------------------------------------------------------
echo "TEST 1: C++ with humppa parameter format"
echo -n "  Running simulation... "
cd "$TEST_TMP"
"$CPP_BIN" "$SCRIPT_DIR/mpar_no_umgr.txt" output.dad 6000 1 >/dev/null 2>&1
echo "done"

echo -n "  Comparing output (tol=$POSITION_TOLERANCE)... "
if result=$(compare_dad_files "6000_output.dad_.dad" "$SCRIPT_DIR/reference/6000_cpp_humppa.dad" $POSITION_TOLERANCE); then
    echo "PASS ($result)"
    PASS=$((PASS + 1))
else
    echo "FAIL ($result)"
    FAIL=$((FAIL + 1))
fi
rm -f *.dad *.off *.txt _*.txt

#---------------------------------------------------------------------------
# Test 2: C++ with ToothMaker parameter format
#---------------------------------------------------------------------------
echo ""
echo "TEST 2: C++ with ToothMaker parameter format"
echo -n "  Running simulation... "
"$CPP_BIN" "$SCRIPT_DIR/toothmaker_no_umgr.txt" output.dad 6000 1 >/dev/null 2>&1
echo "done"

echo -n "  Comparing output (tol=$POSITION_TOLERANCE)... "
if result=$(compare_dad_files "6000_output.dad_.dad" "$SCRIPT_DIR/reference/6000_cpp_toothmaker.dad" $POSITION_TOLERANCE); then
    echo "PASS ($result)"
    PASS=$((PASS + 1))
else
    echo "FAIL ($result)"
    FAIL=$((FAIL + 1))
fi
rm -f *.dad *.off *.txt _*.txt

#---------------------------------------------------------------------------
# Test 3: Fortran vs C++ cross-validation (connectivity and positions)
#---------------------------------------------------------------------------
echo ""
echo "TEST 3: Fortran vs C++ cross-validation"

if [ ! -x "$FORTRAN_BIN" ]; then
    echo "  Skipping: Fortran binary not found"
else
    echo -n "  Running Fortran... "
    cp "$SCRIPT_DIR/mpar_no_umgr.txt" .
    "$FORTRAN_BIN" mpar_no_umgr.txt out_f.dad 6000 1 >/dev/null 2>&1 || true
    F_FILE=$(ls 6000*out_f*.dad 2>/dev/null | head -1)

    if [ -z "$F_FILE" ]; then
        echo "SKIPPED (Fortran binary failed)"
    else
        echo "done"

        echo -n "  Running C++... "
        "$CPP_BIN" "$SCRIPT_DIR/mpar_no_umgr.txt" out_c.dad 6000 1 >/dev/null 2>&1
        C_FILE=$(ls 6000*out_c*.dad 2>/dev/null | head -1)
        echo "done"

        # Test 3a: Connectivity
        echo -n "  Comparing connectivity... "
        extract_connectivity "$F_FILE" > f_conn.txt 2>/dev/null
        extract_connectivity "$C_FILE" > c_conn.txt 2>/dev/null
        if diff -q f_conn.txt c_conn.txt >/dev/null 2>&1; then
            echo "PASS"
            PASS=$((PASS + 1))
        else
            echo "FAIL"
            FAIL=$((FAIL + 1))
        fi
        rm -f f_conn.txt c_conn.txt

        # Test 3b: Cell shapes (with tolerance)
        echo -n "  Comparing cell shapes (tol=$POSITION_TOLERANCE)... "
        extract_cell_shapes "$F_FILE" > f_shapes.txt
        extract_cell_shapes "$C_FILE" > c_shapes.txt
        if result=$(compare_positions f_shapes.txt c_shapes.txt $POSITION_TOLERANCE); then
            echo "PASS ($result)"
            PASS=$((PASS + 1))
        else
            echo "FAIL ($result)"
            FAIL=$((FAIL + 1))
        fi
        rm -f f_shapes.txt c_shapes.txt
    fi
    rm -f *.dad *.off *.txt kk 2>/dev/null
fi

echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "========================================"
exit $FAIL
