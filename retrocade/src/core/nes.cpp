#include "nes.h"

void NES::update(real32 secondsPerFrame)
{
    if (cpu.hasHalted())
    {
        return;
    }

    // TODO: calculate this rate properly and preserve extra cycles in a variable to run later
    // cpu and ppu per needed timings, cpu and apu every 12, ppu every 4

    // TODO: There s a note on the wiki that mentions having a hard coded clock rate
    // "Emulator authors may wish to emulate the NTSC NES/Famicom CPU at 21441960 Hz ((341�262-0.5)�4�60) to ensure a synchronised/stable 60 frames per second."
    // I don't understand how this works, so for now, I'll just calculate it
    int32 masterClockHz = 21477272;
    int32 masterCycles = (int32)(secondsPerFrame * masterClockHz);
    for (int i = 0; i < masterCycles; ++i)
    {
        if (i % 12 == 0)
        {
            singleStep();
        }

        if (i % 4 == 0)
        {
            ppu.tick();
        }
    }

    if (ppu.getVBlankActive() && !wasVBlankActive)
    {
        // flushScreenBuffer();
    }

    wasVBlankActive = ppu.getVBlankActive();
}