#pragma once
#include "common.h"
#include "bus.h"

#define NES_SCREEN_HEIGHT 240
#define NES_SCREEN_WIDTH  256

class PPU
{
public:
    uint8 control;
    uint8 mask;
    uint8 status;
    uint8 oamAddress;
    uint8 oamData;
    uint8 scroll;
    uint8 ppuAddress;
    uint8 ppuData;
    uint8 oamDma;
    uint8 oam[256];

    // Temp for now, will have a screen abstraction later
    // Stores the pallette data while it's being written
    uint8 screenBuffer[NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];

    void connect(IBus* bus) { this->bus = bus; }
    void tick();

    uint32 getCycle() { return cycle; }
    uint32 getScanLine() { return scanline; }

    // These are used by the mapper to write to the internal registers wince the write has side effects
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    void writeRegister(uint16 address, uint8 value);
    uint8 readRegister(uint16 address);

    // Debugging/test functions
    void renderPatternTable();

private:
    uint32 cycle;
    uint32 scanline;
    bool isOddFrame;
    IBus* bus;

    bool writeToggle;

    uint16 vramAddress;
    uint16 tempVramAddress;
    uint8 fineScrollX;
    uint8 fineScrollY;

    uint16 patternTableShift1; // plane 1 data.. I think?
    uint16 patternTableShift2; // plane 2 data
    uint8 attributeShift1;
    uint8 attributeShift2;

    // ===========
    // temp latches for data
    // ==========
    uint8 nameTableByte; // nametable = index into pattern table
    uint8 attributeTableByte;

    // http://wiki.nesdev.com/w/index.php/PPU_nametables background evaluation
    // 4. Fetch the high-order byte of this sliver from an address 8 bytes higher.
    uint8 patternTableLo;
    uint8 patternTableHi;
};