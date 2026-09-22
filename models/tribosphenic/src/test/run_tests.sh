#!/bin/bash
# run_tests.sh - Unit tests for humppa_cpp and humppa (Fortran)
#
# Tests 1-2 compare the C++ against its own stored output (6000 iterations,
# references made on macOS). Tests 3-4 compare the C++ against the Fortran
# built on this machine (Test 4 with every parameter non-zero -- the hidden
# Not2/Not3/Not4 channel, Ina, uMgr and Swi all live -- in 4 steps).
#
# Every comparison is exact: all numbers in the .dad files are parsed and must
# be identical. Both programs write 17 significant digits, which identify a
# double uniquely, and they format them differently, so bytes are never
# compared. The model is chaotic, so any rounding difference -- a compiler
# flag, a libm, a platform -- is either exactly zero or grows to ~1e-4 within
# a few thousand iterations; a tolerance would only hide that. Since 2026-02
# every platform builds with the same clang flags and has produced identical
# results; if Tests 1-2 fail on one platform only, the platforms have diverged.

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

# Every token of a .dad file, one per line, with the "cell shape" words removed.
dad_tokens() {
    sed 's/cell shape//' "$1" | tr -s ' \t\n' '\n' | sed '/^$/d'
}

# Exact comparison of two .dad files: every value, parsed as a number, must be
# identical. Token 32 (ncils) is skipped because the C++ writes ncils-1 there
# by design when compared with the Fortran. On failure the largest deviation
# is reported, which tells a rounding divergence (~1e-4 and up) from a bug.
# Returns 0 if identical. Prints result info.
compare_dad_exact() {
    local reference_file="$1"
    local test_file="$2"

    dad_tokens "$reference_file" > _tok_r.txt
    dad_tokens "$test_file" > _tok_t.txt
    local nr=$(wc -l < _tok_r.txt | tr -d ' ')
    local nt=$(wc -l < _tok_t.txt | tr -d ' ')
    if [ "$nr" != "$nt" ]; then
        echo "token count differs: $nr vs $nt (topology differs)"
        rm -f _tok_r.txt _tok_t.txt
        return 1
    fi

    paste _tok_r.txt _tok_t.txt | awk '
    NR == 32 { next }
    {
        if ($1 + 0 != $2 + 0) {
            bad++
            d = $1 - $2; if (d < 0) d = -d
            if (d > maxd) maxd = d
        }
    }
    END {
        if (bad > 0) printf "%d values, %d mismatches, max |diff| = %.3g", NR - 1, bad, maxd
        else         printf "%d values, all identical", NR - 1
        exit (bad > 0)
    }'
    local rv=$?
    rm -f _tok_r.txt _tok_t.txt
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

echo -n "  Comparing every value with the stored reference... "
if result=$(compare_dad_exact "$SCRIPT_DIR/reference/6000_cpp_humppa.dad" "6000_output.dad_.dad"); then
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

echo -n "  Comparing every value with the stored reference... "
if result=$(compare_dad_exact "$SCRIPT_DIR/reference/6000_cpp_toothmaker.dad" "6000_output.dad_.dad"); then
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
