#pragma once
#include "bus.h"
#include "ppu.h"
#include "apu.h"
#include "cartridge.h"

class CPUBus : public IBus
{
public:
    void connect(PPU* ppu, APU* apu, Cartridge* cart);
    void addWriteCallback(WriteCallback cb);

    uint8 read(uint16 address);
    uint16 readWord(uint16 address);
    void write(uint16 address, uint8 value);

private:
    PPU* ppu;
    APU* apu;
    Cartridge* cart;

    uint8 ram[2 * 1024];
    uint8 joy1;
    uint8 joy2;

    WriteCallback writeCallback;
};