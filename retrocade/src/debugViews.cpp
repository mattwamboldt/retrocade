#include "debugViews.h"

void DebugView::renderMemoryView()
{
    /*
    DrawText("CPU (M)emory", ?, ?);

    cursorPos.Y += textheight + padding;
    x += some amount
    
    for (int i = 0; i < 16; ++i)
    {
        DrawText(formatByte(i));
        x += some amount
    }

    y += textheight + padding
    x = box x

    const int ROWS_TO_DISPLAY = 32;

    uint16 address = ((uint16)debugMemPage) << 8;
    while (cursorPos.Y < ROWS_TO_DISPLAY + 3)
    {
        DrawText(formatAddress(address);
        x += some amount

        for (int i = 0; i < 16; ++i)
        {
            uint8 d = cpuBus.read(address + i);
            if (!asciiMode || !isprint(d))
            {
                DrawText(formatByte(d));
            }
            else
            {
                DrawText((char)d);
            }
        }

        ++cursorPos.Y;
        address += 16;
    }
    */
}

void DebugView::drawFps(real32 frameElapsed)
{
    real32 msPerFrame = 1000.0f * frameElapsed;
    if (msPerFrame)
    {
        real32 framesPerSecond = 1000.0f / msPerFrame;
        // DrawText("MS Per Frame = %08.2f FPS: %.2f", msPerFrame, framesPerSecond);
    }
}

void DebugView::drawCpuStatus()
{
    /*HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 58, 1 };
    SetConsoleCursorPosition(console, cursorPos);

    printf("CPU Registers");

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);

    printf("A=%02X X=%02X Y=%02X", cpu.accumulator, cpu.x, cpu.y);

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);
    printf("PC=%04hX S=0x01%02X P=%02X", cpu.pc, cpu.stack, cpu.status);

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);

    uint8 carry = cpu.status & 0x01;
    uint8 zero = (cpu.status & 0x02) >> 1;
    uint8 interuptDisable = (cpu.status & 0x04) >> 2;
    uint8 overflow = (cpu.status & 0x40) >> 6;
    uint8 negative = (cpu.status & 0x80) >> 7;
    printf("C=%d Z=%d I=%d V=%d N=%d", carry, zero, interuptDisable, overflow, negative);

    cursorPos.Y += 3;
    SetConsoleCursorPosition(console, cursorPos);
    printf("                                  ");
    SetConsoleCursorPosition(console, cursorPos);
    printf("Current Inst: ");
    printInstruction(cpu.instAddr);

    uint8 opcode = cpuBus.read(address);
    uint8 p1 = cpuBus.read(address + 1);
    uint8 p2 = cpuBus.read(address + 2);

    char instructionBuffer[32];
    formatInstruction(instructionBuffer, address, &cpu, &cpuBus);
    printf("0x%04X %-30s", address, instructionBuffer);

    */
}

void DebugView::render()
{
    renderMemoryView();

    drawCpuStatus();

    // drawFps();
}

void DebugView::nextMemPage()
{
    debugMemPage += 2;
}

void DebugView::prevMemPage()
{
    debugMemPage -= 2;
}