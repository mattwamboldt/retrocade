#pragma once
#include "common.h"
#include "bus.h"

#define NES_SCREEN_HEIGHT 240
#define NES_SCREEN_WIDTH  256

class PPU
{
public:
    uint8 mask;
    uint8 oamAddress;
    uint8 oamDma;
    uint8 oam[256];

    // Temp for now, will have a screen abstraction later
    // Stores the pallette data while it's being written
    uint8 screenBuffer[NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];

    void connect(IBus* bus) { this->bus = bus; }
    void tick();

    uint32 getCycle() { return cycle; }
    uint32 getScanLine() { return scanline; }
    bool getVBlankActive() { return vBlankActive; }

    // These are used by the mapper to write to the internal registers wince the write has side effects
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    void writeRegister(uint16 address, uint8 value);
    uint8 readRegister(uint16 address);

private:
    uint32 cycle;
    uint32 scanline;
    bool isOddFrame;
    IBus* bus;

    bool writeToggle;

    uint16 outputOffset;

    uint16 vramAddress;
    uint16 tempVramAddress;
    uint8 fineScrollX;
    uint8 vramIncrement;
    bool generateNMIOnVBlank;
    bool useTallSprites;
    bool vBlankActive;
    bool spriteZeroHit;
    bool overflowSet;
    uint16 spritePatternBase;
    uint16 backgroundPatternBase;

    uint16 patternShiftLo; // plane 1 data.. I think?
    uint16 patternShiftHi; // plane 2 data
    uint8 attributeShiftLo; // These both point to the same attribute register, just shifted for the mux to work well... I think
    uint8 attributeShiftHi;

    // ===========
    // temp latches for data
    // ==========
    uint8 nameTableByte; // nametable = index into pattern table
    uint8 attributeTableByte;
    uint8 patternTableLo;
    uint8 patternTableHi;

    uint8 calcBackgroundPixel();
};