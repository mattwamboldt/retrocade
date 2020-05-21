#include "cpuBus.h"

void CPUBus::connect(PPU* ppu, APU* apu, Cartridge* cart)
{
    this->ppu = ppu;
    this->apu = apu;
    this->cart = cart;
}

void CPUBus::addWriteCallback(WriteCallback cb)
{
    writeCallback = cb;
}

uint8 CPUBus::read(uint16 address)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        return ram[address & 0x07FF];
    }

    if (address < 0x4000)
    {
        return ppu->readRegister(address);
    }

    if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        // TODO: docs mention these as readonly, remove if that matters
        switch (address)
        {
            case 0x4000: return apu->square1Vol;
            case 0x4001: return apu->square1Sweep;
            case 0x4002: return apu->square1Lo;
            case 0x4003: return apu->square1Hi;
            case 0x4004: return apu->square2Vol;
            case 0x4005: return apu->square2Sweep;
            case 0x4006: return apu->square2Lo;
            case 0x4007: return apu->square2Vol;
            case 0x4008: return apu->triangleLinear;
            case 0x400A: return apu->triangleLo;
            case 0x400B: return apu->triangleHi;
            case 0x400C: return apu->noiseVol;
            case 0x400E: return apu->noiseLo;
            case 0x400F: return apu->noiseHi;
            case 0x4010: return apu->dmcFrequency;
            case 0x4011: return apu->dmcRaw;
            case 0x4012: return apu->dmcStart;
            case 0x4013: return apu->dmcLength;
            case 0x4014: return ppu->readRegister(address);
            case 0x4015: return apu->channelStatus;
            case 0x4016: return joy1;
            case 0x4017: return joy2;
            default: return 0;
        }
    }

    return cart->prgRead(address);
}


uint16 CPUBus::readWord(uint16 address)
{
    uint8 lo = read(address);
    uint8 hi = read(address + 1);
    return ((uint16)hi << 8) + lo;
}

void CPUBus::write(uint16 address, uint8 value)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        ram[address & 0x07FF] = value;
        if (writeCallback) writeCallback(address, value);
    }
    else if (address < 0x4000)
    {
        ppu->writeRegister(address, value);
        if (writeCallback) writeCallback(address, value);
    }
    else if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        switch (address)
        {
            case 0x4000: apu->square1Vol = value; break;
            case 0x4001: apu->square1Sweep = value; break;
            case 0x4002: apu->square1Lo = value; break;
            case 0x4003: apu->square1Hi = value; break;
            case 0x4004: apu->square2Vol = value; break;
            case 0x4005: apu->square2Sweep = value; break;
            case 0x4006: apu->square2Lo = value; break;
            case 0x4007: apu->square2Vol = value; break;
            case 0x4008: apu->triangleLinear = value; break;
            case 0x400A: apu->triangleLo = value; break;
            case 0x400B: apu->triangleHi = value; break;
            case 0x400C: apu->noiseVol = value; break;
            case 0x400E: apu->noiseLo = value; break;
            case 0x400F: apu->noiseHi = value; break;
            case 0x4010: apu->dmcFrequency = value; break;
            case 0x4011: apu->dmcRaw = value; break;
            case 0x4012: apu->dmcStart = value; break;
            case 0x4013: apu->dmcLength = value; break;
            case 0x4014: ppu->writeRegister(address, value); break;
            case 0x4015: apu->channelStatus = value; break;
            case 0x4016: joy1 = value; break;
            case 0x4017: joy2 = value; break;
            default: return;
        }

        if (writeCallback) writeCallback(address, value);
    }
    // Cartridge space (logic depends on the mapper)
    else if (cart->prgWrite(address, value))
    {
        if (writeCallback) writeCallback(address, value);
    }
}
