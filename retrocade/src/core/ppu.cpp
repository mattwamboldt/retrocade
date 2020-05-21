#include "ppu.h"
#include <intrin.h>
#include <Windows.h>

// OVERALL TODO:
// - Handle Mask
// - handle oam data transfer
// - handle sprite rendering
// - handle render disable

// PERF: This seems to be the limiter on framerate currently
// Tried using a mux operation with two shifts, then unrolling to this, neither seemed to help much
// could be that doing one pixel per cycle is wildly thrashing the cache and I should instead attempt to do
// the whole tile every 8 cycles. or there are some bit tricks that could help me here. Needs time...
// Switching to another task for now
uint8 PPU::calcBackgroundPixel()
{
    uint8 mask = 0b10000000 >> fineScrollX;
    uint8 offset = 7 - fineScrollX;

    uint8 backgroundColor = ((uint8)(patternShiftLo >> 8) & mask) >> offset;
    --offset;
    backgroundColor |= ((uint8)(patternShiftHi >> 8) & mask) >> offset;
    --offset;
    backgroundColor |= (attributeShiftLo & mask) >> offset;
    --offset;
    backgroundColor |= (attributeShiftHi & mask) >> offset;
    --offset;

    patternShiftLo <<= 1;
    patternShiftHi <<= 1;
    attributeShiftLo <<= 1;
    attributeShiftHi <<= 1;

    return backgroundColor;
}

void PPU::tick()
{
    if (cycle == 0)
    {
        ++cycle;
        return;
    }

    if (cycle == 1)
    {
        if (scanline == 241)
        {
            vBlankActive = true;
        }

        if (scanline == 261)
        {
            vBlankActive = false;
            spriteZeroHit = false;
            overflowSet = false;
            outputOffset = 0;
        }
    }
    
    // TODO: calculate the current pixel from shift registers
    if (cycle <= NES_SCREEN_WIDTH && scanline < NES_SCREEN_HEIGHT)
    {
        uint8 backgroundColor = calcBackgroundPixel();

        // TODO: sprite evaluation
        screenBuffer[outputOffset++] = backgroundColor;
    }

    // Perform data fetches
    switch (cycle % 8)
    {
        case 2: nameTableByte = bus->read(vramAddress); break;
        case 4: attributeTableByte = bus->read(vramAddress); break; // TODO: calculate attribute address
        // TODO: THIS IS WRONG!!!
        // the nametable byte is an index into which tile to pull out of the 256 available
        // but thats not directly mappable to the pattern table address, we need some math in here
        // I'm a dumb dumb!
        // http://wiki.nesdev.com/w/index.php/PPU_pattern_tables
        case 6: patternTableLo = bus->read(nameTableByte); break; // TODO: grab correct "side"
        case 0: patternTableHi = bus->read(nameTableByte + 8); break;
    }

    if (cycle % 8 == 0)
    {
        patternShiftLo &= 0xFF00;
        patternShiftLo |= patternTableLo;
        patternShiftHi &= 0xFF00;
        patternShiftHi |= patternTableHi;

        if (cycle < NES_SCREEN_WIDTH || cycle > 320)
        {
            if ((vramAddress & 0x001F) == 31)
            {
                vramAddress &= 0xFFE0;
                vramAddress ^= 0x0400;
            }
            else
            {
                ++vramAddress;
            }
        }
        else if (cycle == NES_SCREEN_WIDTH)
        {
            if ((vramAddress & 0x7000) != 0x7000)
            {
                vramAddress += 0x1000;
            }
            else
            {
                vramAddress &= ~0x7000;
                int tileRow = (vramAddress & 0x03E0) >> 5;
                if (tileRow == 29)
                {
                    tileRow = 0;
                    vramAddress ^= 0x0800;
                }
                else if (tileRow == 31)
                {
                    tileRow = 0;
                }
                else
                {
                    ++tileRow;
                }

                vramAddress = (vramAddress & ~0x03E0) | (tileRow << 5);
            }
        }
    }

    if (cycle == 257)
    {
        vramAddress &= ~0x041F;
        vramAddress |= tempVramAddress & 0x041F;
    }

    if (scanline == 261 && cycle >= 280 && cycle <= 304)
    {
        vramAddress &= ~0x7BE0;
        vramAddress |= tempVramAddress & 0x7BE0;
    }

    ++cycle;
    if (cycle > 340)
    {
        ++scanline;
        cycle = 0;

        if (scanline > 261)
        {
            scanline = 0;
            if (isOddFrame)
            {
                scanline = 1;
            }

            isOddFrame = !isOddFrame;
        }
    }

}

// Side effects http://wiki.nesdev.com/w/index.php/PPU_scrolling#Register_controls
void PPU::writeRegister(uint16 address, uint8 value)
{
    if (address == 0x4014)
    {
        // TODO: write a function at the console level to engage a dma process
        // that stalls the cpu and transfers this page into oamaddr
        oamDma = value;
        return;
    }

    // map to the ppu register based on the last 3 bits
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    uint16 masked = address & 0x0007;
    switch (masked)
    {
        case 0x00: 
        {
            tempVramAddress &= 0xF3FF;
            tempVramAddress |= (uint16)(value & 0b00000011) << 10;
            vramIncrement = (value & 0b00000100) ? 32 : 1;
            spritePatternBase = (value & 0b00001000) ? 0x1000 : 0;
            backgroundPatternBase = (value & 0b00010000) ? 0x1000 : 0;
            useTallSprites = (value & 0b00100000) > 0;
            generateNMIOnVBlank = (value & 0b10000000) > 0;
            break;
        }
        case 0x01: mask = value; break;
        case 0x03: oamAddress = value; break;
        case 0x04: oam[oamAddress++] = value; break;
        case 0x05: 
        {
            if (!writeToggle)
            {
                fineScrollX = value & 0b00000111;
                tempVramAddress &= 0xFFE0;
                tempVramAddress |= (value & 0b11111000) >> 3;
            }
            else
            {
                tempVramAddress &= 0x8C1F;
                tempVramAddress |= (uint16)(value & 0b00000111) << 12;
                tempVramAddress |= (uint16)(value & 0b11111000) << 2;
            }

            writeToggle = !writeToggle;
            break;
        }
        break;
        case 0x06:
        {
            if (!writeToggle)
            {
                tempVramAddress &= 0x00FF;
                tempVramAddress |= (uint16)value << 8;
            }
            else
            {
                tempVramAddress &= 0xFF00;
                tempVramAddress |= value;
                vramAddress = tempVramAddress;
            }

            writeToggle = !writeToggle;
        }
        break;
        case 0x07:
        {
            bus->write(vramAddress, value);
            vramAddress += vramIncrement;
        }
        break;
    }
}

uint8 PPU::readRegister(uint16 address)
{
    if (address == 0x4014)
    {
        return 0; // write only
    }

    // map to the ppu register based on the last 3 bits
    uint16 masked = address & 0x0007;
    switch (masked)
    {
        case 0x02:
        {
            writeToggle = 0;
            uint8 status = 0;
            if (vBlankActive)
            {
                status |= 0b10000000;
            }

            if (spriteZeroHit)
            {
                status |= 0b01000000;
            }

            if (overflowSet)
            {
                status |= 0b00100000;
            }

            return status;
        }
        case 0x03: return oamAddress;
        case 0x04: return oam[oamAddress]; // TODO: unsure about autoincrement
        case 0x07: // TODO: there's mention of an internal read buffer that may need to be handled, alo unsure about read incrementing
        {
            uint8 result = bus->read(vramAddress);
            vramAddress += vramIncrement;
            return result;
        }
        default: return 0; // TODO: handle reads from writeonly registers
    }
}

void PPU::renderPatternTable()
{
    // TODO: on further research this won't even work without putting stuff into palette ram
    // so come up with an initial pallet selection. this is actually good because the console is limited

    // Easier way for now is to just use an array with existing palette values

    // Getting my feet wet, this function renders out the current pattern table
    int32 x = 0;
    int32 y = 0;

    uint16 patternAddress = 0x0000;
    
    for (int page = 0; page < 2; ++page)
    {
        for (int yTile = 0; yTile < 16; ++yTile)
        {
            for (int xTile = 0; xTile < 16; ++xTile)
            {
                for (int tileY = 0; tileY < 8; ++tileY)
                {
                    uint8 patfield01 = bus->read(patternAddress);
                    uint8 patfield02 = bus->read(patternAddress + 8);

                    for (int bit = 0; bit < 8; ++bit)
                    {
                        uint8 color = (patfield01 & 0b10000000) >> 7;
                        color |= (patfield02 & 0b10000000) >> 6;
                        patfield01 <<= 1;
                        patfield02 <<= 1;
                        screenBuffer[y * NES_SCREEN_WIDTH + x++] = color;
                    }

                    ++patternAddress;
                    x -= 8;
                    ++y;
                }

                patternAddress += 8;
                y -= 8;
                x += 8;
            }

            x -= 128;
            y += 8;
        }

        y = 0;
        x = 128;
    }
}