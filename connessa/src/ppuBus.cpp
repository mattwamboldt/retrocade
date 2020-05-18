#include "ppuBus.h"

void PPUBus::connect(PPU* ppu, Cartridge* cart)
{
    this->cart = cart;
}

void PPUBus::addWriteCallback(WriteCallback cb)
{
    writeCallback = cb;
}

// This read and write is based on mapper000, will expand later
uint8 PPUBus::read(uint16 address)
{
    if (address < 0x2000)
    {
        return cart->chrRead(address);
    }

    if (address < 0x3F00)
    {
        address &= 0x2FFF;
        // TODO: handle mirroring configuration
        return vram[address & 0x07FF];
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;
    switch (address)
    {
        case 0x10: return paletteRam[0x00];
        case 0x14: return paletteRam[0x04];
        case 0x18: return paletteRam[0x08];
        case 0x1C: return paletteRam[0x0C];
        default: return paletteRam[address];
    }
}

uint16 PPUBus::readWord(uint16 address)
{
    uint8 lo = read(address);
    uint8 hi = read(address + 1);
    return ((uint16)hi << 8) + lo;
}

void PPUBus::write(uint16 address, uint8 value)
{
    if (address < 0x2000)
    {
        cart->chrWrite(address, value);
        return;
    }

    if (address < 0x3F00)
    {
        address &= 0x2FFF;
        // TODO: handle mirroring configuration
        vram[address & 0x07FF] = value;
        return;
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;
    switch (address)
    {
        case 0x10: paletteRam[0x00] = value;
        case 0x14: paletteRam[0x04] = value;
        case 0x18: paletteRam[0x08] = value;
        case 0x1C: paletteRam[0x0C] = value;
        default: paletteRam[address] = value;
    }
}