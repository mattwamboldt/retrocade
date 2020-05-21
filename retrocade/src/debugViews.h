#pragma once
#include "common.h"

class DebugView
{
public:
    void render();

    void nextMemPage();
    void prevMemPage();

    void toggleAsciiMode() { asciiMode  = !asciiMode; }

private:
    bool asciiMode = false;
    uint8 debugMemPage = 0x60;

    void renderMemoryView();
    void drawFps(real32 frameElapsed);
    void drawCpuStatus();
};