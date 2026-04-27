# hello_coco — Micro-C 6809 Hello World for CoCo Cartridge

Builds a complete CoCo 2B cartridge ROM from a C source file and runs it
under MAME's `coco2b` emulation. The cartridge autostarts immediately on
boot and outputs text to the CoCo screen via the BASIC CHROUT ROM vector.
After `main()` returns, control passes back to the BASIC `OK` prompt.

## Quick start

```sh
make          # compile hello.c -> hello.rom
make run      # launch MAME coco2b windowed
```

You will see the CoCo boot screen briefly, then:

```
Hello from Micro-C 6809!
Count: 1 2 3 4 5
```

After printing, the program spins (`BRA *`). The CoCo stays alive — press
the reset button or close MAME to exit. Returning to BASIC is not possible
because BASIC would see `$55` at `$C000` and immediately re-launch the
cartridge, causing an infinite loop.

## Two CoCo targets explained

| Target | Driver | CODE_ORG | Output | Exit | Use for |
|--------|--------|----------|--------|------|---------|
| `coco` | `coco_rom` | `$C003` | CHROUT → screen | Return to BASIC | Normal cartridge dev |
| `coco_test` | `coco_test` | `$C003` | RAM buffer → MAME Lua | Spin | Automated MAME tests |

This example uses the `coco` target. The `coco_test` target is used by
`make test-coco` / `tests/run_tests_coco.sh`.

## Build pipeline in detail

### Step 1 — Compile C to Intel HEX

```sh
MCDIR=../.. MCINCLUDE=../../include MCLIBDIR=../../targets/coco/lib09 \
    ../../cc09 hello.c -Iq
```

Produces `hello.HEX`. The lowest address in the HEX file is `$C003`
(`CODE_ORG` from `coco.cfg`). Bytes `$C000–$C002` are absent — `objcopy`
fills them with `$FF`.

The startup code `mktarget` generated at `$C003`:

```asm
?begin  LDS     #$7F00          initialise stack
* Clear BSS: zero RAM from RAM_ORG to STACK_TOP
* Required on real hardware — RAM is not zeroed at reset
        LDX     #$0600
        LDD     #0
?clr1   STD     ,X++
        CMPX    #$7F00
        BLO     ?clr1
        CLR     ?heap           zero heap pointer
        JSR     main            call user program
* Return from main — jump to BASIC warm start
?exit1  JMP     $A027           -> BASIC OK prompt
```

`BSS_CLEAR=1` in `coco.cfg` is what causes `mktarget` to emit the clear
loop. Without it, global variables contain whatever BASIC left in RAM.

### Step 2 — Convert HEX to binary and patch the preamble

```sh
objcopy -I ihex -O binary --gap-fill 0xFF hello.HEX hello_raw.bin
```

`objcopy` produces a flat binary where byte 0 = `$C003` (the first HEX
address). We need byte 0 = `$C000`, so the Python step prepends 3 `$FF`
bytes to shift everything into alignment, then patches the preamble:

```
Offset  Address  Value   Meaning
  0     $C000    $55     CoCo Extended BASIC autostart magic
  1     $C001    $C0     Execution address high byte \
  2     $C002    $03     Execution address low byte   -> JSR $C003
  3     $C003    $10     First byte of LDS #$7F00 (?begin)
  ...
0x1FF0  $DFF0    ...     6809 vector table (8 × FDB ?begin)
0x1FFE  $DFFE    $C0     RESET vector high \
0x1FFF  $DFFF    $03     RESET vector low   -> $C003
```

### Step 3 — Run in MAME

```sh
mame coco2b -window -skip_gameinfo -cart hello.rom
```

MAME maps `hello.rom` to `$C000–$DFFF`, resets the 6809, and the RESET
vector points to `$C003`. BASIC initialises, sees `$55` at `$C000`, and
does `JSR [$C001]` which calls `$C003` — our startup code.

## Memory map

```
$0000-$05FF  BASIC workspace (do not use)
$0600-$7EFF  User RAM — globals ($0600 up), heap grows up, stack grows down
$7F00        Stack top
$A000        POLCAT vector (poll keyboard)
$A002        CHROUT vector (output char in A to screen)
$C000        Autostart magic $55
$C001-$C002  Execution address $C003
$C003-$DFEF  Cartridge code (startup + your program + runtime)
$DFF0-$DFFF  6809 vector table (all -> $C003)
$E000-$FFFF  Extended BASIC ROM
```

## Verifying the ROM

```sh
python3 -c "
data = open('hello.rom','rb').read()
print('Preamble  :', hex(data[0]), hex(data[1]), hex(data[2]))
print('RESET vec :', hex(data[0x1FFE]*256 + data[0x1FFF]))
"
```

Should print:

```
Preamble  : 0x55 0xc0 0x3
RESET vec : 0xc003
```

## Common issues

**No autostart (drops to BASIC OK):** Preamble not patched. Verify with
the python check above — `0xff 0xff 0xff` means the 3-byte prepend was
skipped, so the preamble landed at `$C003` instead of `$C000`.

**Crashes or garbled output:** BSS not cleared. Check that `BSS_CLEAR=1`
is in `targets/coco/coco.cfg` and that `6809RLP.ASM` in
`targets/coco/lib09/` contains the `?clr1` loop.

**`RESET vec : 0xffff`:** Same prepend problem — the vector table landed
3 bytes too high. Run `make clean && make`.
