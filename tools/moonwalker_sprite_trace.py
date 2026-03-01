#!/usr/bin/env python3
"""
moonwalker_sprite_trace.py - Deep analysis of Moonwalker sprite data flow.

Traces how sprite tile data moves from ROM to VRAM by analyzing:
1. Sprite attribute table at 0x01A77E (tile indices, sizes, positions)
2. Frame pointer table at 0x0191A2 (animation frame -> sub-frame list)
3. Data at known sprite offsets (compression detection)
4. References to DMA parameter RAM variables
5. Block copy patterns near sprite bank code
"""

import struct
import sys
import json
from collections import Counter, defaultdict


def read_u16(rom, off):
    return struct.unpack_from('>H', rom, off)[0]

def read_u32(rom, off):
    return struct.unpack_from('>I', rom, off)[0]

def read_i16(rom, off):
    return struct.unpack_from('>h', rom, off)[0]


def analyze_sprite_attributes(rom):
    """Parse the sprite attribute table at 0x01A77E.

    Each entry is 8 bytes in VDP sprite attribute format:
      +0: ------yy yyyyyyyy  (Y position, signed/offset from base)
      +2: ----hhww -lllllll  (h=height-1, w=width-1, link=next sprite)
      +4: pccvhnnn nnnnnnnn  (priority, palette, vflip, hflip, tile#)
      +6: ------xx xxxxxxxx  (X position, signed/offset from base)
    """
    base = 0x01A77E
    print("=" * 70)
    print("Sprite Attribute Table at 0x01A77E")
    print("=" * 70)

    tile_numbers = Counter()
    palette_usage = Counter()
    entries = []

    # Read ~4KB (512 entries max)
    for i in range(512):
        off = base + i * 8
        if off + 8 > len(rom):
            break

        w0 = read_u16(rom, off)
        w1 = read_u16(rom, off + 2)
        w2 = read_u16(rom, off + 4)
        w3 = read_u16(rom, off + 6)

        y = w0 & 0x3FF
        h = ((w1 >> 8) & 0x03) + 1  # height in cells
        w = ((w1 >> 10) & 0x03) + 1  # width in cells
        link = w1 & 0x7F

        priority = (w2 >> 15) & 1
        palette = (w2 >> 13) & 3
        vflip = (w2 >> 12) & 1
        hflip = (w2 >> 11) & 1
        tile = w2 & 0x7FF

        x = w3 & 0x3FF

        # Stop if we hit clearly invalid data (all zeros or all FF)
        if w0 == 0 and w1 == 0 and w2 == 0 and w3 == 0:
            # Could be valid (entry 0), keep going but track
            pass

        if tile > 0 or (w0 != 0 or w1 != 0):  # non-trivial entry
            tile_numbers[tile] += 1
            palette_usage[palette] += 1

        entries.append({
            'index': i, 'offset': off,
            'y': y, 'x': x, 'w': w, 'h': h,
            'tile': tile, 'palette': palette,
            'hflip': hflip, 'vflip': vflip,
            'priority': priority, 'link': link,
        })

    # Find the actual extent of the table by looking for where data stops
    # looking like sprite attributes
    last_valid = 0
    for e in entries:
        if e['tile'] > 0 and e['tile'] < 2048:
            last_valid = e['index']

    print(f"Table entries analyzed: {len(entries)}")
    print(f"Last entry with valid tile#: {last_valid} (offset 0x{base + last_valid*8:06X})")
    print(f"Table size: ~{(last_valid+1)*8} bytes")
    print()

    # Tile number distribution
    print("Tile number range used by sprites:")
    if tile_numbers:
        min_tile = min(t for t in tile_numbers if t > 0)
        max_tile = max(tile_numbers)
        print(f"  Range: {min_tile} - {max_tile} (0x{min_tile:03X} - 0x{max_tile:03X})")
        print(f"  VRAM offset range: 0x{min_tile*32:05X} - 0x{(max_tile+1)*32:05X}")
        print(f"  Unique tile indices: {len(tile_numbers)}")

        # Most common tiles
        print(f"  Top 10 most-used tiles: {tile_numbers.most_common(10)}")

    print(f"\nPalette usage: {dict(palette_usage)}")

    # Print first 20 entries as example
    print("\nFirst 30 sprite attribute entries:")
    print(f"  {'#':>3}  {'Off':>8}  {'Y':>4}  {'X':>4}  {'WxH':>5}  {'Tile':>5}  {'Pal':>3}  {'Flips':>5}  {'Link':>4}")
    for e in entries[:30]:
        flips = ('H' if e['hflip'] else '.') + ('V' if e['vflip'] else '.')
        print(f"  {e['index']:3d}  0x{e['offset']:06X}  {e['y']:4d}  {e['x']:4d}  "
              f"{e['w']}x{e['h']:d}  0x{e['tile']:03X}  {e['palette']:3d}  {flips:>5}  {e['link']:4d}")

    return entries[:last_valid+1], tile_numbers


def analyze_frame_pointer_table(rom):
    """Parse the frame pointer table at 0x0191A2."""
    base = 0x0191A2
    print("\n" + "=" * 70)
    print("Frame Pointer Table at 0x0191A2")
    print("=" * 70)

    frames = []
    # Read up to 200 entries
    for i in range(200):
        off = base + i * 4
        if off + 4 > len(rom):
            break

        ptr = read_u32(rom, off)

        # Valid pointers should be within ROM range and word-aligned
        if ptr == 0 or ptr >= len(rom) or (ptr & 1):
            # Could still be valid if it's a sentinel
            if ptr >= 0x01000000:  # clearly not a ROM pointer
                frames.append({'index': i, 'ptr': ptr, 'valid': False})
                continue

        if ptr < len(rom):
            subframe_count = read_u16(rom, ptr)
            frames.append({
                'index': i, 'ptr': ptr, 'valid': True,
                'subframe_count': subframe_count,
            })
        else:
            frames.append({'index': i, 'ptr': ptr, 'valid': False})

    # Find last valid frame
    last_valid = 0
    for f in frames:
        if f['valid'] and 0 < f.get('subframe_count', 0) < 100:
            last_valid = f['index']

    print(f"Entries read: {len(frames)}")
    print(f"Last valid frame: {last_valid}")

    valid_frames = [f for f in frames if f['valid'] and 0 < f.get('subframe_count', 0) < 100]
    print(f"Valid frames with reasonable subframe counts: {len(valid_frames)}")

    # Show subframe count distribution
    sf_counts = Counter(f['subframe_count'] for f in valid_frames)
    print(f"Subframe count distribution: {dict(sf_counts)}")

    # Print first 40 frames
    print(f"\nFrame table entries:")
    print(f"  {'#':>3}  {'Pointer':>10}  {'SubFrames':>10}  Notes")
    for f in frames[:min(80, last_valid+5)]:
        if f['valid']:
            sf = f.get('subframe_count', '?')
            note = ""
            if isinstance(sf, int) and (sf == 0 or sf > 50):
                note = " <-- suspicious"
            print(f"  {f['index']:3d}  0x{f['ptr']:08X}  {sf:>10}  {note}")
        else:
            print(f"  {f['index']:3d}  0x{f['ptr']:08X}       N/A   <-- invalid ptr")

    # For valid frames, follow pointers and read subframe offset lists
    print("\nDetailed subframe analysis (first 10 valid frames):")
    shown = 0
    for f in valid_frames:
        if shown >= 10:
            break
        ptr = f['ptr']
        count = f['subframe_count']
        print(f"\n  Frame {f['index']} @ 0x{ptr:06X}: {count} sub-frames")

        # Read subframe offsets (16-bit each, after the count word)
        offsets = []
        for j in range(min(count, 40)):
            off_val = read_u16(rom, ptr + 2 + j * 2)
            offsets.append(off_val)

        # These offsets index into the sprite attribute table at 0x01A77E
        print(f"    Offsets (into attr table): {['0x{:04X}'.format(o) for o in offsets[:20]]}")
        if len(offsets) > 20:
            print(f"    ... and {len(offsets)-20} more")

        # Convert offsets to entry indices (each entry = 8 bytes)
        indices = [o // 8 for o in offsets]
        print(f"    Entry indices: {indices[:20]}")

        shown += 1

    return frames[:last_valid+1]


def check_compression_headers(rom):
    """Check for Nemesis/Kosinski compression at known sprite chunk starts."""
    print("\n" + "=" * 70)
    print("Compression Detection at Known Sprite Offsets")
    print("=" * 70)

    chunks = [
        (0x01D760, "Characters - large set", 2, 4, 73),
        (0x0220E0, "MJ Anim Set A", 2, 4, 23),
        (0x023800, "MJ Anim Set B", 2, 5, 43),
        (0x026DE0, "Characters - chunk D", 2, 4, 13),
        (0x027B60, "Characters - chunk E", 2, 4, 21),
        (0x0290A0, "Large characters", 3, 5, 3),
        (0x029660, "Characters - chunk F", 2, 4, 13),
    ]

    for offset, name, w, h, fc in chunks:
        expected_size = w * h * 32 * fc
        first_word = read_u16(rom, offset)
        first_long = read_u32(rom, offset)

        # Nemesis: first word = number of tiles (with possible bit 15 = XOR mode)
        nem_tiles = first_word & 0x7FFF
        nem_xor = (first_word >> 15) & 1
        nem_expected_tiles = w * h * fc

        # Check if first 16 bytes look like tile data (4bpp) or compressed
        data = rom[offset:offset+32]
        zero_count = sum(1 for b in data if b == 0)

        print(f"\n  {name} @ 0x{offset:06X}:")
        print(f"    Expected: {w}x{h} × {fc} frames = {nem_expected_tiles} tiles, {expected_size} bytes")
        print(f"    First word: 0x{first_word:04X} (as tile count: {nem_tiles}, XOR: {nem_xor})")
        print(f"    First 32 bytes (1 tile): {data.hex()}")
        print(f"    Zero bytes in first tile: {zero_count}/32 ({zero_count/32:.0%})")

        # Heuristic: if zero_count > 8 and values are small nibbles, likely raw tile data
        nibble_max = max(max(b >> 4, b & 0xF) for b in data if b > 0) if any(data) else 0
        if zero_count > 6 and nibble_max <= 15:
            print(f"    Assessment: Likely UNCOMPRESSED (high zero ratio, max nibble={nibble_max})")
        elif first_word == nem_expected_tiles or first_word == (nem_expected_tiles | 0x8000):
            print(f"    Assessment: Possible NEMESIS compressed (tile count matches)")
        else:
            print(f"    Assessment: Likely UNCOMPRESSED (first word doesn't match Nemesis header)")


def find_ram_variable_references(rom):
    """Find code that writes to the DMA parameter RAM variables."""
    print("\n" + "=" * 70)
    print("References to DMA Parameter RAM Variables")
    print("=" * 70)

    # Key RAM addresses used as DMA parameters
    ram_vars = {
        0xDFF4: "DMA source (call at 0x001FFA)",
        0xDFF6: "DMA dest (call at 0x001FFA)",
        0xDEBA: "DMA source (call at 0x00572C)",
        0xDEBC: "DMA dest (call at 0x00572C)",
        0xDEB6: "DMA length (call at 0x00572C)",
        0xDE20: "DMA trampoline (cmd word)",
    }

    for addr_short, desc in ram_vars.items():
        addr_full = 0xFF0000 | addr_short
        # Search for the short address form in ROM (sign-extended addressing)
        addr_bytes = struct.pack('>H', addr_short)

        refs = []
        for offset in range(0, len(rom) - 2, 2):
            if rom[offset:offset+2] == addr_bytes:
                # Check if this looks like an instruction operand (not just random data)
                # Look at previous word for opcode context
                if offset >= 2:
                    prev_word = read_u16(rom, offset - 2)
                    # Common patterns: MOVE.x Dn, (abs.w) or MOVE.x (abs.w), Dn
                    refs.append(offset)

        if refs:
            # Filter to refs in code area (< 0x016000 roughly)
            code_refs = [r for r in refs if r < 0x016000]
            print(f"\n  0x{addr_short:04X} (0x{addr_full:06X}) - {desc}")
            print(f"    Refs in code area: {len(code_refs)} (of {len(refs)} total)")
            for r in code_refs[:10]:
                # Show context: 6 bytes before and 6 after
                ctx_start = max(0, r - 6)
                ctx_end = min(len(rom), r + 8)
                ctx = rom[ctx_start:ctx_end].hex()
                print(f"      0x{r:06X}: ...{ctx}...")


def find_block_copy_patterns(rom):
    """Search for block copy loops in the sprite loading area."""
    print("\n" + "=" * 70)
    print("Block Copy Patterns (0x014800-0x015600)")
    print("=" * 70)

    start = 0x014800
    end = 0x015600

    # Look for DBF/DBRA (opcode 0x51C8-0x51CF)
    print("\n  DBF/DBRA loops:")
    for offset in range(start, end, 2):
        word = read_u16(rom, offset)
        if 0x51C8 <= word <= 0x51CF:
            dn = word & 0x07
            disp = read_i16(rom, offset + 2)
            target = offset + 2 + disp
            print(f"    0x{offset:06X}: DBF D{dn}, 0x{target:06X} (disp={disp})")

    # Look for MOVEM.L (0x48E0-0x48FF for -(An), 0x4CE0-0x4CFF for (An)+)
    print("\n  MOVEM.L instructions:")
    for offset in range(start, end, 2):
        word = read_u16(rom, offset)
        if (word & 0xFFC0) == 0x48C0:  # MOVEM.L regs, <ea>
            regmask = read_u16(rom, offset + 2)
            mode = (word >> 3) & 0x07
            reg = word & 0x07
            if mode == 4:  # -(An) predecrement
                print(f"    0x{offset:06X}: MOVEM.L regs, -(A{reg}) mask=0x{regmask:04X}")
            elif mode == 2:  # (An) indirect
                print(f"    0x{offset:06X}: MOVEM.L regs, (A{reg}) mask=0x{regmask:04X}")
        elif (word & 0xFFC0) == 0x4CC0:  # MOVEM.L <ea>, regs
            regmask = read_u16(rom, offset + 2)
            mode = (word >> 3) & 0x07
            reg = word & 0x07
            if mode == 3:  # (An)+ postincrement
                print(f"    0x{offset:06X}: MOVEM.L (A{reg})+, regs mask=0x{regmask:04X}")


def analyze_sprite_tile_mapping(rom):
    """Try to determine how tile indices in sprite attributes map to ROM offsets.

    The sprite attribute table uses VRAM tile indices. We need to find where
    the game loads those tiles FROM in ROM, and WHERE in VRAM they go.
    """
    print("\n" + "=" * 70)
    print("Sprite Tile Index to ROM Offset Mapping Analysis")
    print("=" * 70)

    # Read the DMA call at 0x001FFA more carefully
    # The code around 0x001F00-0x002100 handles the main per-frame sprite update
    print("\n  Hex dump of main sprite update routine (0x001F00-0x002000):")
    for off in range(0x001F00, 0x002010, 16):
        hex_str = rom[off:off+16].hex()
        # Insert spaces every 4 chars (2 bytes = 1 word)
        words = ' '.join(hex_str[i:i+4] for i in range(0, len(hex_str), 4))
        print(f"    0x{off:06X}: {words}")

    # Also dump the routine that loads sprite patterns (around 0x014B00-0x014E00)
    print("\n  Hex dump of sprite pattern loader (0x014B00-0x014EA0):")
    for off in range(0x014B00, 0x014EA0, 16):
        hex_str = rom[off:off+16].hex()
        words = ' '.join(hex_str[i:i+4] for i in range(0, len(hex_str), 4))
        print(f"    0x{off:06X}: {words}")


def main():
    rom_path = sys.argv[1] if len(sys.argv) > 1 else 'roms/Moonwalker.bin'

    with open(rom_path, 'rb') as f:
        rom = f.read()

    print(f"Loaded ROM: {rom_path} ({len(rom)} bytes)")
    print()

    # 1. Analyze sprite attribute table
    attrs, tile_nums = analyze_sprite_attributes(rom)

    # 2. Analyze frame pointer table
    frames = analyze_frame_pointer_table(rom)

    # 3. Check compression at sprite offsets
    check_compression_headers(rom)

    # 4. Find RAM variable references
    find_ram_variable_references(rom)

    # 5. Find block copy patterns
    find_block_copy_patterns(rom)

    # 6. Tile mapping analysis
    analyze_sprite_tile_mapping(rom)


if __name__ == '__main__':
    main()
