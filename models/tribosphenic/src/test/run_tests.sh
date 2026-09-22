#!/bin/bash
# run_tests.sh - Unit tests for humppa_cpp and humppa (Fortran)
#
# Tests 1-2 compare the C++ against stored references (6000 iterations,
# tolerance 1e-3: the references were made on macOS and libm rounding may
# differ elsewhere).
# Tests 3-4 compare the C++ against the Fortran built on this machine and
# require every number in the .dad file to match exactly (the C++ writes 6
# decimals, so the Fortran values are rounded to 6 decimals first). Test 4
# has every parameter non-zero -- the hidden Not2/Not3/Not4 channel, Ina,
# uMgr and Swi are all live -- and runs in 4 steps.

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_BIN="$SCRIPT_DIR/../../bin/humppa_cpp_64bit"
FORTRAN_BIN="$SCRIPT_DIR/../../bin/humppa_fortran_64bit"
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

# Every token of a .dad file, one per line, with the "cell shape" words removed.
dad_tokens() {
    sed 's/cell shape//' "$1" | tr -s ' \t\n' '\n' | sed '/^$/d'
}

# Exact comparison of a Fortran .dad (17 significant digits) with a C++ .dad
# (6 decimals): every C++ token must equal the Fortran value printed with %.6f.
# Token 32 (ncils) is skipped: the C++ writes ncils-1 there by design.
# Returns 0 if identical. Prints result info.
compare_dad_exact() {
    local fortran_file="$1"
    local cpp_file="$2"

    dad_tokens "$fortran_file" > _tok_f.txt
    dad_tokens "$cpp_file" > _tok_c.txt
    local nf=$(wc -l < _tok_f.txt | tr -d ' ')
    local nc=$(wc -l < _tok_c.txt | tr -d ' ')
    if [ "$nf" != "$nc" ]; then
        echo "token count differs: $nf vs $nc (topology differs)"
        rm -f _tok_f.txt _tok_c.txt
        return 1
    fi

    paste _tok_f.txt _tok_c.txt | awk '
    NR == 32 { next }
    {
        if ($2 ~ /[.eE]/) { if (sprintf("%.6f", $1 + 0) != $2) bad++ }
        else              { if ($1 + 0 != $2 + 0) bad++ }
    }
    END {
        printf "%d values, %d mismatches", NR - 1, bad + 0
        exit (bad > 0)
    }'
    local rv=$?
    rm -f _tok_f.txt _tok_c.txt
    return $rv
}

# Runs the Fortran and the C++ on the same parameter file and compares the
# .dad of the given iteration counts exactly. The Fortran binary truncates
# paths longer than 30 characters, so the parameter file is copied next to it.
# $1 = parameter file (in SCRIPT_DIR), $2 = iterations per step, $3 = steps,
# remaining args = iteration counts whose .dad files to compare.
fortran_vs_cpp_exact() {
    local par="$1"; local iters="$2"; local steps="$3"; shift 3

    echo -n "  Running Fortran... "
    cp "$SCRIPT_DIR/$par" .
    "$FORTRAN_BIN" "$par" out_f.dad "$iters" "$steps" >/dev/null 2>&1 || true
    if [ -z "$(ls $((iters * steps))*out_f*.dad 2>/dev/null)" ]; then
        echo "SKIPPED (Fortran binary failed)"
        rm -f *.dad *.off *.txt kk 2>/dev/null
        return
    fi
    echo "done"

    echo -n "  Running C++... "
    "$CPP_BIN" "$SCRIPT_DIR/$par" out_c.dad "$iters" "$steps" >/dev/null 2>&1
    echo "done"

    for it in "$@"; do
        echo -n "  Comparing every value at iteration $it... "
        F_FILE=$(ls ${it}*out_f*.dad 2>/dev/null | head -1)
        if result=$(compare_dad_exact "$F_FILE" "${it}_out_c.dad_.dad"); then
            echo "PASS ($result)"
            PASS=$((PASS + 1))
        else
            echo "FAIL ($result)"
            FAIL=$((FAIL + 1))
        fi
    done
    rm -f *.dad *.off *.txt kk 2>/dev/null
}

echo "========================================"
echo "Running humppa unit tests"
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
# Test 3: Fortran vs C++, exact match, default test parameters
#---------------------------------------------------------------------------
echo ""
echo "TEST 3: Fortran vs C++ exact match (10000 iterations)"

if [ ! -x "$FORTRAN_BIN" ]; then
    echo "  Skipping: Fortran binary not found"
else
    fortran_vs_cpp_exact mpar_no_umgr.txt 10000 1 10000
fi

#---------------------------------------------------------------------------
# Test 4: Fortran vs C++, exact match, every parameter non-zero, 4 steps
#---------------------------------------------------------------------------
echo ""
echo "TEST 4: Fortran vs C++ exact match, all parameters non-zero (4 x 2000 iterations)"

if [ ! -x "$FORTRAN_BIN" ]; then
    echo "  Skipping: Fortran binary not found"
else
    fortran_vs_cpp_exact all_nonzero.txt 2000 4 4000 8000
fi

echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "========================================"
exit $FAIL
