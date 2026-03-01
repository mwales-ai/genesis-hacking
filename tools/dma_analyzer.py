#!/usr/bin/env python3
"""
dma_analyzer.py - Static analysis of Sega Genesis ROM DMA transfers.

Scans a Genesis ROM for 68K instructions that set up VDP DMA transfers by
writing to VDP registers 19-23 via the control port at 0xC00004. Extracts
source addresses, lengths, and VRAM destinations for hardcoded transfers,
and identifies dynamic DMA subroutines for breakpoint placement.

Usage:
    python3 dma_analyzer.py roms/Moonwalker.bin
    python3 dma_analyzer.py roms/Moonwalker.bin --json moonwalker.json --output report.json
    python3 dma_analyzer.py roms/Moonwalker.bin --verbose
"""

import argparse
import json
import struct
import sys
from collections import defaultdict


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

VDP_CTRL_PORT = 0x00C00004
VDP_DATA_PORT = 0x00C00000

# VDP register command: bits 15-13 = 100, bits 12-8 = register, bits 7-0 = value
# Register write: 0x8000 | (reg << 8) | value
VDP_REG_DMA_LEN_LO  = 19   # $93xx
VDP_REG_DMA_LEN_HI  = 20   # $94xx
VDP_REG_DMA_SRC_LO  = 21   # $95xx
VDP_REG_DMA_SRC_MID = 22   # $96xx
VDP_REG_DMA_SRC_HI  = 23   # $97xx
VDP_REG_AUTO_INC    = 15   # $8Fxx

VDP_REG_NAMES = {
    0: 'Mode1', 1: 'Mode2', 2: 'PlaneA_NT', 3: 'Window_NT',
    4: 'PlaneB_NT', 5: 'Sprite_Table', 6: 'Sprite_Pat_Base', 7: 'BG_Color',
    10: 'HBlank_Counter', 11: 'Mode3', 12: 'Mode4', 13: 'HScroll_Addr',
    15: 'Auto_Increment', 16: 'Plane_Size', 17: 'Window_HPos', 18: 'Window_VPos',
    19: 'DMA_Len_Lo', 20: 'DMA_Len_Hi', 21: 'DMA_Src_Lo',
    22: 'DMA_Src_Mid', 23: 'DMA_Src_Hi',
}

# CD field meanings for VDP command words
VDP_CD_NAMES = {
    0b000000: 'VRAM read',
    0b000001: 'VRAM write',
    0b000011: 'CRAM write',
    0b000100: 'VSRAM read',
    0b000101: 'VSRAM write',
    0b001000: 'CRAM read',
    0b100001: 'VRAM DMA write',
    0b100011: 'CRAM DMA write',
    0b100101: 'VSRAM DMA write',
}


# ---------------------------------------------------------------------------
# ROM reading helpers
# ---------------------------------------------------------------------------

def read_u16(rom, offset):
    """Read big-endian 16-bit word from ROM."""
    if offset + 2 > len(rom):
        return None
    return struct.unpack_from('>H', rom, offset)[0]


def read_u32(rom, offset):
    """Read big-endian 32-bit long from ROM."""
    if offset + 4 > len(rom):
        return None
    return struct.unpack_from('>I', rom, offset)[0]


def read_i16(rom, offset):
    """Read big-endian signed 16-bit word from ROM."""
    if offset + 2 > len(rom):
        return None
    return struct.unpack_from('>h', rom, offset)[0]


# ---------------------------------------------------------------------------
# VDP command word decoding
# ---------------------------------------------------------------------------

def is_vdp_reg_write(word):
    """Check if a 16-bit word is a VDP register write command (bits 15-13 = 100)."""
    return (word & 0xE000) == 0x8000


def decode_vdp_reg_write(word):
    """Decode a VDP register write. Returns (reg_num, value) or None."""
    if not is_vdp_reg_write(word):
        return None
    reg = (word >> 8) & 0x1F
    val = word & 0xFF
    return (reg, val)


def decode_vdp_cmd_word(high_word, low_word):
    """Decode a 32-bit VDP address/command word. Returns (cd, address) dict."""
    cd = ((high_word >> 14) & 0x03) | (((low_word >> 4) & 0x0F) << 2)
    addr = (high_word & 0x3FFF) | (((low_word >> 2) & 0x03) << 14)
    is_dma = (cd & 0x20) != 0
    target = VDP_CD_NAMES.get(cd, f'unknown(CD={cd:#08b})')
    return {'cd': cd, 'address': addr, 'target': target, 'is_dma': is_dma}


def calc_dma_source(r21, r22, r23):
    """Calculate ROM source address from DMA registers 21-23.

    The source address stored in registers is the actual address >> 1.
    R23 bit 7 and 6 select DMA type:
      0x = 68K -> VDP transfer (bit 6 = bit 24 of source addr, usually 0 for ROM)
      10 = VRAM fill
      11 = VRAM copy
    """
    dma_type_bits = (r23 >> 6) & 0x03
    if dma_type_bits >= 2:
        # VRAM fill or copy - source is VRAM address, not ROM
        return None, 'fill' if dma_type_bits == 2 else 'copy'

    source = ((r23 & 0x7F) << 16 | r22 << 8 | r21) << 1
    return source, 'transfer'


def calc_dma_length(r19, r20):
    """Calculate DMA length in bytes from registers 19-20.

    Length is in words, so multiply by 2. Length of 0 = 0x10000 words.
    """
    length_words = (r20 << 8) | r19
    if length_words == 0:
        length_words = 0x10000
    return length_words * 2


# ---------------------------------------------------------------------------
# 68K instruction pattern matching
# ---------------------------------------------------------------------------

def find_lea_vdp_ctrl(rom):
    """Find all LEA.L #$00C00004, An instructions.

    LEA.L #imm32, An encoding:
      0100 An[2:0] 111 111001 = 0x41F9 + (An << 9)
      followed by 32-bit immediate
    """
    results = []
    for an in range(8):  # A0-A7
        opcode = 0x41F9 | (an << 9)
        op_hi = (opcode >> 8) & 0xFF
        op_lo = opcode & 0xFF
        offset = 0
        while offset < len(rom) - 6:
            if rom[offset] == op_hi and rom[offset + 1] == op_lo:
                imm32 = read_u32(rom, offset + 2)
                if imm32 == VDP_CTRL_PORT:
                    results.append({
                        'offset': offset,
                        'addr_reg': f'A{an}',
                        'addr_reg_num': an,
                        'instr_size': 6,
                    })
            offset += 2  # 68K instructions are word-aligned
    return sorted(results, key=lambda x: x['offset'])


def find_move_w_imm_to_abs32(rom, target_addr):
    """Find MOVE.W #imm16, (abs32).L instructions targeting a specific address.

    Encoding: 0x33FC, imm16, addr32 (8 bytes total)
    """
    results = []
    for offset in range(0, len(rom) - 8, 2):
        if rom[offset] == 0x33 and rom[offset + 1] == 0xFC:
            addr = read_u32(rom, offset + 4)
            if addr == target_addr:
                imm = read_u16(rom, offset + 2)
                results.append({
                    'offset': offset,
                    'immediate': imm,
                    'instr_size': 8,
                })
    return results


def find_move_l_imm_to_abs32(rom, target_addr):
    """Find MOVE.L #imm32, (abs32).L instructions targeting a specific address.

    Encoding: 0x23FC, imm32, addr32 (10 bytes total)
    """
    results = []
    for offset in range(0, len(rom) - 10, 2):
        if rom[offset] == 0x23 and rom[offset + 1] == 0xFC:
            addr = read_u32(rom, offset + 6)
            if addr == target_addr:
                imm = read_u32(rom, offset + 2)
                results.append({
                    'offset': offset,
                    'immediate': imm,
                    'instr_size': 10,
                })
    return results


def scan_dma_setup_after_lea(rom, lea_info, max_scan=120):
    """After a LEA to VDP control port, scan forward for DMA register writes.

    Looks for MOVE.W #imm, (An) and MOVE.L #imm, (An) where An is the
    address register loaded by the LEA.

    Returns dict of register values found and the DMA trigger command word.
    """
    an = lea_info['addr_reg_num']
    start = lea_info['offset'] + lea_info['instr_size']

    # MOVE.W #imm16, (An) encoding: 0011 An[2:0] 010 111 100 = varies
    # More precisely: size=01 (word), dst mode=010 (indirect), dst reg=An, src mode=111, src reg=100 (imm)
    # Opcode: 0 0 1 1 An2 An1 An0 0 1 0 1 1 1 1 0 0
    move_w_imm_indirect = 0x3080 | (an << 9) | 0x003C
    # Actually let me recalculate:
    # MOVE.W = 0011, dst_reg=An, dst_mode=010 (addr indirect), src_mode=111, src_reg=100
    # = 0011 | An<<9 | 010<<6 | 111100
    # = 0x3000 | (an<<9) | 0x00BC
    move_w_opcode = 0x30BC | (an << 9)

    # MOVE.L #imm32, (An): same but size=10 (long)
    # = 0010 | An<<9 | 010<<6 | 111100
    move_l_opcode = 0x20BC | (an << 9)

    regs = {}  # reg_num -> value
    trigger_cmd = None
    writes = []  # list of all VDP writes found

    offset = start
    end = min(start + max_scan, len(rom) - 4)

    while offset < end:
        word = read_u16(rom, offset)
        if word is None:
            break

        if word == move_w_opcode:
            # MOVE.W #imm16, (An)
            imm = read_u16(rom, offset + 2)
            if imm is None:
                break
            writes.append({'offset': offset, 'value': imm, 'size': 2})
            rv = decode_vdp_reg_write(imm)
            if rv:
                regs[rv[0]] = rv[1]
            offset += 4
            continue

        if word == move_l_opcode:
            # MOVE.L #imm32, (An) - two packed 16-bit VDP writes
            imm = read_u32(rom, offset + 2)
            if imm is None:
                break
            hi = (imm >> 16) & 0xFFFF
            lo = imm & 0xFFFF
            writes.append({'offset': offset, 'value_hi': hi, 'value_lo': lo, 'size': 4})

            # Each half could be a register write
            rv_hi = decode_vdp_reg_write(hi)
            rv_lo = decode_vdp_reg_write(lo)
            if rv_hi:
                regs[rv_hi[0]] = rv_hi[1]
            if rv_lo:
                regs[rv_lo[0]] = rv_lo[1]

            # If neither is a reg write, this might be the DMA trigger command
            if not rv_hi and not rv_lo:
                cmd = decode_vdp_cmd_word(hi, lo)
                if cmd['is_dma']:
                    trigger_cmd = cmd
                    trigger_cmd['raw_hi'] = hi
                    trigger_cmd['raw_lo'] = lo

            offset += 6
            continue

        # Check for RTS (0x4E75) or other branch - stop scanning
        if word == 0x4E75:  # RTS
            break
        if word == 0x4E73:  # RTE
            break

        offset += 2

    return {
        'regs': regs,
        'trigger': trigger_cmd,
        'writes': writes,
        'scan_start': start,
        'scan_end': offset,
    }


def classify_dma_setup(dma_info, lea_offset):
    """Classify a DMA setup as hardcoded or dynamic based on which registers were found."""
    regs = dma_info['regs']

    has_src = all(r in regs for r in [21, 22, 23])
    has_len = all(r in regs for r in [19, 20])

    if has_src and has_len:
        source, dma_type = calc_dma_source(regs[21], regs[22], regs[23])
        length = calc_dma_length(regs[19], regs[20])
        return {
            'type': 'hardcoded',
            'dma_type': dma_type,
            'source_addr': source,
            'length_bytes': length,
            'trigger': dma_info.get('trigger'),
            'lea_offset': lea_offset,
            'regs': {k: v for k, v in regs.items()},
        }
    elif has_src and not has_len:
        source, dma_type = calc_dma_source(regs[21], regs[22], regs[23])
        return {
            'type': 'partial_hardcoded',
            'dma_type': dma_type,
            'source_addr': source,
            'length_bytes': None,
            'note': 'Source hardcoded but length is dynamic',
            'lea_offset': lea_offset,
            'regs': {k: v for k, v in regs.items()},
        }
    else:
        return {
            'type': 'dynamic',
            'note': 'DMA parameters loaded from registers/RAM at runtime',
            'lea_offset': lea_offset,
            'regs_found': list(regs.keys()),
            'regs': {k: v for k, v in regs.items()},
        }


# ---------------------------------------------------------------------------
# ROM-to-RAM copy detection
# ---------------------------------------------------------------------------

def find_lea_in_range(rom, addr_start, addr_end):
    """Find all LEA.L #imm32, An instructions where imm32 falls in [addr_start, addr_end)."""
    results = []
    for an in range(8):
        opcode = 0x41F9 | (an << 9)
        op_hi = (opcode >> 8) & 0xFF
        op_lo = opcode & 0xFF
        for offset in range(0, len(rom) - 6, 2):
            if rom[offset] == op_hi and rom[offset + 1] == op_lo:
                imm32 = read_u32(rom, offset + 2)
                if addr_start <= imm32 < addr_end:
                    results.append({
                        'offset': offset,
                        'addr_reg': f'A{an}',
                        'target_addr': imm32,
                    })
    return sorted(results, key=lambda x: x['offset'])


def find_move_imm32_refs(rom, addr_start, addr_end):
    """Find MOVE.L #imm32, ... where imm32 falls in the given range.

    Catches addresses being loaded into data registers or pushed to stack.
    MOVE.L #imm32, Dn = 0x203C | (Dn << 9) followed by imm32
    """
    results = []
    for offset in range(0, len(rom) - 6, 2):
        word = read_u16(rom, offset)
        if word is None:
            break
        # MOVE.L #imm32, Dn: 0010 Dn 000 111 100 = 0x203C | (Dn << 9)
        if (word & 0xF1FF) == 0x203C:
            dn = (word >> 9) & 0x07
            imm32 = read_u32(rom, offset + 2)
            if imm32 is not None and addr_start <= imm32 < addr_end:
                results.append({
                    'offset': offset,
                    'data_reg': f'D{dn}',
                    'target_addr': imm32,
                })
    return sorted(results, key=lambda x: x['offset'])


# ---------------------------------------------------------------------------
# Tile data density analysis
# ---------------------------------------------------------------------------

def analyze_tile_density(rom, start, end, tile_size=32):
    """Analyze a ROM region for 4bpp tile data density.

    4bpp tile data has specific characteristics:
    - Each byte has two 4-bit nibbles (each 0x0-0xF)
    - Typically many zero bytes (transparent pixels)
    - Non-random distribution of nibble values

    Returns density metrics.
    """
    data = rom[start:end]
    if len(data) == 0:
        return None

    total_bytes = len(data)
    zero_bytes = data.count(0)
    tiles = total_bytes // tile_size

    # Count nibble distribution
    nibble_counts = [0] * 16
    for b in data:
        nibble_counts[b >> 4] += 1
        nibble_counts[b & 0x0F] += 1

    # Tile data typically has nibble 0 as the most common (transparent)
    max_nibble = max(range(16), key=lambda i: nibble_counts[i])

    return {
        'start': start,
        'end': end,
        'total_bytes': total_bytes,
        'tile_count': tiles,
        'zero_byte_ratio': zero_bytes / total_bytes if total_bytes else 0,
        'dominant_nibble': max_nibble,
        'nibble_0_ratio': nibble_counts[0] / (total_bytes * 2) if total_bytes else 0,
    }


# ---------------------------------------------------------------------------
# Subroutine boundary detection
# ---------------------------------------------------------------------------

def find_containing_subroutine(rom, target_offset):
    """Try to find the start of the subroutine containing target_offset.

    Scans backward for common subroutine entry patterns:
    - MOVEM.L reglist, -(SP)  (0x48E7)
    - LINK An, #disp          (0x4E50 + An)
    - Or simply the first RTS/RTE before target_offset
    """
    # Scan backward from target looking for RTS (0x4E75) or RTE (0x4E73)
    # The subroutine starts right after the previous RTS
    offset = target_offset - 2
    while offset >= 0:
        word = read_u16(rom, offset)
        if word == 0x4E75 or word == 0x4E73:
            return offset + 2  # subroutine starts after previous RTS
        offset -= 2
        if target_offset - offset > 0x1000:  # don't scan too far
            break
    return None


# ---------------------------------------------------------------------------
# Main analysis
# ---------------------------------------------------------------------------

def analyze_rom(rom_data, game_def=None, verbose=False):
    """Run full DMA analysis on a ROM."""
    results = {
        'rom_size': len(rom_data),
        'hardcoded_dma': [],
        'dynamic_dma_routines': [],
        'sprite_bank_refs': [],
        'direct_vdp_writes': [],
        'tile_density': [],
    }

    # Check ROM header
    header_magic = rom_data[0x100:0x104]
    if header_magic in (b'SEGA', b'SEG '):
        game_name = rom_data[0x120:0x150].decode('ascii', errors='replace').strip()
        results['game_name'] = game_name
        if verbose:
            print(f"ROM: {game_name}")
            print(f"Size: {len(rom_data)} bytes ({len(rom_data)/1024:.0f} KB)")
            print()

    # -----------------------------------------------------------------------
    # Phase 1: Find all LEA.L #$C00004, An
    # -----------------------------------------------------------------------
    if verbose:
        print("=" * 70)
        print("Phase 1: Scanning for LEA.L #$C00004, An")
        print("=" * 70)

    leas = find_lea_vdp_ctrl(rom_data)
    if verbose:
        print(f"Found {len(leas)} LEA instructions to VDP control port")
        for lea in leas:
            print(f"  0x{lea['offset']:06X}: LEA.L #$C00004, {lea['addr_reg']}")
        print()

    # -----------------------------------------------------------------------
    # Phase 2: For each LEA, scan forward for DMA register writes
    # -----------------------------------------------------------------------
    if verbose:
        print("=" * 70)
        print("Phase 2: Scanning DMA register setup after each LEA")
        print("=" * 70)

    for lea in leas:
        dma_info = scan_dma_setup_after_lea(rom_data, lea)
        classification = classify_dma_setup(dma_info, lea['offset'])

        if classification['type'] == 'hardcoded':
            classification['addr_reg'] = lea['addr_reg']
            results['hardcoded_dma'].append(classification)
            if verbose:
                src = classification['source_addr']
                length = classification['length_bytes']
                dma_type = classification['dma_type']
                trigger = classification.get('trigger', {})
                vram_dest = trigger.get('address', '?') if trigger else '?'
                vram_target = trigger.get('target', '?') if trigger else '?'
                src_str = f"0x{src:06X}" if src is not None else "N/A"
                vram_str = f"0x{vram_dest:04X}" if isinstance(vram_dest, int) else str(vram_dest)
                print(f"  0x{lea['offset']:06X}: HARDCODED DMA {dma_type}")
                print(f"    Source: {src_str}  Length: {length} bytes  "
                      f"Dest: {vram_str} ({vram_target})")

        elif classification['type'] == 'partial_hardcoded':
            classification['addr_reg'] = lea['addr_reg']
            results['hardcoded_dma'].append(classification)
            if verbose:
                src = classification['source_addr']
                src_str = f"0x{src:06X}" if src is not None else "N/A"
                print(f"  0x{lea['offset']:06X}: PARTIAL DMA (source={src_str}, length=dynamic)")

        elif classification['type'] == 'dynamic':
            # Try to find the subroutine start
            sub_start = find_containing_subroutine(rom_data, lea['offset'])
            classification['subroutine_start'] = sub_start
            classification['addr_reg'] = lea['addr_reg']
            results['dynamic_dma_routines'].append(classification)
            if verbose:
                sub_str = f"0x{sub_start:06X}" if sub_start else "unknown"
                print(f"  0x{lea['offset']:06X}: DYNAMIC DMA routine "
                      f"(subroutine at {sub_str})")
                if classification['regs_found']:
                    print(f"    Hardcoded regs: {classification['regs_found']}")

    if verbose:
        print()

    # -----------------------------------------------------------------------
    # Phase 3: Find direct MOVE.W #$93xx+, ($C00004).L instructions
    # -----------------------------------------------------------------------
    if verbose:
        print("=" * 70)
        print("Phase 3: Scanning for direct MOVE.W #imm, ($C00004).L")
        print("=" * 70)

    direct_writes = find_move_w_imm_to_abs32(rom_data, VDP_CTRL_PORT)
    dma_reg_writes = [w for w in direct_writes
                      if is_vdp_reg_write(w['immediate'])
                      and 19 <= decode_vdp_reg_write(w['immediate'])[0] <= 23]

    if verbose:
        print(f"Found {len(direct_writes)} direct MOVE.W to VDP control port")
        print(f"  Of which {len(dma_reg_writes)} are DMA register writes")
        for w in dma_reg_writes:
            reg, val = decode_vdp_reg_write(w['immediate'])
            name = VDP_REG_NAMES.get(reg, f'R{reg}')
            print(f"    0x{w['offset']:06X}: {name} = 0x{val:02X}")
        print()

    results['direct_vdp_writes'] = [{
        'offset': w['offset'],
        'register': decode_vdp_reg_write(w['immediate'])[0],
        'value': decode_vdp_reg_write(w['immediate'])[1],
    } for w in dma_reg_writes]

    # Also check MOVE.L #imm32, ($C00004).L (packed double writes)
    direct_long_writes = find_move_l_imm_to_abs32(rom_data, VDP_CTRL_PORT)
    if verbose and direct_long_writes:
        print(f"Found {len(direct_long_writes)} direct MOVE.L to VDP control port")
        for w in direct_long_writes:
            hi = (w['immediate'] >> 16) & 0xFFFF
            lo = w['immediate'] & 0xFFFF
            rv_hi = decode_vdp_reg_write(hi)
            rv_lo = decode_vdp_reg_write(lo)
            desc_hi = f"R{rv_hi[0]}=0x{rv_hi[1]:02X}" if rv_hi else f"cmd=0x{hi:04X}"
            desc_lo = f"R{rv_lo[0]}=0x{rv_lo[1]:02X}" if rv_lo else f"cmd=0x{lo:04X}"
            print(f"    0x{w['offset']:06X}: {desc_hi} | {desc_lo}")
        print()

    # -----------------------------------------------------------------------
    # Phase 4: Find references to sprite bank region
    # -----------------------------------------------------------------------
    sprite_bank_start = 0x01B000
    sprite_bank_end = 0x02A400

    # Expand search if game definition provides tile ranges
    if game_def and 'tile_ranges' in game_def:
        for tr in game_def['tile_ranges']:
            s = int(tr.get('start_offset', '0x0'), 16)
            e = int(tr.get('end_offset', '0x0'), 16)
            if s < sprite_bank_start:
                sprite_bank_start = s
            if e > sprite_bank_end:
                sprite_bank_end = e

    if verbose:
        print("=" * 70)
        print(f"Phase 4: Finding references to sprite bank "
              f"(0x{sprite_bank_start:06X}-0x{sprite_bank_end:06X})")
        print("=" * 70)

    lea_refs = find_lea_in_range(rom_data, sprite_bank_start, sprite_bank_end)
    move_refs = find_move_imm32_refs(rom_data, sprite_bank_start, sprite_bank_end)

    all_refs = []
    for ref in lea_refs:
        ref['type'] = 'LEA'
        all_refs.append(ref)
    for ref in move_refs:
        ref['type'] = 'MOVE.L'
        all_refs.append(ref)
    all_refs.sort(key=lambda x: x['offset'])

    results['sprite_bank_refs'] = all_refs

    if verbose:
        print(f"Found {len(lea_refs)} LEA and {len(move_refs)} MOVE.L "
              f"referencing sprite bank")
        for ref in all_refs:
            reg = ref.get('addr_reg', ref.get('data_reg', '?'))
            print(f"  0x{ref['offset']:06X}: {ref['type']} #{ref['target_addr']:#08X}, {reg}")
        print()

    # -----------------------------------------------------------------------
    # Phase 5: Tile density analysis of known regions
    # -----------------------------------------------------------------------
    if verbose:
        print("=" * 70)
        print("Phase 5: Tile data density analysis")
        print("=" * 70)

    # Analyze the main sprite bank
    density = analyze_tile_density(rom_data, sprite_bank_start, sprite_bank_end)
    if density:
        results['tile_density'].append(density)
        if verbose:
            print(f"  Region 0x{density['start']:06X}-0x{density['end']:06X}:")
            print(f"    Tiles: {density['tile_count']}, "
                  f"Zero-byte ratio: {density['zero_byte_ratio']:.2%}, "
                  f"Nibble-0 ratio: {density['nibble_0_ratio']:.2%}")

    # Also analyze regions around known sprite offsets from game def
    if game_def and 'sprite_groups' in game_def:
        for group in game_def['sprite_groups']:
            for sprite in group.get('sprites', []):
                rom_off = int(sprite['rom_offset'], 16)
                w = sprite.get('width_tiles', 1)
                h = sprite.get('height_tiles', 1)
                fc = sprite.get('frame_count', 1)
                size = w * h * 32 * fc
                d = analyze_tile_density(rom_data, rom_off, rom_off + size)
                if d:
                    d['sprite_name'] = sprite['name']
                    results['tile_density'].append(d)
                    if verbose:
                        print(f"  {sprite['name']} @ 0x{rom_off:06X} "
                              f"({size} bytes, {w}x{h}x{fc}):")
                        print(f"    Zero-byte: {d['zero_byte_ratio']:.2%}, "
                              f"Nibble-0: {d['nibble_0_ratio']:.2%}")

    if verbose:
        print()

    # -----------------------------------------------------------------------
    # Phase 6: Cross-reference with game definition
    # -----------------------------------------------------------------------
    if game_def and verbose:
        print("=" * 70)
        print("Phase 6: Cross-reference with game definition")
        print("=" * 70)

        for group in game_def.get('sprite_groups', []):
            print(f"\n  Group: {group['name']}")
            for sprite in group.get('sprites', []):
                rom_off = int(sprite['rom_offset'], 16)
                w = sprite.get('width_tiles', 1)
                h = sprite.get('height_tiles', 1)
                fc = sprite.get('frame_count', 1)
                size = w * h * 32 * fc

                # Check if any hardcoded DMA covers this range
                covered = False
                for dma in results['hardcoded_dma']:
                    if dma.get('source_addr') is not None:
                        dma_start = dma['source_addr']
                        dma_end = dma_start + (dma.get('length_bytes') or 0)
                        if dma_start <= rom_off < dma_end:
                            covered = True
                            print(f"    {sprite['name']}: COVERED by hardcoded DMA "
                                  f"at 0x{dma['lea_offset']:06X}")
                            break

                # Check if any LEA/MOVE references point near this sprite
                nearby_refs = [r for r in all_refs
                               if abs(r['target_addr'] - rom_off) < 0x100]

                if not covered:
                    if nearby_refs:
                        ref = nearby_refs[0]
                        print(f"    {sprite['name']}: No direct DMA, but reference "
                              f"at 0x{ref['offset']:06X} "
                              f"({ref['type']} to 0x{ref['target_addr']:06X})")
                    else:
                        print(f"    {sprite['name']}: NO DMA COVERAGE - "
                              f"needs runtime verification")

    return results


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def print_summary(results):
    """Print a human-readable summary of the analysis."""
    print("\n" + "=" * 70)
    print("ANALYSIS SUMMARY")
    print("=" * 70)

    print(f"\nROM size: {results['rom_size']} bytes "
          f"({results['rom_size']/1024:.0f} KB)")
    if 'game_name' in results:
        print(f"Game: {results['game_name']}")

    # Hardcoded DMA transfers
    hc = results['hardcoded_dma']
    transfers = [d for d in hc if d['dma_type'] == 'transfer']
    fills = [d for d in hc if d['dma_type'] == 'fill']
    copies = [d for d in hc if d['dma_type'] == 'copy']

    print(f"\nHardcoded DMA transfers: {len(transfers)}")
    for d in transfers:
        src = d['source_addr']
        src_str = f"0x{src:06X}" if src is not None else "N/A"
        length = d.get('length_bytes', '?')
        trigger = d.get('trigger', {})
        dest = trigger.get('address', '?') if trigger else '?'
        dest_str = f"0x{dest:04X}" if isinstance(dest, int) else str(dest)
        tgt = trigger.get('target', '') if trigger else ''
        print(f"  ROM {src_str} -> {dest_str} ({tgt}), {length} bytes "
              f"  [code @ 0x{d['lea_offset']:06X}]")

    if fills:
        print(f"\nHardcoded VRAM fills: {len(fills)}")
    if copies:
        print(f"\nHardcoded VRAM copies: {len(copies)}")

    # Dynamic DMA routines
    dyn = results['dynamic_dma_routines']
    print(f"\nDynamic DMA routines (breakpoint targets): {len(dyn)}")
    for d in dyn:
        sub = d.get('subroutine_start')
        sub_str = f"0x{sub:06X}" if sub else "unknown"
        lea_str = f"0x{d['lea_offset']:06X}"
        print(f"  Subroutine: {sub_str}  (LEA at {lea_str})")

    # Sprite bank references
    refs = results['sprite_bank_refs']
    if refs:
        print(f"\nReferences to sprite bank region: {len(refs)}")
        # Group by target address
        by_addr = defaultdict(list)
        for r in refs:
            by_addr[r['target_addr']].append(r)
        for addr in sorted(by_addr.keys()):
            ref_list = by_addr[addr]
            locs = ", ".join(f"0x{r['offset']:06X}" for r in ref_list)
            print(f"  0x{addr:06X}: referenced from {locs}")

    print()


def generate_json_report(results, output_path):
    """Write analysis results as JSON."""
    # Convert to JSON-serializable format
    report = {
        'rom_size': results['rom_size'],
        'game_name': results.get('game_name', 'Unknown'),
        'hardcoded_dma': [],
        'dynamic_dma_routines': [],
        'sprite_bank_references': [],
        'breakpoint_recommendations': [],
    }

    for d in results['hardcoded_dma']:
        entry = {
            'code_offset': f"0x{d['lea_offset']:06X}",
            'dma_type': d['dma_type'],
            'type': d['type'],
        }
        if d.get('source_addr') is not None:
            entry['source_addr'] = f"0x{d['source_addr']:06X}"
        if d.get('length_bytes') is not None:
            entry['length_bytes'] = d['length_bytes']
        if d.get('trigger'):
            entry['vram_dest'] = f"0x{d['trigger']['address']:04X}"
            entry['target_type'] = d['trigger']['target']
        report['hardcoded_dma'].append(entry)

    for d in results['dynamic_dma_routines']:
        entry = {
            'lea_offset': f"0x{d['lea_offset']:06X}",
        }
        if d.get('subroutine_start') is not None:
            entry['subroutine_start'] = f"0x{d['subroutine_start']:06X}"
            report['breakpoint_recommendations'].append({
                'address': f"0x{d['subroutine_start']:06X}",
                'purpose': 'Dynamic DMA routine entry point',
                'what_to_check': 'Print D0-D3, A0-A1 to see DMA source/dest/length',
            })
        report['dynamic_dma_routines'].append(entry)

    for r in results['sprite_bank_refs']:
        report['sprite_bank_references'].append({
            'code_offset': f"0x{r['offset']:06X}",
            'target_addr': f"0x{r['target_addr']:06X}",
            'instruction': r['type'],
            'register': r.get('addr_reg', r.get('data_reg', '?')),
        })

    with open(output_path, 'w') as f:
        json.dump(report, f, indent=2)
    print(f"JSON report written to {output_path}")


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Analyze Sega Genesis ROM for DMA transfer patterns')
    parser.add_argument('rom', help='Path to Genesis ROM file')
    parser.add_argument('--json', '-j', dest='game_def',
                        help='Path to game definition JSON for cross-reference')
    parser.add_argument('--output', '-o',
                        help='Output path for JSON report')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Show detailed analysis output')
    args = parser.parse_args()

    # Load ROM
    with open(args.rom, 'rb') as f:
        rom_data = f.read()

    print(f"Loaded ROM: {args.rom} ({len(rom_data)} bytes)")

    # Load game definition if provided
    game_def = None
    if args.game_def:
        with open(args.game_def) as f:
            game_def = json.load(f)
        print(f"Loaded game definition: {args.game_def}")

    print()

    # Run analysis
    results = analyze_rom(rom_data, game_def, verbose=args.verbose)

    # Print summary
    print_summary(results)

    # Write JSON report
    if args.output:
        generate_json_report(results, args.output)


if __name__ == '__main__':
    main()
