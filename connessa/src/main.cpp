#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include "common.h"
#include "6502.h"
#include "ppu.h"
#include "apu.h"
#include "cpuBus.h"

bool running = true;

// globals are bad!!! change this later!!!
static MOS6502 cpu = {};
static PPU ppu = {};
static APU apu = {};
static CPUBus cpuBus = {};
static Cartridge cartridge = {};

static bool renderMode = false;
static uint8 debugMemPage;

void renderMemCell(uint16 address, uint8 val)
{
    if (renderMode)
    {
        return;
    }

    if (address >> 8 != debugMemPage && address >> 8 != debugMemPage + 1)
    {
        return;
    }

    uint8 x = (uint8)(address & 0x000F);
    uint8 y = (address & 0x01F0) >> 4;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 7, 2 };
    cursorPos.X += x * 3;
    cursorPos.Y += y;
    SetConsoleCursorPosition(console, cursorPos);
    printf("%02X", val);
}

void setConsoleSize(int16 cols, int16 rows, int charWidth, int charHeight)
{
    // console buffer can never be smaller than the window and you cant set them in tandem
    // so we have to shrink the window to nothing, adjust the buffer then resize back to where we want
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowRect = {0, 0, 1, 1};
    BOOL result = SetConsoleWindowInfo(console, TRUE, &windowRect);

    COORD newSize = {cols, rows};
    result = SetConsoleScreenBufferSize(console, newSize);

    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(fontInfo);
    fontInfo.nFont = 0;
    fontInfo.dwFontSize.X = charWidth;
    fontInfo.dwFontSize.Y = charHeight;
    fontInfo.FontFamily = FF_DONTCARE;
    fontInfo.FontWeight = FW_NORMAL;
    wcscpy_s(fontInfo.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(console, false, &fontInfo);

    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    result = GetConsoleScreenBufferInfo(console, &bufferInfo);

    windowRect = {0, 0, bufferInfo.dwMaximumWindowSize.X - 1, bufferInfo.dwMaximumWindowSize.Y - 1 };
    result = SetConsoleWindowInfo(console, TRUE, &windowRect);
}

void hideCursor()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 10;
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(console, &cursorInfo);
}

void showCursor()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 10;
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(console, &cursorInfo);
}

char instructionBuffer[32];
FILE* logFile = 0;

void formatInstruction(uint16 address)
{
    uint8 opcode = cpuBus.read(address);
    uint8 p1 = cpuBus.read(address + 1);
    uint8 p2 = cpuBus.read(address + 2);

    Operation op = operations[opcode];
    char* s = instructionBuffer;
    sprintf(s, "%s", opCodeNames[op.opCode]);
    s += strlen(s);

    address = cpu.calcAddress(op.addressMode, address, p1, p2);

    switch (op.addressMode)
    {
        case Absolute:
        {
            if (op.opCode == JMP || op.opCode == JSR)
            {
                sprintf(s, " $%04X", address);
            }
            else
            {
                uint8 result = cpuBus.read(address);
                sprintf(s, " $%04X = #$%02X", address, result);
            }
        }
        break;
        case AbsoluteX:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " $%02X%02X,X @ $%04X = #$%02X", p2, p1, address, result);
        }
        break;
        case AbsoluteY:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " $%02X%02X,Y @ $%04X = #$%02X", p2, p1, address, result);
        }
        break;
        case Immediate:
            sprintf(s, " #$%02X", p1);
            break;
        case Indirect:
        {
            sprintf(s, " $%04X", address);
        }
        break;
        case IndirectX:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " ($%02X,X) @ $%04X = #$%02X", p1, address, result);
        }
        break;
        case IndirectY:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " ($%02X),Y @ $%04X = #$%02X", p1, address, result);
        }
        break;
        case Relative:
            sprintf(s, " $%04X", address);
            break;
        case ZeroPage:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " $%04X = #$%02X", address, result);
        }
        break;
        case ZeroPageX:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " $%02X,X @ $%04X = #$%02X", p1, address, result);
        }
        break;
        case ZeroPageY:
        {
            uint8 result = cpuBus.read(address);
            sprintf(s, " $%02X,Y @ $%04X = #$%02X", p1, address, result);
        }
        break;
    }
}

void logInstruction(uint16 address)
{
    formatInstruction(address);
    if (!logFile)
    {
        logFile = fopen("data/log.txt", "w");
    }

    uint8 opcode = cpuBus.read(address);
    uint8 p1 = cpuBus.read(address + 1);
    uint8 p2 = cpuBus.read(address + 2);
    
    Operation op = operations[opcode];

    char hexFormAddress[20] = {};
    switch (op.addressMode)
    {
    case Absolute:
    case AbsoluteX:
    case AbsoluteY:
    case Indirect:
        sprintf(hexFormAddress, "$%04X:%02X %02X %02X", address, opcode, p1, p2);
        break;
    case Immediate:
    case IndirectX:
    case IndirectY:
    case Relative:
    case ZeroPage:
    case ZeroPageX:
    case ZeroPageY:
        sprintf(hexFormAddress, "$%04X:%02X %02X", address, opcode, p1);
        break;
    default:
        sprintf(hexFormAddress, "$%04X:%02X", address, opcode);
    }

    char registerPrintout[32] = {};
    sprintf(registerPrintout, "A:%02X X:%02X Y:%02X S:%02X P:nvubdizc",
        cpu.accumulator, cpu.x, cpu.y, cpu.stack);

    if (cpu.isFlagSet(STATUS_NEGATIVE)) registerPrintout[22] = 'N';
    if (cpu.isFlagSet(STATUS_OVERFLOW)) registerPrintout[23] = 'V';
    if (cpu.isFlagSet(0b00100000)) registerPrintout[24] = 'U';
    if (cpu.isFlagSet(0b00010000)) registerPrintout[25] = 'B';
    if (cpu.isFlagSet(STATUS_DECIMAL)) registerPrintout[26] = 'D';
    if (cpu.isFlagSet(STATUS_INT_DISABLE)) registerPrintout[27] = 'I';
    if (cpu.isFlagSet(STATUS_ZERO)) registerPrintout[28] = 'Z';
    if (cpu.isFlagSet(STATUS_CARRY)) registerPrintout[29] = 'C';

    fprintf(logFile, "%-15s %-27s%s\n", hexFormAddress, instructionBuffer, registerPrintout);
}

void printInstruction(uint16 address, uint8 opcode, uint8 p1, uint8 p2)
{
    formatInstruction(address);
    printf("0x%04X %-30s", address, instructionBuffer);
}

void printInstruction(uint16 address)
{
    uint8 opcode = cpuBus.read(address);
    uint8 p1 = cpuBus.read(address + 1);
    uint8 p2 = cpuBus.read(address + 2);
    printInstruction(address, opcode, p1, p2);
}

static bool memViewRendered = false;
static int64 frameElapsed = 1;
static int64 cpuFreq = 1;
static int64 cpuGap = 0;
static int64 instCount = 1;

static bool traceEnabled = false;

void drawFps()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 100, 3 };
    SetConsoleCursorPosition(console, cursorPos);

    int32 msPerFrame = (int32)((1000 * frameElapsed) / cpuFreq);
    if (msPerFrame)
    {
        int32 framesPerSecond = 1000 / msPerFrame;
        int64 timePerInstruction = 0;
        if (instCount)
        {
            timePerInstruction = cpuGap / instCount;
        }

        printf("MS Per Frame = %08d FPS: %d IPS: %lld", msPerFrame, framesPerSecond, timePerInstruction);
    }
}

bool asciiMode = false;

void renderMemoryView()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = {};
    SetConsoleCursorPosition(console, cursorPos);

    printf("CPU (M)emory\n");
    printf("      ");
    for (int i = 0; i < 16; ++i)
    {
        printf(" %02X", i);
    }

    cursorPos.Y = 2;
    SetConsoleCursorPosition(console, cursorPos);

    const int ROWS_TO_DISPLAY = 32;

    uint16 address = ((uint16)debugMemPage) << 8;
    while (cursorPos.Y < ROWS_TO_DISPLAY + 2)
    {
        //TODO: do proper console manipulation to put these where I want it
        printf("0x%04X", address);

        for (int i = 0; i < 16; ++i)
        {
            uint8 d = cpuBus.read(address + i);
            if (!asciiMode || !isprint(d))
            {
                printf(" %02X", d);
            }
            else
            {
                printf("  %c", (char)d);
            }
        }

        ++cursorPos.Y;
        address += 16;
        SetConsoleCursorPosition(console, cursorPos);
    }
}

void debugView()
{
    if (!memViewRendered)
    {
        renderMemoryView();
        memViewRendered = true;
    }

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 56, 0 };
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

    drawFps();
}

void readValue()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = {};
    cursorPos.X = 0;
    cursorPos.Y = 35;
    SetConsoleCursorPosition(console, cursorPos);
    printf("               ");
    SetConsoleCursorPosition(console, cursorPos);

    printf("Address: ");
    showCursor();
    uint16 address = 0;
    scanf("%hx", &address);
    hideCursor();

    SetConsoleCursorPosition(console, cursorPos);
    printf("               ");
    SetConsoleCursorPosition(console, cursorPos);
    printf("Result: %02X   ", cpuBus.read(address));
}

#define NES_SCREEN_HEIGHT 240
#define NES_SCREEN_WIDTH 256

static uint32 ppuCycle = 0;

static uint8 nametableByte;
static uint8 attributetableByte;
static uint8 patternLo;
static uint8 patternHi;

static CHAR_INFO palette[0x4F] = {
    { '#', 0 },
    { '#', 0x10 },
    { '#', 0x20 },
    { '#', 0x30 },
    { '#', 0x40 },
    { '#', 0x50 },
    { '#', 0x60 },
    { '#', 0x70 },
    { '#', 0x80 },
    { '#', 0x90 },
    { '#', 0xA0 },
    { '#', 0xB0 },
    { '#', 0xD0 },
    { '#', 0xE0 },
    { '#', 0xF0 },
    { ' ', 0 },
    { ' ', 0x10 },
    { ' ', 0x20 },
    { ' ', 0x30 },
    { ' ', 0x40 },
    { ' ', 0x50 },
    { ' ', 0x60 },
    { ' ', 0x70 },
    { ' ', 0x80 },
    { ' ', 0x90 },
    { ' ', 0xA0 },
    { ' ', 0xB0 },
    { ' ', 0xD0 },
    { ' ', 0xE0 },
    { ' ', 0xF0 },
    { '.', FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED },
    { '.', FOREGROUND_GREEN | FOREGROUND_RED },
    { '.', FOREGROUND_RED },
    { '.', FOREGROUND_BLUE | FOREGROUND_RED },
    { '.', FOREGROUND_BLUE },
    { '.', FOREGROUND_GREEN },
    { '.', BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED },
    { '.', BACKGROUND_GREEN | BACKGROUND_RED },
    { '.', BACKGROUND_RED },
    { '.', BACKGROUND_BLUE | BACKGROUND_RED },
    { '.', BACKGROUND_BLUE },
    { '.', BACKGROUND_GREEN },
    { '.', BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_RED },
    { '.', BACKGROUND_RED | FOREGROUND_BLUE },
    { '.', BACKGROUND_BLUE | BACKGROUND_RED },
    { '.', BACKGROUND_GREEN | BACKGROUND_RED },
    { '#', 0xBF },
    { '#', 0x0F },
    { '#', 0x1F },
    { '#', 0x2F },
    { '#', 0x3F },
    { '#', 0x4F },
    { '#', 0x5F },
    { '#', 0x6F },
    { '#', 0x7F },
    { '#', 0x8F },
    { '#', 0x9F },
    { '#', 0xAF },
    { '#', 0xDF },
    { '#', 0xEF },
    { '#', 0xFF },
};

static CHAR_INFO pixels[NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];

void drawRect(int paletteIndex, int x, int y, int width, int height)
{
    for (int py = y; py < NES_SCREEN_HEIGHT && py < y + height; ++py)
    {
        for (int px = x; px < NES_SCREEN_WIDTH && px < x + width; ++px)
        {
            pixels[NES_SCREEN_WIDTH * py + px] = palette[paletteIndex];
        }
    }
}

void gameView()
{
    drawRect(5, 0, 0, 16, 16);
    drawRect(10, 100, 100, 25, 25);

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT location = {0, 0, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT };
    WriteConsoleOutputA(console, pixels, { NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT }, { 0, 0 }, &location);
}

void activateRenderMode()
{
    setConsoleSize(NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 4, 4);
    renderMode = true;
}

void activateDebugMode()
{
    setConsoleSize(160, 40, 8, 16);
    renderMode = false;
}

bool singleStepMode = true;

int main(int argc, char *argv[])
{
    activateDebugMode();
    hideCursor();

    cpu.connect(&cpuBus);
    cpuBus.connect(&ppu, &apu, &cartridge);
    cpuBus.addWriteCallback(renderMemCell);
    cartridge.load("data/01-basics.nes");

    cpu.reset();

    debugView();

    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    cpuFreq = counterFrequency.QuadPart;

    LARGE_INTEGER lastCounter;
    QueryPerformanceCounter(&lastCounter);
    int64 refreshRate = cpuFreq / 30;
    LARGE_INTEGER lastRender = lastCounter;

    bool stepRequested = false;
    while (running)
    {
        // TODO: kbhit is eating the cpu, only process input once per frame
        if (_kbhit())
        {
            char input = _getch();
            if (input == 'q')
            {
                running = false;
            }

            if (input == 'd')
            {
                debugMemPage += 2;
                memViewRendered = false;
            }

            if (input == 'a')
            {
                debugMemPage -= 2;
                memViewRendered = false;
            }

            if (input == 'c')
            {
                asciiMode = !asciiMode;
            }

            if (input == 'p')
            {
                singleStepMode = !singleStepMode;
            }

            stepRequested = input == ' ';

            if (input == 'r')
            {
                if (renderMode)
                {
                    activateDebugMode();
                }
                else
                {
                    activateRenderMode();
                }
            }
        }
        
        // TODO: Process the required number of cycles in a loop rather than once per overall loop
        if (!singleStepMode || stepRequested)
        {
            if (traceEnabled && !cpu.hasHalted())
            {
                logInstruction(cpu.instAddr);
            }

            bool instructionExecuted = cpu.tick();
            ++instCount;
            if (instructionExecuted && cpu.hasHalted())
            {
                fflush(logFile);
                singleStepMode = true;
            }

            stepRequested = false;
        }

        // TODO: This timing is Garbage, but all I need at the moment is to slow the renderer so the sim can run full force.
        LARGE_INTEGER preRenderCounter;
        QueryPerformanceCounter(&preRenderCounter);
        int64 frameGap = preRenderCounter.QuadPart - lastRender.QuadPart;

        if (frameGap > refreshRate)
        {
            if (renderMode)
            {
                gameView();
            }
            else
            {
                debugView();
            }

            instCount = 0;
            cpuGap = 0;
            frameElapsed = frameGap;
            lastRender = preRenderCounter;
        }

        // TODO: Sleep on remaining frame time
    }

    return 0;
}