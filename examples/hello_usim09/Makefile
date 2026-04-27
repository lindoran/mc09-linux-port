# Makefile for hello_usim09 example
#
# Builds hello.c against the usim09 target and runs it under usim09.
#
# Usage:
#   make          build hello.HEX
#   make run      build and run under usim09
#   make clean    remove generated files
#
# Exit: usim09 spins after main() returns (EXIT_MODE=0). The simulator
# exits when its stdin reaches EOF. The `run` target pipes an empty
# string so EOF arrives immediately after program output is complete.
# To exit interactively if running usim09 directly: Ctrl+C.

ROOT     := ../..
MCDIR    := $(ROOT)
MCINCLUDE:= $(ROOT)/include
MCLIBDIR := $(ROOT)/targets/usim09/lib09
CC09     := $(ROOT)/cc09
USIM     := $(shell which usim09 2>/dev/null || echo $(ROOT)/usim09)

export MCDIR MCINCLUDE MCLIBDIR

.PHONY: all run clean

all: hello.HEX

hello.HEX: hello.c $(MCLIBDIR)/EXTINDEX.LIB
	$(CC09) hello.c -Iq

# Pipe empty stdin so usim09 sees EOF and exits after output completes.
run: hello.HEX
	echo "" | timeout 1 $(USIM) hello.HEX 2>/dev/null | tr -d '\r'

clean:
	rm -f hello.HEX hello.asm hello.lst

# Regenerate the target lib09 if it doesn't exist
$(MCLIBDIR)/EXTINDEX.LIB:
	cd $(ROOT) && python3 mktarget targets/usim09/usim09.cfg targets/usim09
