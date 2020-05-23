#include "cartridge.h"
#include <stdio.h>

// PRG = cartridge side connected to the cpu
// CHR = cartridge side connected to the ppu

struct INESHeader
{
    uint32 formatMarker; // 0x4E45531A ("NES" followed by MS-DOS end-of-file)
    uint8 prgRomSize; // Size of PRG ROM in 16 KB units
    uint8 chrRomSize; // Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
    uint8 flags6;
    uint8 flags7;
    uint8 flags8;
    uint8 flags9;
    uint8 flags10;
    uint8 reserved[5];
};

void Cartridge::load(const char* file)
{
    FILE* testFile = fopen(file, "rb");
    fseek(testFile, 0, SEEK_END);
    size_t fileSize = ftell(testFile);
    char* contents = new char[fileSize];

    rewind(testFile);
    size_t bytesRead = fread(contents, sizeof(char), fileSize, testFile);

    fclose(testFile);

    uint8* readHead = (uint8*)contents;

    // Initialize the PC to the value of FFFC = lo and FFFD = hi.
    // needs to be done after the mapper is created so we grab the right code from the rom
    INESHeader* header = (INESHeader*)readHead;
    readHead += sizeof(INESHeader);

    prgRomSize = header->prgRomSize;
    prgRom = prgRomBank1 = (uint8*)readHead;
    prgRomBank2 = prgRom + kilobytes(16) * (prgRomSize - 1);

    readHead += header->prgRomSize * kilobytes(16);

    chrRom = 0;
    if (header->chrRomSize > 0)
    {
        chrRom = patternTable0 = (uint8*)readHead;
        patternTable1 = chrRom + kilobytes(4);
    }
    else
    {
        patternTable0 = chrRam;
        patternTable1 = chrRam + kilobytes(4);
    }
}

uint8 Cartridge::prgRead(uint16 address)
{
    if (address < 0x6000)
    {
        return 0;
    }

    if (address < 0x8000)
    {
        return cartRam[address - 0x6000];
    }

    if (address < 0xC000)
    {
        return prgRomBank1[address - 0x8000];
    }

    return prgRomBank2[address - 0xC000];
}

bool Cartridge::prgWrite(uint16 address, uint8 value)
{
    if (address > 0x6000 && address < 0x8000)
    {
        cartRam[address - 0x6000] = value;
        return true;
    }

    return false;
}

uint8 Cartridge::chrRead(uint16 address)
{
    if (address < 0x1000)
    {
        return patternTable0[address];
    }

    return patternTable1[address - 0x1000];
}

bool Cartridge::chrWrite(uint16 address, uint8 value)
{
    if (address < 0x1000)
    {
        patternTable0[address] = value;
    }
    else
    {
        patternTable1[address - 0x1000] = value;
    }

    return true;
}