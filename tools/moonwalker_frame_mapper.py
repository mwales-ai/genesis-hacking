#!/usr/bin/env python3
"""
moonwalker_frame_mapper.py - Map animation frames to ROM tile data offsets.

The sprite system works as follows:
1. Frame pointer table at 0x0191A2: frame_index -> ROM pointer to sub-frame list
2. Each sub-frame list: count word + array of 16-bit offsets into attribute table
3. Attribute table at 0x01A77E: 8-byte VDP sprite attribute entries
4. Each attribute entry has a tile index that references VRAM patterns

This script traces the complete chain and maps each animation frame to:
- Which tile indices it uses
- Where in VRAM those tiles need to be
- What the frame looks like (dimensions, sub-sprite layout)

Then correlates with the known sprite data chunks in ROM to determine
which ROM offsets correspond to which animation frames.
"""

import struct
import sys
import json
from collections import defaultdict


def read_u16(rom, off):
    return struct.unpack_from('>H', rom, off)[0]

def read_u32(rom, off):
    return struct.unpack_from('>I', rom, off)[0]

def read_i16(rom, off):
    return struct.unpack_from('>h', rom, off)[0]


def parse_sprite_attribute(rom, offset):
    """Parse a single VDP sprite attribute entry (8 bytes)."""
    w0 = read_u16(rom, offset)
    w1 = read_u16(rom, offset + 2)
    w2 = read_u16(rom, offset + 4)
    w3 = read_u16(rom, offset + 6)

    # Y is screen-relative (128 = top of screen in VDP coords)
    y_raw = w0 & 0x3FF
    h_cells = ((w1 >> 8) & 0x03) + 1
    w_cells = ((w1 >> 10) & 0x03) + 1
    link = w1 & 0x7F

    priority = (w2 >> 15) & 1
    palette = (w2 >> 13) & 3
    vflip = (w2 >> 12) & 1
    hflip = (w2 >> 11) & 1
    tile = w2 & 0x7FF

    x_raw = w3 & 0x3FF

    # Convert to signed offsets (centered around 128)
    y = y_raw - 128 if y_raw > 0 else y_raw
    x = x_raw - 128 if x_raw > 0 else x_raw

    return {
        'y': y, 'x': x,
        'w_cells': w_cells, 'h_cells': h_cells,
        'tile': tile,
        'palette': palette,
        'hflip': hflip, 'vflip': vflip,
        'priority': priority,
        'link': link,
        'total_tiles': w_cells * h_cells,
    }


def map_all_frames(rom):
    """Map every animation frame to its tile indices and layout."""
    frame_table = 0x0191A2
    attr_table = 0x01A77E

    frames = []

    for frame_idx in range(200):
        ptr_off = frame_table + frame_idx * 4
        if ptr_off + 4 > len(rom):
            break

        ptr = read_u32(rom, ptr_off)

        # Skip invalid pointers
        if ptr >= len(rom) or ptr == 0 or (ptr & 1):
            frames.append(None)
            continue

        subframe_count = read_u16(rom, ptr)
        if subframe_count == 0 or subframe_count > 100:
            frames.append(None)
            continue

        subframes = []
        all_tiles = set()
        min_tile = 9999
        max_tile = 0
        total_tile_count = 0

        for sf_idx in range(subframe_count):
            attr_offset = read_u16(rom, ptr + 2 + sf_idx * 2)
            attr_addr = attr_table + attr_offset

            if attr_addr + 8 > len(rom):
                continue

            attr = parse_sprite_attribute(rom, attr_addr)
            subframes.append(attr)

            if attr['tile'] > 0:
                tile_start = attr['tile']
                tile_end = tile_start + attr['total_tiles'] - 1
                for t in range(tile_start, tile_end + 1):
                    all_tiles.add(t)
                if tile_start < min_tile:
                    min_tile = tile_start
                if tile_end > max_tile:
                    max_tile = tile_end
                total_tile_count += attr['total_tiles']

        if not all_tiles:
            frames.append(None)
            continue

        frames.append({
            'index': frame_idx,
            'ptr': ptr,
            'subframe_count': subframe_count,
            'subframes': subframes,
            'tile_range': (min_tile, max_tile),
            'unique_tiles': sorted(all_tiles),
            'total_tiles': total_tile_count,
            'vram_bytes': total_tile_count * 32,
            'palettes_used': sorted(set(sf['palette'] for sf in subframes if sf['tile'] > 0)),
        })

    return frames


def find_tile_loading_tables(rom):
    """Search for tables that map animation frames to ROM tile data offsets.

    Look for pointer tables or offset tables near the sprite loading code
    (0x014B00-0x014EA0) that contain addresses within the sprite bank.
    """
    sprite_bank_start = 0x01B000
    sprite_bank_end = 0x02A400

    # Search the code/data area for consecutive 32-bit values that look like
    # ROM pointers into the sprite bank
    print("=" * 70)
    print("Searching for pointer tables into sprite bank")
    print("=" * 70)

    for search_start in range(0x014B00, 0x016000, 2):
        # Check if 4+ consecutive longs point into sprite bank
        consecutive = 0
        for i in range(20):
            off = search_start + i * 4
            if off + 4 > len(rom):
                break
            val = read_u32(rom, off)
            if sprite_bank_start <= val < sprite_bank_end:
                consecutive += 1
            else:
                break

        if consecutive >= 4:
            print(f"\n  Potential pointer table at 0x{search_start:06X}:")
            for i in range(consecutive + 2):
                off = search_start + i * 4
                if off + 4 > len(rom):
                    break
                val = read_u32(rom, off)
                marker = " <-- sprite bank" if sprite_bank_start <= val < sprite_bank_end else ""
                print(f"    [{i:2d}] 0x{off:06X}: 0x{val:08X}{marker}")

    # Also search the entire ROM for tables
    print("\n  Scanning full ROM for pointer tables into sprite bank...")
    found_tables = []
    offset = 0x200
    while offset < len(rom) - 16:
        consecutive = 0
        for i in range(40):
            off = offset + i * 4
            if off + 4 > len(rom):
                break
            val = read_u32(rom, off)
            if sprite_bank_start <= val < sprite_bank_end:
                consecutive += 1
            elif consecutive > 0:
                break

        if consecutive >= 6:
            found_tables.append((offset, consecutive))
            offset += consecutive * 4
        else:
            offset += 2

    for tbl_off, count in found_tables:
        print(f"\n  Table at 0x{tbl_off:06X} ({count} entries):")
        for i in range(min(count, 30)):
            off = tbl_off + i * 4
            val = read_u32(rom, off)
            # Calculate what sprite chunk this points to
            chunk_name = identify_chunk(val)
            print(f"    [{i:2d}] 0x{val:06X}  {chunk_name}")
        if count > 30:
            print(f"    ... {count-30} more entries")


def identify_chunk(addr):
    """Identify which known sprite chunk an address falls in."""
    chunks = [
        (0x01D760, 0x01D760 + 18688, "Characters-large (2x4x73)"),
        (0x0220E0, 0x0220E0 + 5888, "MJ Anim A (2x4x23)"),
        (0x023800, 0x023800 + 13760, "MJ Anim B (2x5x43)"),
        (0x026DE0, 0x026DE0 + 3328, "Chunk D (2x4x13)"),
        (0x027B60, 0x027B60 + 5376, "Chunk E (2x4x21)"),
        (0x0290A0, 0x0290A0 + 1440, "Large chars (3x5x3)"),
        (0x029660, 0x029660 + 3328, "Chunk F (2x4x13)"),
    ]
    for start, end, name in chunks:
        if start <= addr < end:
            frame_offset = addr - start
            return f"{name} +0x{frame_offset:04X}"
    return ""


def analyze_vram_layout(frames):
    """Analyze what VRAM regions are used by different frames."""
    print("\n" + "=" * 70)
    print("VRAM Tile Usage by Frame")
    print("=" * 70)

    # Group frames by their tile range
    tile_ranges = defaultdict(list)
    for f in frames:
        if f is None:
            continue
        tile_ranges[f['tile_range']].append(f['index'])

    print(f"\nUnique VRAM tile ranges: {len(tile_ranges)}")
    for (lo, hi), frame_list in sorted(tile_ranges.items()):
        vram_start = lo * 32
        vram_end = (hi + 1) * 32
        print(f"  Tiles {lo}-{hi} (VRAM 0x{vram_start:05X}-0x{vram_end:05X}): "
              f"used by frames {frame_list[:10]}{'...' if len(frame_list) > 10 else ''}")

    # Find the overall VRAM footprint
    all_tiles = set()
    for f in frames:
        if f is None:
            continue
        all_tiles.update(f['unique_tiles'])

    if all_tiles:
        print(f"\nOverall tile range: {min(all_tiles)}-{max(all_tiles)}")
        print(f"Total unique tiles used: {len(all_tiles)}")
        print(f"VRAM needed: {len(all_tiles) * 32} bytes ({len(all_tiles) * 32 / 1024:.1f} KB)")


def correlate_with_rom_data(rom, frames):
    """Try to correlate VRAM tile patterns with ROM sprite data.

    Look at the tile data loading code at 0x014B00+ to understand the
    mapping between frame indices and ROM offsets.
    """
    print("\n" + "=" * 70)
    print("Frame-to-ROM Correlation")
    print("=" * 70)

    # The sprite loading uses a jump table at 0x014D42 based on 0xDE42
    # (which appears to be a level/character type selector)
    # Let's examine the dispatch table and its targets

    print("\n  Jump table at 0x014D42 (level/char type dispatch):")
    jt_base = 0x014D50
    for i in range(15):
        off = jt_base + i * 4
        word = read_u16(rom, off)
        # BRA.W displacement
        if (word & 0xF000) == 0x6000:
            disp = read_i16(rom, off + 2)
            target = off + 2 + disp
            print(f"    [{i:2d}] 0x{off:06X}: BRA 0x{target:06X}")

    # Look at the data tables at known addresses referenced in the sprite loader
    # The table at 0x014CA2 seems to be a pattern/palette descriptor table
    print("\n  Descriptor table at 0x014CA2:")
    off = 0x014CA2
    for i in range(12):
        if off + 10 > len(rom):
            break
        w0 = read_u16(rom, off)
        w1 = read_u16(rom, off + 2)
        w2 = read_u16(rom, off + 4)
        w3 = read_u16(rom, off + 6)
        w4 = read_u16(rom, off + 8)

        # Try to interpret as: tile_count, something, width, height, offset_word
        ptr = read_u32(rom, off + 6)
        print(f"    [{i:2d}] 0x{off:06X}: {w0:04X} {w1:04X} {w2:04X} {w3:04X} {w4:04X}  "
              f"(ptr if long: 0x{ptr:08X})")
        off += 10  # entries might be variable size, need to check

    # Check specific patterns: the code at 0x014B50 reads from a table via
    # "MOVE.W $DE42.W, D0; LSL.W #3, D0; MOVEA.L (A3,D0.W), A3"
    # which means it indexes into the frame table by level type
    print("\n  Frame-to-level-type analysis:")
    print("  The variable at 0xDE42 selects the character/level type")
    print("  This indexes into jump tables to select different loading routines")

    # Let me look at what's stored in the sprite bank more carefully
    # by checking the boundaries between chunks
    print("\n  Sprite bank data boundaries:")
    chunks = [
        (0x01D760, 2, 4, 73, "Characters-large"),
        (0x0220E0, 2, 4, 23, "MJ Anim A"),
        (0x023800, 2, 5, 43, "MJ Anim B"),
        (0x026DE0, 2, 4, 13, "Chunk D"),
        (0x027B60, 2, 4, 21, "Chunk E"),
        (0x0290A0, 3, 5, 3, "Large chars"),
        (0x029660, 2, 4, 13, "Chunk F"),
    ]

    # Check gaps between chunks
    chunks_sorted = sorted(chunks, key=lambda c: c[0])
    for i, (off, w, h, fc, name) in enumerate(chunks_sorted):
        size = w * h * 32 * fc
        end = off + size
        print(f"    0x{off:06X}-0x{end:06X} ({size:5d} bytes): {name} ({w}x{h}x{fc})")

        if i + 1 < len(chunks_sorted):
            next_off = chunks_sorted[i+1][0]
            gap = next_off - end
            if gap > 0:
                # Check if gap contains tile data
                gap_data = rom[end:next_off]
                zero_ratio = gap_data.count(0) / len(gap_data) if gap_data else 0
                print(f"      GAP: {gap} bytes (0x{end:06X}-0x{next_off:06X}), "
                      f"zero ratio: {zero_ratio:.2%}")
                # Show first 16 bytes of gap
                print(f"      First 16 bytes: {gap_data[:16].hex()}")
            elif gap < 0:
                print(f"      OVERLAP: {-gap} bytes!")

    # Check data BEFORE the first known chunk (0x01B000-0x01D760)
    pre_chunk = rom[0x01B000:0x01D760]
    zero_ratio = pre_chunk.count(0) / len(pre_chunk) if pre_chunk else 0
    print(f"\n    Pre-chunk data (0x01B000-0x01D760): {len(pre_chunk)} bytes, "
          f"zero ratio: {zero_ratio:.2%}")
    # Count how many 256-byte (2x4 sprite) blocks have good zero ratios
    good_blocks = 0
    for i in range(0, len(pre_chunk) - 256, 256):
        block = pre_chunk[i:i+256]
        bz = block.count(0) / 256
        if bz > 0.3:
            good_blocks += 1
    print(f"    Blocks with >30% zeros (likely tiles): {good_blocks}/{len(pre_chunk)//256}")


def main():
    rom_path = sys.argv[1] if len(sys.argv) > 1 else 'roms/Moonwalker.bin'

    with open(rom_path, 'rb') as f:
        rom = f.read()

    print(f"Loaded ROM: {rom_path} ({len(rom)} bytes)\n")

    # Map all animation frames
    frames = map_all_frames(rom)
    valid_frames = [f for f in frames if f is not None]

    print("=" * 70)
    print(f"Animation Frame Summary")
    print("=" * 70)
    print(f"Total frames defined: {len(frames)}")
    print(f"Valid frames: {len(valid_frames)}")

    # Print each frame's tile usage
    print(f"\n{'Frame':>5} {'SubFr':>5} {'Tiles':>6} {'Range':>12} {'VRAM bytes':>10} {'Pals':>8}")
    for f in valid_frames:
        tile_lo, tile_hi = f['tile_range']
        pals = ','.join(str(p) for p in f['palettes_used'])
        print(f"  {f['index']:3d}   {f['subframe_count']:3d}   {f['total_tiles']:4d}   "
              f"{tile_lo:4d}-{tile_hi:4d}   {f['vram_bytes']:6d}      [{pals}]")

    # VRAM layout analysis
    analyze_vram_layout(frames)

    # Search for pointer tables into sprite bank
    find_tile_loading_tables(rom)

    # Correlate with ROM data
    correlate_with_rom_data(rom, frames)


if __name__ == '__main__':
    main()
