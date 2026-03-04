# BlastEm Debug Features Guide

BlastEm is a Genesis/Mega Drive emulator with extensive built-in debugging tools: graphical debug views, an interactive command-line debugger, and DMA tracing. This guide covers the features available in our fork (github.com:mwales-ai/blastem.git).

## Getting Started

### Entering the Debugger

There are three ways to enter the interactive debugger:

1. **Command line** — start with `-d` to break at the first instruction:
   ```
   blastem -d rom.bin
   ```

2. **At runtime** — press `u` (default binding for `ui.enter_debugger`) while the game is running. The emulator pauses and drops to the debugger prompt in the terminal.

3. **Breakpoints/watchpoints** — execution stops automatically when a breakpoint or watchpoint is hit.

The debugger prompt looks like:
```
68K 00:ings00200>
```
showing the current PC address. Type `help` to list all commands.

### Command Line Flags

| Flag | Description |
|------|-------------|
| `-d` | Enter debugger on startup |
| `-D` | Enter GDB remote debugger on startup |
| `--dma-log FILE` | Log all VDP DMA transfers to a file |
| `--dma-history SIZE` | Set DMA history ring buffer size (default: 1024, minimum: 16) |

## Debug Views

BlastEm provides several graphical debug windows, toggled by key bindings. Each opens a separate window.

| Key | Binding Name | View |
|-----|-------------|------|
| `b` | `ui.plane_debug` | **Plane Debug** — shows Plane A, Plane B, Window, and Sprites layers |
| `v` | `ui.vram_debug` | **VRAM Debug** — shows all tiles in Video RAM |
| `c` | `ui.cram_debug` | **CRAM Debug** — shows all 4 palette lines (64 colors) |
| `n` | `ui.compositing_debug` | **Composite Debug** — shows the composited output with layer info |
| `o` | `ui.oscilloscope` | **Oscilloscope** — audio channel waveform display |
| `[` | `ui.vdp_debug_mode` | Cycle through sub-modes within the current debug view |

Press the same key again to close the debug window. Key bindings can be remapped in the BlastEm configuration file.

### Plane Debug View

The Plane Debug window (`b`) has four sub-modes cycled with `[`:
- **Plane A** — background layer A
- **Plane B** — background layer B
- **Window** — window plane overlay
- **Sprites** — all sprites rendered at their screen positions

In **Sprites** mode, hover the mouse over a sprite to see the **Sprite Inspector** panel.

### VRAM Debug View

Shows all 2048 tiles (8x8 pixel patterns) stored in VRAM, arranged in a grid. Useful for finding tile indices and verifying DMA transfers loaded the expected graphics.

### CRAM Debug View

Shows all 64 color entries across 4 palette lines. Each line has 16 colors (color 0 in each line is the transparent/background color). CRAM values are 9-bit Genesis format (3 bits each for R, G, B).

### Composite Debug View

Shows the final composited frame with additional debug information about which layer contributed each pixel.

### Oscilloscope

Displays audio channel waveforms from the YM-2612 and PSG. Useful for debugging sound driver issues.

## Sprite Inspector

When the **Plane Debug** view is in **Sprites** mode, hovering over a sprite displays an information panel with:

| Field | Description |
|-------|-------------|
| **Sprite #N** | Sprite index in the Sprite Attribute Table (SAT) |
| **X / Y** | Screen position (SAT value minus 128) |
| **Size** | Width x Height in pixels |
| **Pat** | Tile pattern index (first tile of the sprite) |
| **VRAM** | Byte address of the tile data in VRAM |
| **Pal / Pri** | Palette line (0-3) and priority bit |
| **Flip H/V** | Horizontal and vertical flip flags |
| **DMA** | ROM or RAM source address of the tile data (from DMA history) |
| **Palette swatches** | Row of 16 colored rectangles showing the sprite's palette colors |
| **CRAM** | Byte address range in CRAM for this palette line |
| **Pal DMA** | ROM or RAM source of the palette DMA transfer |

### Interpreting DMA Source Addresses

- **DMA:ROM $0xxxxx** — tile data was DMA'd from ROM. This is the offset in the ROM file where the sprite graphics live. Use this with a hex editor or Binary Ninja to find and modify sprite art.
- **DMA:RAM $FFxxxx** — tile data was DMA'd from 68K work RAM. The game decompressed or generated the tiles at runtime.
- **DMA: not found** — no DMA transfer to this VRAM address was recorded. The data may have been written via FIFO (CPU writes to VDP data port) or the DMA history buffer wrapped.

The same logic applies to **Pal DMA** for palette data.

## Debugger Commands

### Execution Control

| Command | Aliases | Description |
|---------|---------|-------------|
| `continue` | `c` | Resume execution |
| `step` | `s` | Step one instruction (into subroutines) |
| `over` | | Step over subroutines |
| `next` | | Advance to next instruction address |
| `advance ADDRESS` | | Run until ADDRESS is reached |
| `frames N` | | Resume for N video frames, then break |
| `quit` | | Quit BlastEm |
| `softreset` | `sr` | Soft-reset the system |

### Breakpoints and Watchpoints

| Command | Description |
|---------|-------------|
| `breakpoint ADDRESS` or `b ADDRESS` | Set a breakpoint at ADDRESS |
| `watchpoint ADDRESS [SIZE]` | Set a write watchpoint. SIZE defaults to 2 for even addresses, 1 for odd |
| `readwatch ADDRESS [SIZE]` or `rw ADDRESS [SIZE]` | Set a read watchpoint. Breaks when the address is read |
| `delete N` or `d N` | Remove breakpoint/watchpoint by index |
| `condition N EXPR` | Make breakpoint N conditional on EXPR |
| `commands N` | Set commands to run when breakpoint N is hit |

When a watchpoint fires, it reports the old and new values:
```
68K Watchpoint 1 hit, old value: 0, new value 42
```

### Inspection

| Command | Aliases | Description |
|---------|---------|-------------|
| `print EXPR` | `p` | Print an expression (registers, memory, math) |
| `printf FMT, ARGS...` | | Printf-style formatted output |
| `display EXPR` | | Print EXPR every time the debugger is entered |
| `deletedisplay N` | `dd N` | Remove a display expression |
| `disassemble [ADDRESS]` | `disasm` | Disassemble instructions at ADDRESS (default: PC) |
| `backtrace` | `bt` | Print call stack |
| `symbols [FILE]` | | Load symbol file or list loaded symbols |

### Registers

Access registers directly in expressions: `d0`-`d7`, `a0`-`a7`, `sr`, `pc`, `sp` (alias for `a7`).

Examples:
```
print d0
print (a0)        # read word at address in a0
print (a0).b      # read byte at address in a0
print (a0).l      # read long at address in a0
set d0 = $1234
set ($FF0000).w = $ABCD
```

### Variables and Control Flow

| Command | Description |
|---------|-------------|
| `variable NAME VALUE` | Create a debugger variable |
| `array NAME` | Create an array variable |
| `append NAME VALUE` | Append to an array |
| `set TARGET = VALUE` | Set register, memory, or variable |
| `if EXPR` | Conditional block |
| `while EXPR` | Loop block |
| `function NAME` | Define a user function |
| `return [VALUE]` | Return from a function |

### File I/O

| Command | Description |
|---------|-------------|
| `save NAME FILE [START LEN]` | Save array or memory range to file |
| `load FILE NAME/ADDRESS [LEN]` | Load file into array or memory |

### VDP and Genesis-Specific

| Command | Aliases | Description |
|---------|---------|-------------|
| `vdpsprites` | `vs` | Print the VDP sprite attribute table |
| `vdpregs` | `vr` | Print all VDP register values |
| `vdpregbreak REG [VALUE]` | `vrb` | Break on VDP register write |
| `dmatrace` | `dmat` | Log DMA transfers to the terminal |
| `ymchannel [N]` | `yc` | Print YM-2612 channel info |
| `ymtimer` | `yt` | Print YM-2612 timer info |
| `z80` | | Switch to Z80 debugger context |

### Input Simulation

| Command | Description |
|---------|-------------|
| `bindup BINDING` | Simulate key-up for a binding name |
| `binddown BINDING` | Simulate key-down for a binding name |

## DMA Tracing

### Runtime DMA Logging

Use the `dmatrace` (`dmat`) debugger command to log DMA transfers to the terminal as they happen. This shows source address, destination, length, and type for every DMA operation.

### File-Based DMA Logging

Start BlastEm with `--dma-log FILE` to write all DMA transfers to a file:
```
blastem --dma-log dma.log rom.bin
```

### DMA History for Debug Views

The DMA history ring buffer (default 1024 entries, configurable with `--dma-history`) records recent 68K→VRAM and 68K→CRAM DMA transfers. The Sprite Inspector uses this to show where tile and palette data came from.

For games that use many small DMA transfers, increase the history size:
```
blastem --dma-history 4096 rom.bin
```

## Screenshots

| Key | Binding Name | Description |
|-----|-------------|-------------|
| `p` | `ui.screenshot` | Save a screenshot of the current frame |

Screenshots are saved as PPM files using the template `blastem_%c.ppm` where `%c` is replaced with an incrementing character. The output path can be configured via `ui.screenshot_path` and `ui.screenshot_template` in the config file.

To capture debug views, open the desired debug window first, then press `p`. The screenshot captures the main emulator window. For debug window screenshots, use your OS screenshot tool or redirect the debug framebuffer.

## Quick Reference

```
# Start with debugger
blastem -d rom.bin

# Start with DMA logging
blastem --dma-log dma.log --dma-history 4096 rom.bin

# In-game debug views
b = planes/sprites    v = VRAM tiles    c = CRAM palette
n = composite         o = oscilloscope  [ = cycle sub-mode

# Common debugger workflow
u                           # enter debugger (key)
b $200                      # breakpoint at $200
watchpoint $FF0000 4        # watch 4 bytes at $FF0000 for writes
readwatch $FF9200 2         # watch 2 bytes at $FF9200 for reads
c                           # continue
p d0                        # print register d0
vs                          # dump sprite table
disasm                      # disassemble at PC
```
