# Makefile for Dunfield Micro-C 6809 toolchain — Linux port
#
# Tools:
#   cc09      Command coordinator (mcc09 + slink + asm09 in one command)
#   mcc09     Micro-C 6809 cross-compiler
#   mco09     Peephole optimizer
#   asm09     6809 cross-assembler
#   slink     Source linker (resolves $EX: externals from lib09/)
#   slib      Source library manager
#   sindex    Source library index builder
#   sconvert  Source file converter
#
# Pipeline (manual):
#   mcc09 -I./include prog.c prog.asm
#   slink prog.asm l=./lib09 prog_linked.asm
#   asm09 prog_linked.asm l=prog.lst c=prog.HEX
#
# Pipeline (cc09 coordinator):
#   export MCDIR=/path/to/mc09-linux-port
#   export MCINCLUDE=$$MCDIR/include
#   export MCLIBDIR=$$MCDIR/lib09           # or targets/usim09/lib09
#   cc09 prog.c [-O optimizer] [-I Intel hex] [S=crt0] [-q quiet]
#
# make              build all tools
# make test         hello end-to-end (manual pipeline, default lib)
# make test-usim    hello via cc09, usim09 target, run in simulator
# make clean        remove build outputs
# make install      install to PREFIX (default /usr/local)

PREFIX    ?= /usr/local
USIM09    ?= $(shell which usim09 2>/dev/null || \
               echo /tmp/small-focus-testing-micro-c/modules/usim/usim09)

CC        = gcc
CFLAGS    = -std=gnu89 \
            -Wno-implicit-int \
            -Wno-implicit-function-declaration \
            -Wno-pointer-sign \
            -Wno-return-type \
            -Wno-unused-function \
            -I.

.PHONY: all test test-usim clean install

all: mcc09 asm09 mco09 slink slib sindex sconvert cc09 mcp macro

# ── build ──────────────────────────────────────────────────────────────────

mcc09: compile.c io.c 6809cg.c compile.h tokens.h portab.h
	$(CC) $(CFLAGS) -DCPU=6809 -o $@ compile.c io.c 6809cg.c

asm09: asm09.c xasm.h portab.h
	$(CC) $(CFLAGS) -o $@ asm09.c

mco09: mco.c 6809.mco microc.h
	$(CC) $(CFLAGS) -DCPU=6809 -o $@ mco.c

slink: slink.c microc.h
	$(CC) $(CFLAGS) -o $@ slink.c

slib: slib.c microc.h
	$(CC) $(CFLAGS) -o $@ slib.c

sindex: sindex.c microc.h
	$(CC) $(CFLAGS) -o $@ sindex.c

sconvert: sconvert.c microc.h
	$(CC) $(CFLAGS) -o $@ sconvert.c

cc09: cc09.c microc.h
	$(CC) $(CFLAGS) -o $@ cc09.c

mcp: mcp.c microc.h
	$(CC) $(CFLAGS) -o $@ mcp.c

macro: macro.c xasm.h portab.h
	$(CC) $(CFLAGS) -o $@ macro.c

# ── smoke test: manual pipeline ───────────────────────────────────────────

test: mcc09 asm09 slink
	@echo "=== Compiling hello.c ==="
	./mcc09 -I./include hello.c hello.asm
	@echo "=== Linking ==="
	./slink hello.asm l=./lib09 hello_linked.asm
	@echo "=== Assembling ==="
	./asm09 hello_linked.asm l=hello.lst c=hello.HEX
	@echo "=== hello.HEX ==="
	@cat hello.HEX

# ── usim09 test: cc09 coordinator, usim09 target ─────────────────────────

test-regen: mktarget
	@echo "=== Regenerating usim09 target from config ==="
	python3 mktarget targets/usim09/usim09.cfg /tmp/mc09-regen-test
	@echo "=== Verifying generated target assembles correctly ==="
	./mcc09 -I./include hello_clean.c hello_regen.asm
	./slink hello_regen.asm s=CRT0.ASM l=/tmp/mc09-regen-test/lib09 hello_regen_lnk.asm
	./asm09 hello_regen_lnk.asm -I l=hello_regen.lst c=hello_regen.HEX
	@grep -c error hello_regen.lst || echo "0 errors"
	@echo "" | timeout 5 $(USIM09) hello_regen.HEX || true
	@rm -f hello_regen.asm hello_regen_lnk.asm hello_regen.lst hello_regen.HEX

test-usim: all
	@echo "=== cc09 -> usim09 (single command) ==="
	MCDIR=. MCINCLUDE=./include MCLIBDIR=./targets/usim09/lib09 \
	  ./cc09 hello_clean.c -Iq S=CRT0.ASM
	@echo ""
	@echo "=== Running hello_clean.HEX in usim09 ==="
	@echo "" | timeout 5 $(USIM09) hello_clean.HEX || true

# ── install ────────────────────────────────────────────────────────────────

install: all
	install -d $(PREFIX)/bin
	install -d $(PREFIX)/share/mc09/include
	install -d $(PREFIX)/share/mc09/lib09
	install -d $(PREFIX)/share/mc09/targets/usim09/lib09
	install -m 755 mcc09 asm09 mco09 slink slib sindex sconvert cc09 mcp macro \
	               $(PREFIX)/bin/
	install -m 755 mc09pp mktarget $(PREFIX)/bin/
	install -m 644 include/*.h                           $(PREFIX)/share/mc09/include/
	install -m 644 lib09/[A-Z]*.ASM lib09/EXTINDEX.LIB  $(PREFIX)/share/mc09/lib09/
	install -d $(PREFIX)/share/mc09/drivers
	install -m 644 drivers/*.asm                         $(PREFIX)/share/mc09/drivers/
	install -m 644 targets/usim09/lib09/*.ASM \
	               targets/usim09/lib09/EXTINDEX.LIB     \
	               $(PREFIX)/share/mc09/targets/usim09/lib09/
	install -m 644 targets/usim09/usim09.cfg             $(PREFIX)/share/mc09/targets/usim09/
	install -d $(PREFIX)/share/mc09/targets/coco
	install -m 644 targets/coco/coco.cfg                 $(PREFIX)/share/mc09/targets/coco/
	@echo ""
	@echo "Installed. Typical use:"
	@echo "  export MCDIR=$(PREFIX)/share/mc09"
	@echo "  export MCINCLUDE=\$$MCDIR/include"
	@echo "  export MCLIBDIR=\$$MCDIR/lib09          # default target"
	@echo "  # or: MCLIBDIR=\$$MCDIR/targets/usim09/lib09  # usim09 target"
	@echo "  cc09 prog.c -Iq S=CRT0.ASM            # compile for usim09"

# ── clean ──────────────────────────────────────────────────────────────────

clean:
	rm -f mcc09 asm09 mco09 slink slib sindex sconvert cc09 mcp macro
	rm -f hello.asm hello_linked.asm hello.lst hello.HEX
	rm -f hello_clean.asm hello_clean.HEX hello_test.asm hello_test_opt.asm
	rm -f hello_usim_src.asm hello_usim_lnk.asm hello_usim.lst hello_usim.HEX
	rm -f hello_clean_usim.asm hello_clean_usim.lst hello_clean_usim.HEX
	rm -f test_arith.asm test_arith_linked.asm test_arith_linked.lst test_arith_linked.HEX
	rm -f '$$'[0-9] *.o
