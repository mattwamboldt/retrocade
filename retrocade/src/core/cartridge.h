#pragma once
#include "../common.h"

class Cartridge
{
public:
    void load(const char* file);

    uint8 prgRead(uint16 address);
    bool prgWrite(uint16 address, uint8 value);
    uint8 chrRead(uint16 address);
    bool chrWrite(uint16 address, uint8 value);

private:
    // TODO: This stuff will change when we get multiple mappers
    uint8 cartRam[kilobytes(8)] = {};
    uint8 prgRomSize;
    uint8* prgRom;
    uint8* prgRomBank1;
    uint8* prgRomBank2;

    uint8* chrRom;
    uint8 chrRam[kilobytes(8)];
    uint8* patternTable0;
    uint8* patternTable1;
};