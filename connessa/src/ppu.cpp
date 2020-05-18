#include "ppu.h"

// OVERALL TODO:
// - Handle Mask
// - handle oam data transfer
// - handle sprite rendering
// - handle render disable

void PPU::tick()
{
    if (cycle == 0)
    {
        return;
    }

    // extracting info from ppuctrl
    // http://wiki.nesdev.com/w/index.php/PPU_registers#Controller_.28.242000.29_.3E_write
    uint16 nameTableBase = 0x2000 + ((control & 0b00000011) * 0x400);
    uint8 vramIncrement = 1;
    if (control & 0b00000100)
    {
        vramIncrement = 32;
    }

    uint16 spriteTableAddress = 0;
    bool largeSprites = control & 0b00010000 > 0;
    if (!largeSprites && control & 0b00001000)
    {
        spriteTableAddress = 0x1000;
    }

    bool generateNMI = control & 0b10000000;
    
    // This now makes sense..! I hope... Took awhile to colate all this into one process
    // http://wiki.nesdev.com/w/index.php/PPU_rendering#Cycles_1-256
    // vram address is pointing to a location in the nametable
    // first byte we read the value at this address, which is an index into the pattern table
    // http://wiki.nesdev.com/w/index.php/PPU_nametables
    // second byte, we read the attribute byte for that nametable location
    // http://wiki.nesdev.com/w/index.php/PPU_attribute_tables
    // third and forth byte, we use the nametable value to get the two planes of the pattern data
    // http://wiki.nesdev.com/w/index.php/PPU_pattern_tables
    // then we advance by the vramincrement and store our results into the shift registers
    // Repeat... forever!! with some excetions for vblanking, and some other stuff

    // The attribute byte determines which of four pallettes to use and
    // the pattern bits give us which of four colours in that palette to use
    // These actually form an address into paletteRam, which is filled with colors
    // From the available nes palette. http://wiki.nesdev.com/w/index.php/PPU_palettes
    // These palettes are in ram and can be adjusted at run time, which would explain how
    // tetris level colorization would have worked, also means you can only have
    // 16 colors at once without mid frame shenanigans

    // Sprites are evaluated in a slightly more complex but similar way out of object memory
    // then rendered instead of the background value depending on priority

    // Scrolling within an 8 bit tile is done by reading bits further in on the shift registers
    // and lower down in the nametable than a given scanline is supposed to
    // http://wiki.nesdev.com/w/index.php/PPU_scrolling (lots of other good stuff on this page)

    if (cycle % 8 == 0)
    {
        // nameTableByte = bus->read();
        // attributeTableByte = bus->read();
        // patternTableLo = bus->read();
        // patternTableHi = bus->read();
    }

    // TODO: Implement vblank period, statuss interrupts idle etc.
    ++cycle;
    if (cycle > 340)
    {
        // TODO: handle wrap around to next frame render
        ++scanline;
        cycle = 0;
    }
}

// TODO: Implement side effects give here http://wiki.nesdev.com/w/index.php/PPU_scrolling#Register_controls
void PPU::writeRegister(uint16 address, uint8 value)
{
    if (address == 0x4014)
    {
        oamData = value;
        return;
    }

    // map to the ppu register based on the last 3 bits
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    uint16 masked = address & 0x0007;
    switch (masked)
    {
        case 0x00: control = value; break;
        case 0x01: mask = value; break;
        case 0x02: status = value; break;
        case 0x03: oamAddress = value; break;
        case 0x04: oamData = value; break;
        case 0x05: scroll = value; break;
        case 0x06: ppuAddress = value; break;
        case 0x07: ppuData = value; break;
    }
}

uint8 PPU::readRegister(uint16 address)
{
    if (address == 0x4014)
    {
        return oamData; // TODO: advance oamaddr per write
    }

    // map to the ppu register based on the last 3 bits
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    // TODO: might be better to store the registers as an array or union
    // doing the dumb obvious thing for now
    uint16 masked = address & 0x0007;
    switch (masked)
    {
        case 0x00: return control; // TODO: Should we care about readonly and writeonly?
        case 0x01: return mask;
        case 0x02: return status;
        case 0x03: return oamAddress;
        case 0x04: return oamData;
        case 0x05: return scroll; // TODO: handle double write logic
        case 0x06: return ppuAddress; // TODO: handle double write logic
        case 0x07: return ppuData; // TODO: advance teh vramaddress by ctrl increment
        default: return 0;
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