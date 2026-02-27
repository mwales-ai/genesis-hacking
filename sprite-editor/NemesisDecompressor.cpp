#include "NemesisDecompressor.h"
#include <QMessageBox>
#include <vector>

// Nemesis decompression for Sega Genesis 4bpp tile data.
//
// Format reference: https://segaretro.org/Nemesis_compression
//
// Header (2 bytes, big-endian):
//   bit 15     : XOR mode flag
//   bits 14-0  : tile count (0 = 256 tiles)
//
// Code table (follows header, terminated by 0xFF):
//   Group header byte 0x80-0x8F  : set current nibble to (byte & 0x0F)
//   Code entry (0x00-0x7F) + code byte:
//     descriptor[6:4] : repeat count - 1  (1-8 copies)
//     descriptor[3:0] : code length in bits (1-8)
//     code byte       : left-justified code bits
//
// Special inline literal in bitstream:
//   Six consecutive 1-bits followed by 7 bits [count_minus1:3][nibble:4]
//
// XOR mode: each row (4 bytes) within a tile is XORed with the previous row.

QByteArray NemesisDecompressor::decompress(const QByteArray &romData,
                                            uint32_t offset,
                                            uint32_t *outCompressedSize)
{
    if (outCompressedSize) *outCompressedSize = 0;

    const int dataSize = romData.size();
    if (int(offset) + 2 > dataSize) return {};

    // ---- Header ----
    uint16_t header  = (uint8_t(romData[offset]) << 8) | uint8_t(romData[offset + 1]);
    bool     xorMode  = (header & 0x8000) != 0;
    int      tileCnt  = header & 0x7FFF;
    if (tileCnt == 0) tileCnt = 256;

    int pos = int(offset) + 2;

    // ---- Code table ----
    // table[codeLen * 256 + codeBits] = {nibble, repeatCount, valid}
    struct Entry { uint8_t nibble; uint8_t count; bool valid; };
    std::vector<Entry> table(9 * 256, {0, 0, false});

    uint8_t curNibble = 0;
    while (pos < dataSize) {
        uint8_t b = uint8_t(romData[pos++]);
        if (b == 0xFF) break;                // end of table

        if (b & 0x80) {                      // group header
            curNibble = b & 0x0F;
            continue;
        }

        // Code entry
        int repeatCnt = ((b >> 4) & 0x07) + 1;  // bits 6-4 + 1  → 1..8
        int codeLen   =   b & 0x0F;              // bits 3-0      → 1..8
        if (codeLen == 0 || codeLen > 8 || pos >= dataSize) break;

        uint8_t codeByte = uint8_t(romData[pos++]);
        int     codeBits = codeByte >> (8 - codeLen);   // left-justified → right-aligned

        table[codeLen * 256 + codeBits] = { curNibble, uint8_t(repeatCnt), true };
    }

    // ---- Bitstream decoding ----
    int totalNibbles = tileCnt * 64;   // 8×8 pixels, 4bpp → 64 nibbles/tile
    QByteArray raw(tileCnt * 32, '\0');

    int outByte  = 0;
    int outHalf  = 0;    // 0 = high nibble of current byte, 1 = low nibble
    int nibsDone = 0;

    // Bit reader (MSB first within each byte)
    uint8_t bitBuf   = 0;
    int     bitsLeft = 0;

    auto readBit = [&]() -> int {
        if (bitsLeft == 0) {
            if (pos >= dataSize) return -1;
            bitBuf   = uint8_t(romData[pos++]);
            bitsLeft = 8;
        }
        return (bitBuf >> --bitsLeft) & 1;
    };

    auto putNibble = [&](uint8_t n) {
        if (outByte >= int(raw.size())) return;
        if (outHalf == 0) {
            raw[outByte] = char(n << 4);
            outHalf = 1;
        } else {
            raw[outByte] = char(uint8_t(raw[outByte]) | n);
            ++outByte;
            outHalf = 0;
        }
        ++nibsDone;
    };

    bool streamOk = true;
    while (nibsDone < totalNibbles && streamOk) {
        int acc = 0, accLen = 0;
        bool found = false;

        while (accLen < 8 && !found) {
            int bit = readBit();
            if (bit < 0) { streamOk = false; break; }
            acc = (acc << 1) | bit;
            ++accLen;

            // Inline literal: six 1-bits in a row
            if (acc == 0x3F && accLen == 6) {
                int extra = 0;
                for (int i = 0; i < 7 && streamOk; ++i) {
                    int b2 = readBit();
                    if (b2 < 0) streamOk = false;
                    else        extra = (extra << 1) | b2;
                }
                if (streamOk) {
                    uint8_t cnt = uint8_t((extra >> 4) & 0x07) + 1;
                    uint8_t nib = uint8_t(extra & 0x0F);
                    for (int i = 0; i < cnt && nibsDone < totalNibbles; ++i)
                        putNibble(nib);
                }
                found = true;
                break;
            }

            // Huffman table lookup
            Entry &e = table[accLen * 256 + acc];
            if (e.valid) {
                for (int i = 0; i < e.count && nibsDone < totalNibbles; ++i)
                    putNibble(e.nibble);
                found = true;
            }
        }

        if (!found) break;   // no valid code — likely end of compressed data
    }

    // ---- XOR mode post-processing ----
    // Apply cumulative forward XOR per tile: actual_row[i] = raw_row[i] ^ actual_row[i-1]
    if (xorMode) {
        for (int tile = 0; tile < tileCnt; ++tile) {
            int base = tile * 32;
            for (int row = 1; row < 8; ++row) {
                for (int col = 0; col < 4; ++col)
                    raw[base + row*4 + col] ^= raw[base + (row-1)*4 + col];
            }
        }
    }

    if (outCompressedSize) *outCompressedSize = uint32_t(pos) - offset;
    return raw;
}

QByteArray NemesisDecompressor::compress(const QByteArray & /*srcData*/)
{
    QMessageBox::warning(nullptr, "Not Implemented",
        "Nemesis re-compression is not yet implemented.\n"
        "Only sprites with compression 'none' can be replaced.");
    return {};
}
