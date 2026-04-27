#!/bin/sh
# run_tests.sh — Micro-C 6809 test suite runner (usim09 target)
#
# Usage:
#   cd tests/
#   ./run_tests.sh                  # run all tests
#   ./run_tests.sh t01_controlflow  # run one test by prefix
#
# Requirements: build the toolchain first with `make` in the root directory.
# The usim09 binary must be on PATH or present in the root directory.
#
# Exit code: 0 if all tests pass, 1 if any test fails.

set -e

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$TESTS_DIR")"
USIM="${USIM:-$(which usim09 2>/dev/null || echo "$ROOT/usim09")}"

MCDIR="$ROOT"
MCINCLUDE="$ROOT/include"
MCLIBDIR="$ROOT/targets/usim09/lib09"
export MCDIR MCINCLUDE MCLIBDIR

PASS_FILES=0; FAIL_FILES=0
PASS_TESTS=0; FAIL_TESTS=0
COMPILE_FAIL=0

cd "$TESTS_DIR"

# Determine which tests to run
if [ $# -gt 0 ]; then
    PATTERN="$1"
    FILES=$(ls t[0-9]*.c 2>/dev/null | grep "^${PATTERN}" | sort | uniq)
    if [ -z "$FILES" ]; then
        echo "No test files matching: ${PATTERN}*.c" >&2
        exit 1
    fi
else
    FILES=$(ls t[0-9][0-9]_*.c t[0-9][0-9][a-z]_*.c 2>/dev/null | sort | uniq)
fi

printf "%-26s %s\n" "Test" "Result"
printf "%-26s %s\n" "----" "------"

for f in $FILES; do
    name="$(basename "$f" .c)"
    hex="${f%.c}.HEX"

    # Compile
    err=$("$ROOT/cc09" "$f" -PIq 2>&1) || true
    if [ ! -f "$hex" ]; then
        printf "%-26s COMPILE ERROR\n" "$name"
        printf "  %s\n" "$err"
        COMPILE_FAIL=$((COMPILE_FAIL+1))
        FAIL_FILES=$((FAIL_FILES+1))
        continue
    fi

    # Run
    out=$(echo "" | timeout 1 "$USIM" "$hex" 2>/dev/null | tr -d '\r')
    suite=$(echo "$out" | grep "^SUITE:" | tail -1)
    p=$(echo "$out" | grep ":OK$" 2>/dev/null | wc -l | tr -d " ")
    fails=$(echo "$out" | grep ":FAIL" 2>/dev/null || true)
    nfail=$(echo "$fails" | grep ":FAIL" 2>/dev/null | wc -l | tr -d " ")

    PASS_TESTS=$((PASS_TESTS+p))
    FAIL_TESTS=$((FAIL_TESTS+nfail))

    if echo "$suite" | grep -q "SUITE:PASS"; then
        printf "%-26s %s\n" "$name" "$suite"
        PASS_FILES=$((PASS_FILES+1))
    else
        printf "%-26s %s\n" "$name" "${suite:-NO SUITE LINE}"
        echo "$fails" | while read line; do
            [ -n "$line" ] && printf "  %s\n" "$line"
        done
        FAIL_FILES=$((FAIL_FILES+1))
    fi
done

echo ""
echo "Files : $PASS_FILES passed, $FAIL_FILES failed"
echo "Tests : $PASS_TESTS passed, $FAIL_TESTS failed"

if [ "$FAIL_FILES" -gt 0 ] || [ "$COMPILE_FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
