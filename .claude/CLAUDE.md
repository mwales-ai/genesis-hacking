# CLAUDE.md

Refer to the readme for guidance on the point of this project and goals.

## Repository Overview

This is a collection of 3 independent top-level projects focused on security research and SEGA Genesis ROM hacking:

- **bn-genesis/**: Binary Ninja plugin for SEGA Genesis/Megadrive ROM analysis.
  This is a fork of an existing repo that we can use and expand on if we want
  to expand the features of.
- **genesis-hacking/**: Documentation and notes for Genesis ROM hacking.  This
  be the main repo we want to work in for our Sega Genesis ROM hacking tools
  and patches.

## Build Commands

### bn-genesis Plugin
No build step — install directly as a Binary Ninja plugin. Requires `gcc-m68k-linux-gnu` and the `binaryninja-m68k` processor module at runtime (used by `assemble.py` to shell out to the cross-compiler).

## Testing & Linting

There are no automated tests, test frameworks, CI/CD pipelines, or linting configurations in this repository.

## Architecture: bn-genesis Plugin

The plugin registers 4 `PluginCommand` handlers in `__init__.py`:

```
Binary Ninja host
    └── __init__.py          Entry point; registers plugin commands
        ├── loader.py        GenesisView: parses ROM header, maps memory segments
        │                    (ROM, RAM @ 0xff0000, Z80 @ 0xa00000, System IO, VDP)
        ├── assemble.py      GenesisAssemble: shells out to gcc-m68k-linux-gnu to
        │                    assemble M68k code and patch it into the open ROM
        ├── checksum.py      GenesisChecksum: recalculates and writes ROM checksum
        ├── vdp_analysis.py  VdpAnalysis: adds comments to VDP register writes
        └── call_table_enum.py  Deprecated; targets older Binary Ninja API versions
```

`loader.py` identifies Genesis ROMs by magic bytes and Genesis header fields, then builds Binary Ninja memory segments and sections accordingly.

## Build Commands

### Sprite Editor
```bash
cd sprite-editor && bash build.sh
```
Binary output: `sprite-editor/build/SpriteEditor`

### BlastEm (forked, at ../blastem/)
```bash
cd ../blastem && make
```
Binary output: `../blastem/blastem`

## System Dependencies

- `gcc-m68k-linux-gnu` — M68k cross-compiler (required by bn-genesis assemble)
- `qt6-base-dev` — Qt6 for C++ GUI tools (sprite editor)
- Binary Ninja (proprietary) — required to use bn-genesis or binja_scripts
- `blastem` — Genesis emulator with built-in debugger (forked at ../blastem/)
