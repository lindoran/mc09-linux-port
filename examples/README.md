# mc09 Examples

This directory contains self-contained example projects that demonstrate
how to build and run Micro-C 6809 programs against each supported target.

## Examples

| Directory | Target | What it demonstrates |
|-----------|--------|----------------------|
| `hello_usim09/` | usim09 simulator | Simplest possible build and run |
| `hello_coco/`   | MAME coco2b      | CoCo cartridge ROM with autostart preamble |

## Prerequisites

Build the toolchain first from the repo root:

```sh
cd ..
make
```

Both examples assume the mc09 tools (`cc09`, `objcopy`, `python3`) are on
PATH or present in the repo root. See the top-level README for full
dependency details.
