#pragma once
#include "6502.h"
#include "ppu.h"
#include "apu.h"
#include "cpuBus.h"
#include "ppuBus.h"

class NES
{
public: // making public for now, will wrap this as needed later
    MOS6502 cpu = {};
    PPU ppu = {};
    PPUBus ppuBus = {};
    APU apu = {};
    CPUBus cpuBus = {};
    Cartridge cartridge = {};

    NES();
    void loadRom(const char* path);
    void update(real32 secondsPerFrame);
    void singleStep();

    void toggleSingleStep() { singleStepMode = !singleStepMode; }

private:
    bool wasVBlankActive;
    bool traceEnabled;
    bool singleStepMode;

    void cpuStep();
};