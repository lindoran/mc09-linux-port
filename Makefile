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
#   cc09 prog.c [-O optimizer] [-I Intel hex] [S=6809RLP.ASM] [-q quiet]
#
# Targets:
#   make                  build all tools and regenerate target lib09 directories
#   make test             compile and run the full test suite under usim09
#   make test TEST=t06    run a single test suite by prefix
#   make test-coco        compile and run the full test suite under MAME coco2b
#   make test-coco TEST=t06  run a single suite under MAME
#   make env              generate env.sh — sourceable environment setup
#   make clean            remove build outputs and generated target lib09 dirs
#   make install          install to PREFIX (default /usr/local)

PREFIX    ?= /usr/local
USIM09    ?= $(shell which usim09 2>/dev/null || echo ./usim09)

CC        = gcc
CFLAGS    = -g -std=c99 -pedantic -Wall -Wextra -I.
LDFLAGS   = -g

# Test filter — empty runs all suites; set to a prefix to run one:
#   make test TEST=t06
#   make test-coco TEST=t06
TEST ?=

_LIB09_SRCS := $(wildcard lib09/*.ASM lib09/*.LIB drivers/*.asm)

.PHONY: all test test-coco coco coco-test usim09-target env clean install

all: mcc09 asm09 mco09 slink slib sindex sconvert cc09 mcp macro \
     coco coco-test usim09-target env

# ── target lib09 generation ────────────────────────────────────────────────
# All target lib09 directories are generated from their config files via
# mktarget. They rebuild whenever a config, base lib09 source, or driver
# changes. `make clean` removes the generated directories; `make all`
# recreates them.
#
# coco_test uses the TESTBUF_WRIDX/TESTBUF_BASE keys in its config so
# mktarget generates the BSS clear loop and WRIDX init automatically —
# no manual post-mktarget edits to 6809RLP.ASM are needed.

targets/coco/lib09/EXTINDEX.LIB: targets/coco/coco.cfg mktarget $(_LIB09_SRCS)
	python3 mktarget targets/coco/coco.cfg targets/coco

coco: targets/coco/lib09/EXTINDEX.LIB

targets/coco_test/lib09/EXTINDEX.LIB: targets/coco_test/coco_test.cfg mktarget $(_LIB09_SRCS)
	python3 mktarget targets/coco_test/coco_test.cfg targets/coco_test

coco-test: targets/coco_test/lib09/EXTINDEX.LIB

targets/usim09/lib09/EXTINDEX.LIB: targets/usim09/usim09.cfg mktarget $(_LIB09_SRCS)
	python3 mktarget targets/usim09/usim09.cfg targets/usim09

usim09-target: targets/usim09/lib09/EXTINDEX.LIB

# ── build ──────────────────────────────────────────────────────────────────

mcc09: compile.c io.c 6809cg.c compile.h tokens.h portab.h 6809cg.h
	$(CC) $(CFLAGS) -DCPU=6809 -o $@ compile.c io.c 6809cg.c

asm09: asm09.c xasm.h portab.h
	$(CC) $(CFLAGS) -o $@ asm09.c

mco09: mco.c 6809.mco microc.h portab.h
	$(CC) $(CFLAGS) -DCPU=6809 -o $@ mco.c

slink: slink.c microc.h portab.h
	$(CC) $(CFLAGS) -o $@ slink.c

slib: slib.c microc.h portab.h
	$(CC) $(CFLAGS) -o $@ slib.c

sindex: sindex.c microc.h portab.h
	$(CC) $(CFLAGS) -o $@ sindex.c

sconvert: sconvert.c microc.h portab.h
	$(CC) $(CFLAGS) -o $@ sconvert.c

cc09: cc09.c
	$(CC) $(CFLAGS) -o $@ cc09.c

mcp: mcp.c microc.h
	$(CC) $(CFLAGS) -o $@ mcp.c

macro: macro.c xasm.h portab.h
	$(CC) $(CFLAGS) -o $@ macro.c

# ── test suite (usim09) ────────────────────────────────────────────────────
# Compiles each tests/t*.c with cc09 against the usim09 target and runs the
# resulting HEX file under the usim09 simulator.
#
# Full suite:        make test
# Single suite:      make test TEST=t06
#
# Requires usim09 on PATH, or present in the root directory.

test: all
	@MCDIR=. \
	 MCINCLUDE=./include \
	 MCLIBDIR=./targets/usim09/lib09 \
	 USIM=$(USIM09) \
	 sh tests/run_tests.sh $(TEST)

# ── test suite (MAME coco2b) ───────────────────────────────────────────────
# Compiles each tests/t*.c with cc09 against the coco_test target, builds a
# CoCo cartridge ROM with the correct autostart preamble, and runs it
# headlessly under MAME coco2b. A Lua monitor script captures the output
# from the linear RAM buffer and exits MAME when SUITE:PASS/FAIL is seen.
#
# Full suite:        make test-coco
# Single suite:      make test-coco TEST=t06
#
# Requires:
#   mame on PATH with coco2b ROMs (mame -verifyroms coco2b)
#   objcopy on PATH (binutils)
#   python3 on PATH

test-coco: all coco-test
	@MCDIR=. \
	 MCINCLUDE=./include \
	 MCLIBDIR=./targets/coco_test/lib09 \
	 sh tests/run_tests_coco.sh $(TEST)

# ── env.sh: sourceable environment setup ───────────────────────────────────

env: env.sh

env.sh: Makefile
	@printf '# mc09 environment — generated by make env\n' > env.sh
	@printf '# source this file: . ./env.sh\n' >> env.sh
	@printf '# then optionally set MC09_TARGET=coco before sourcing\n\n' >> env.sh
	@printf 'MCDIR="$$(cd "$$(dirname "$${BASH_SOURCE[0]:-$$0}")" && pwd)"\n' >> env.sh
	@printf 'export MCDIR\n\n' >> env.sh
	@printf 'export MCINCLUDE="$$MCDIR/include"\n\n' >> env.sh
	@printf '_target="$${MC09_TARGET:-default}"\n' >> env.sh
	@printf 'case "$$_target" in\n' >> env.sh
	@printf '  coco)      export MCLIBDIR="$$MCDIR/targets/coco/lib09" ;;\n' >> env.sh
	@printf '  coco_test) export MCLIBDIR="$$MCDIR/targets/coco_test/lib09" ;;\n' >> env.sh
	@printf '  usim09)    export MCLIBDIR="$$MCDIR/targets/usim09/lib09" ;;\n' >> env.sh
	@printf '  *)         export MCLIBDIR="$$MCDIR/lib09" ;;\n' >> env.sh
	@printf 'esac\n\n' >> env.sh
	@printf 'export PATH="$$MCDIR:$$PATH"\n\n' >> env.sh
	@printf 'echo "mc09: MCDIR=$$MCDIR  MCLIBDIR=$$MCLIBDIR"\n' >> env.sh
	@chmod +x env.sh
	@echo "Generated env.sh  —  use:  . ./env.sh"
	@echo "CoCo target:             MC09_TARGET=coco . ./env.sh"
	@echo "CoCo test target:        MC09_TARGET=coco_test . ./env.sh"

# ── install ────────────────────────────────────────────────────────────────

install: all env
	install -d $(PREFIX)/bin
	install -d $(PREFIX)/share/mc09/include
	install -d $(PREFIX)/share/mc09/lib09
	install -d $(PREFIX)/share/mc09/targets/usim09/lib09
	install -d $(PREFIX)/share/mc09/targets/coco/lib09
	install -d $(PREFIX)/share/mc09/targets/coco_test/lib09
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
	install -m 644 targets/coco/lib09/*.ASM \
	               targets/coco/lib09/EXTINDEX.LIB       \
	               $(PREFIX)/share/mc09/targets/coco/lib09/
	install -m 644 targets/coco/coco.cfg                 $(PREFIX)/share/mc09/targets/coco/
	install -m 644 targets/coco_test/lib09/*.ASM \
	               targets/coco_test/lib09/EXTINDEX.LIB  \
	               $(PREFIX)/share/mc09/targets/coco_test/lib09/
	install -m 644 targets/coco_test/coco_test.cfg       $(PREFIX)/share/mc09/targets/coco_test/
	install -m 644 tests/coco_monitor.lua                $(PREFIX)/share/mc09/tests/
	install -m 755 tests/run_tests_coco.sh               $(PREFIX)/share/mc09/tests/
	@# Install env.sh pointing at the installed share dir
	@sed 's|MCDIR=".*"|MCDIR="$(PREFIX)/share/mc09"|' env.sh \
	    | sed '/MCDIR=.*pwd/d' \
	    > $(PREFIX)/share/mc09/env.sh
	@chmod +x $(PREFIX)/share/mc09/env.sh
	@echo ""
	@echo "Installed to $(PREFIX).  To set up your environment:"
	@echo "  . $(PREFIX)/share/mc09/env.sh                    # default lib09"
	@echo "  MC09_TARGET=coco . $(PREFIX)/share/mc09/env.sh  # CoCo target"

# ── clean ──────────────────────────────────────────────────────────────────

clean:
	rm -f mcc09 asm09 mco09 slink slib sindex sconvert cc09 mcp macro
	rm -f tests/*.HEX tests/*.asm tests/*.lst
	rm -f examples/hello_usim09/*.HEX examples/hello_usim09/*.asm examples/hello_usim09/*.lst
	rm -f examples/hello_coco/*.HEX examples/hello_coco/*.asm examples/hello_coco/*.lst
	rm -f '$$'[0-9] *.o env.sh
	rm -rf targets/coco/lib09 targets/coco_test/lib09 targets/usim09/lib09
