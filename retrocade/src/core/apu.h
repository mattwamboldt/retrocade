#pragma once
#include "../common.h"

// http://wiki.nesdev.com/w/index.php/2A03
struct APU
{
    uint8 square1Vol;
    uint8 square1Sweep;
    uint8 square1Lo;
    uint8 square1Hi;

    uint8 square2Vol;
    uint8 square2Sweep;
    uint8 square2Lo;
    uint8 square2Hi;

    uint8 triangleLinear;
    uint8 triangleLo;
    uint8 triangleHi;

    uint8 noiseVol;
    uint8 noiseLo;
    uint8 noiseHi;

    uint8 dmcFrequency;
    uint8 dmcRaw;
    uint8 dmcStart;
    uint8 dmcLength;

    uint8 channelStatus;
};