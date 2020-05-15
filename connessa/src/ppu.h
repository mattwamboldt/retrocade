#pragma once
#include "common.h"

struct PPU
{
    uint8 control;
    uint8 mask;
    uint8 status;
    uint8 oamAddress;
    uint8 oamData;
    uint8 scroll;
    uint8 address;
    uint8 data;
    uint8 oamDma;
    uint8 oam[256];
};