#pragma once
#include "bus.h"
#include "ppu.h"
#include "cartridge.h"

class PPUBus : public IBus
{
public:
    void connect(PPU* ppu, Cartridge* cart);
    void addWriteCallback(WriteCallback cb);

    uint8 read(uint16 address);
    void write(uint16 address, uint8 value);

private:
    PPU* ppu;
    Cartridge* cart;

    uint8 vram[2 * 1024] = {};
    uint8 paletteRam[32];

    WriteCallback writeCallback;
};