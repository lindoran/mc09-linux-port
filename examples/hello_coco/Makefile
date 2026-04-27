# Makefile for hello_coco example
#
# Builds hello.c as a CoCo cartridge ROM and runs it under MAME coco2b.
# Uses the `coco` target (coco_rom driver) so output appears on the
# CoCo screen in the MAME window via CHROUT.
#
# Usage:
#   make          build hello.rom (with autostart preamble)
#   make run      run in MAME (windowed — text appears on CoCo screen)
#   make clean    remove generated files
#
# Requirements:
#   mame on PATH with coco2b ROMs  (mame -verifyroms coco2b)
#   objcopy on PATH (binutils)
#   python3 on PATH

ROOT     := ../..
MCDIR    := $(ROOT)
MCINCLUDE:= $(ROOT)/include
MCLIBDIR := $(ROOT)/targets/coco/lib09
CC09     := $(ROOT)/cc09

export MCDIR MCINCLUDE MCLIBDIR

# Entry address: CODE_ORG = $C003 (?begin startup, right after preamble)
ENTRY_HI := C0
ENTRY_LO := 03

.PHONY: all run clean

all: hello.rom

# Step 1: Compile C -> Intel HEX
hello.HEX: hello.c $(MCLIBDIR)/EXTINDEX.LIB
	$(CC09) hello.c -Iq

# Step 2: Convert HEX -> raw binary, prepend 3 gap bytes so offset 0
#         aligns to $C000, patch in autostart preamble, pad to 8192 bytes.
#
# Why prepend 3 bytes:
#   objcopy produces a binary where byte 0 = lowest HEX address = $C003.
#   We need byte 0 = $C000, so we shift the whole binary right by 3.
#   Then bytes 0-2 ($C000-$C002) are $FF gap fill, ready to be patched.
hello.rom: hello.HEX
	objcopy -I ihex -O binary --gap-fill 0xFF hello.HEX hello_raw.bin
	python3 -c "\
src = bytearray(open('hello_raw.bin','rb').read()); \
data = bytearray([0xFF,0xFF,0xFF]) + src; \
data = data[:8192]; \
data += bytearray([0xFF]*(8192-len(data))); \
data[0]=0x55; data[1]=0x$(ENTRY_HI); data[2]=0x$(ENTRY_LO); \
open('hello.rom','wb').write(data)"
	@echo ""
	@echo "Built hello.rom (8192 bytes)"
	@echo "  Preamble  : \$$C000=0x55  \$$C001=0x$(ENTRY_HI)  \$$C002=0x$(ENTRY_LO)"
	@echo "  Entry addr: \$$$(ENTRY_HI)$(ENTRY_LO) (?begin startup)"
	@python3 -c "\
data=open('hello.rom','rb').read(); \
print('  RESET vec : \$$%04X' % (data[0x1FFE]*256+data[0x1FFF]))"
	@rm -f hello_raw.bin

# Run in MAME windowed — text appears on the CoCo screen via CHROUT
run: hello.rom
	mame coco2b \
	    -window \
	    -skip_gameinfo \
	    -cart hello.rom

clean:
	rm -f hello.HEX hello.asm hello.lst hello_raw.bin hello.rom

# Regenerate target lib09 if it doesn't exist
$(MCLIBDIR)/EXTINDEX.LIB:
	cd $(ROOT) && python3 mktarget targets/coco/coco.cfg targets/coco
