#!/bin/sh
# run_tests_coco.sh - Micro-C 6809 test suite runner using MAME coco2b
#
# Usage:
#   cd ~/mc09
#   sh tests/run_tests_coco.sh              # run all tests
#   sh tests/run_tests_coco.sh t01          # run one test by prefix
#
# Requirements:
#   - toolchain built (make)
#   - targets/coco_test/lib09/ generated and patched (Steps 2-6)
#   - mame on PATH with coco2b ROMs
#   - objcopy on PATH
#   - python3 on PATH

set -e

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$TESTS_DIR")"

MCDIR="$ROOT"
MCINCLUDE="$ROOT/include"
MCLIBDIR="$ROOT/targets/coco_test/lib09"
export MCDIR MCINCLUDE MCLIBDIR

LUA_SCRIPT="$TESTS_DIR/coco_monitor.lua"
OUTFILE="/tmp/mc09_coco_out.txt"
RAW_BIN="/tmp/mc09_coco_raw.bin"
ROMFILE="/tmp/mc09_coco_test.rom"

# Safety limit: MAME exits after this many emulated seconds even if
# SUITE:PASS/FAIL is never seen (catches hangs).
TIMEOUT_SECS=20

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

    rm -f "$OUTFILE" "$RAW_BIN" "$ROMFILE"

    # --- Compile ---
    err=$("$ROOT/cc09" "$f" -PIq 2>&1) || true
    if [ ! -f "$hex" ]; then
        printf "%-26s COMPILE ERROR\n" "$name"
        printf "  %s\n" "$err"
        COMPILE_FAIL=$((COMPILE_FAIL+1))
        FAIL_FILES=$((FAIL_FILES+1))
        continue
    fi

    # --- Convert HEX to binary and patch in autostart preamble ---
    #
    # objcopy maps Intel HEX addresses directly into the binary.
    # With CODE_ORG=$C003, the binary is:
    #   offset 0x0000 ($C000): 0xFF gap fill
    #   offset 0x0001 ($C001): 0xFF gap fill
    #   offset 0x0002 ($C002): 0xFF gap fill
    #   offset 0x0003 ($C003): first byte of ?begin (LDS opcode $10)
    #   ...
    #   offset 0x1FF0 ($DFF0): vector table (8 FDB entries -> ?begin at $C003)
    #
    # CoCo Extended BASIC autostart preamble (at $C000):
    #   $C000 = $55        magic byte
    #   $C001 = $C0        execution address high byte
    #   $C002 = $03        execution address low byte  -> JSR $C003
    #
    objcopy -I ihex -O binary --gap-fill 0xFF "$hex" "$RAW_BIN"

    python3 - "$RAW_BIN" "$ROMFILE" <<'PYEOF'
import sys

src_path = sys.argv[1]
dst_path = sys.argv[2]

with open(src_path, "rb") as f:
    data = bytearray(f.read())

# objcopy binary starts at $C003 (CODE_ORG), prepend 3 bytes
# to shift it back to $C000 so offsets align correctly
data = bytearray([0xFF, 0xFF, 0xFF]) + data

# Pad or trim to exactly 8192 bytes ($C000-$DFFF)
data = data[:8192]
while len(data) < 8192:
    data.append(0xFF)

# Patch autostart preamble at offsets 0-2 (= $C000-$C002)
data[0] = 0x55   # CoCo autostart magic
data[1] = 0xC0   # execution address high  \
data[2] = 0x03   # execution address low    -> $C003 = ?begin

with open(dst_path, "wb") as f:
    f.write(data)
PYEOF

    # --- Run in MAME (headless, no throttle) ---
    mame coco2b \
        -video none \
        -sound none \
        -nothrottle \
        -skip_gameinfo \
        -seconds_to_run "$TIMEOUT_SECS" \
        -cart "$ROMFILE" \
        -autoboot_script "$LUA_SCRIPT" \
        >/dev/null 2>&1 || true

    # --- Parse output ---
    if [ ! -f "$OUTFILE" ]; then
        printf "%-26s NO OUTPUT (check MAME/ROMs/Lua)\n" "$name"
        FAIL_FILES=$((FAIL_FILES+1))
        continue
    fi

    out=$(cat "$OUTFILE" | tr -d '\r')
    suite=$(echo "$out" | grep "^SUITE:" | tail -1)
    p=$(echo "$out" | grep ":OK$" | wc -l | tr -d " ")
    fails=$(echo "$out" | grep ":FAIL" || true)
    nfail=$(echo "$fails" | grep -c ":FAIL" || true)

    PASS_TESTS=$((PASS_TESTS+p))
    FAIL_TESTS=$((FAIL_TESTS+nfail))

    if echo "$suite" | grep -q "SUITE:PASS"; then
        printf "%-26s %s\n" "$name" "$suite"
        PASS_FILES=$((PASS_FILES+1))
    else
        printf "%-26s %s\n" "$name" "${suite:-TIMEOUT or NO SUITE LINE}"
        echo "$fails" | while read line; do
            [ -n "$line" ] && printf "  %s\n" "$line"
        done
        FAIL_FILES=$((FAIL_FILES+1))
    fi

    rm -f "$hex"
done

echo ""
echo "Files : $PASS_FILES passed, $FAIL_FILES failed"
echo "Tests : $PASS_TESTS passed, $FAIL_TESTS failed"

if [ "$FAIL_FILES" -gt 0 ] || [ "$COMPILE_FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
