#pragma once
#include "common.h"

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
    uint8 cartRam[8 * 1024] = {};
    uint8* map000prgRomBank1;
    uint8* map000prgRomBank2;
    uint8* map000prgRam;
    uint8* chrRom;
};