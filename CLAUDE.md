# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Refer to the readme for guidance on the point of this project and goals.

## Repository Overview

This is a collection of 3 independent top-level projects focused on security research and SEGA Genesis ROM hacking:

- **bn-genesis/**: Binary Ninja plugin for SEGA Genesis/Megadrive ROM analysis.
  This is a fork of an existing repo that we can use and expand on if we want
  to expand the features of.
- **genesis-hacking/**: Documentation and notes for Genesis ROM hacking.  This
  be the main repo we want to work in for our Sega Genesis ROM hacking tools
  and patches.
- **security/**: Security toolkit with CTF solutions, reverse engineering
  utilities, and analysis scripts.  This should be use just to get an example
  of how I like to make tools (small python scripts, C or C++ applications,
  with GUI development in Qt or SDL)

## Build Commands

### Qt/C++ Utilities (FindFloats, ReverseCRC, QemuConfigTool, etc.)
```bash
cd security/utils/<tool-name>
bash build.sh  # runs qmake + make -j4
```

### Simple C++ Tools
```bash
g++ --std=c++11 -o caesar security/utils/caesar/caesar.cpp
```

### Cross-architecture Type Analysis
```bash
cd security/type_analysis
bash build.sh  # builds both ARM and x86 variants
```

### DEFCON Badge (Arduino)
```bash
cd security/defcon_badge
~/apps/arduino-cli compile sao_badge
~/apps/arduino-cli upload -p /dev/ttyACM0 sao_badge
```

### bn-genesis Plugin
No build step — install directly as a Binary Ninja plugin. Requires `gcc-m68k-linux-gnu` and the `binaryninja-m68k` processor module at runtime (used by `assemble.py` to shell out to the cross-compiler).

### CTF Scripts
Standalone Python/Bash scripts — run directly with `python3 script.py` or `bash script.sh`.

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

## Architecture: security/utils

17 standalone tools, each self-contained:

| Tool | Type | Purpose |
|------|------|---------|
| FindFloats | C++/Qt GUI | Hex → float/double conversion |
| ReverseCRC | C++/Qt GUI | Brute-force CRC-8/CRC-16 seeds |
| caesar | C++ CLI | Caesar cipher solver |
| aesSource | C++/Qt | AES implementation |
| QemuConfigTool | C++/Qt GUI | QEMU process/QMP socket manager |
| outputJudge | Python | CTF challenge output verifier |
| ubootDump2Bin | Python | U-Boot flash dump with CRC validation |
| entropy | C/Python | File entropy analysis |
| xorFiles | C | XOR two files |
| endian-fix (ef.py) | Python | Endianness conversion |

## Architecture: security/ctf

24+ competition writeup directories (33C3, 34C3, defcon30, picoctf_2025, etc.). Each is an independent folder with ad-hoc Python/Bash scripts and notes for specific challenges. No shared infrastructure between competitions.

## System Dependencies

- `gcc-m68k-linux-gnu` — M68k cross-compiler (required by bn-genesis assemble)
- `qt5-qmake`, `qtbase5-dev` — Qt5 for C++ GUI tools
- Binary Ninja (proprietary) — required to use bn-genesis or binja_scripts
- Arduino CLI at `~/apps/arduino-cli` — required for defcon_badge
