#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include <nescore/nes.h>
#include <nescore/cpuTrace.h>

// General TODO:
// - naive printfs and sprintfs are slow as mollasses for this kind of thing
// paging the memview always causes a frame miss, even at 30fps, so that code needs attention

// Start on the ppu, may help with the other problems which means testing
// rendering mode at speed which will have its own console issues.
// May drive need to port/pull into retrocade proper, but gonna try
// to console as long as I can, easier to think about/iterate on

static bool isRunning;

// globals are bad!!! change this later!!!
static MOS6502 cpu = {};
static PPU ppu = {};
static PPUBus ppuBus = {};
static APU apu = {};
static CPUBus cpuBus = {};
static Cartridge cartridge = {};

static bool renderMode = false;
static bool traceEnabled = true;
static bool asciiMode = false;
static bool singleStepMode = false;

static uint8 debugMemPage = 0x60;
static bool memViewRendered = false;

static LARGE_INTEGER frameTime;
real32 frameElapsed;
static int64 cpuFreq = 1;
static int64 cpuGap = 0;
static int64 instCount = 1;

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
    COORD cursorPos = { 8, 3 };
    cursorPos.X += x * 3;
    cursorPos.Y += y;
    SetConsoleCursorPosition(console, cursorPos);
    if (!asciiMode || !isprint(val))
    {
        printf(" %02X", val);
    }
    else
    {
        printf("  %c", (char)val);
    }
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

void printInstruction(uint16 address)
{
    uint8 opcode = cpuBus.read(address);
    uint8 p1 = cpuBus.read(address + 1);
    uint8 p2 = cpuBus.read(address + 2);
    
    char instructionBuffer[32];
    formatInstruction(instructionBuffer, address, &cpu, &cpuBus);
    printf("0x%04X %-30s", address, instructionBuffer);
}

void drawFps()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 58, 9 };
    SetConsoleCursorPosition(console, cursorPos);

    real32 msPerFrame = 1000.0f * frameElapsed;
    if (msPerFrame)
    {
        real32 framesPerSecond = 1000.0f / msPerFrame;
        printf("MS Per Frame = %08.2f FPS: %.2f IPS: %-8lld", msPerFrame, framesPerSecond, instCount);
    }
}

void renderMemoryView()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 2, 1 };
    SetConsoleCursorPosition(console, cursorPos);

    printf("CPU (M)emory");

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);

    printf("      ");
    for (int i = 0; i < 16; ++i)
    {
        printf(" %02X", i);
    }

    cursorPos.Y = 3;
    SetConsoleCursorPosition(console, cursorPos);

    const int ROWS_TO_DISPLAY = 32;

    uint16 address = ((uint16)debugMemPage) << 8;
    while (cursorPos.Y < ROWS_TO_DISPLAY + 3)
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
}

void readValue()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = {};
    cursorPos.X = 2;
    cursorPos.Y = 31;
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

void gameView()
{
    
}

void activateRenderMode()
{
    SetConsoleTitleA("ConNessa: Render Debug");
    renderMode = true;
}

void activateDebugMode()
{
    SetConsoleTitleA("ConNessa: Debug Mode");
    renderMode = false;
}

LARGE_INTEGER getClockTime()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter;
}

real32 getSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    int64 elapsed = end.QuadPart - start.QuadPart;
    return (real32)elapsed / (real32)cpuFreq;
}

void singleStep()
{
    // Do a single cpu and apu step, 3 ppu steps;
    if (traceEnabled && !cpu.hasHalted() && !cpu.isExecuting())
    {
        logInstruction("data/6502.log", cpu.instAddr, &cpu, &cpuBus);
    }

    bool instructionExecuted = cpu.tick();
    if (instructionExecuted)
    {
        ++instCount;
        if (cpu.hasHalted())
        {
            flushLog();
            singleStepMode = true;
        }
    }
}

bool wasVBlankActive = false;

void update(real32 secondsPerFrame)
{
    if (cpu.hasHalted())
    {
        return;
    }

    // TODO: calculate this rate properly and preserve extra cycles in a variable to run later
    // cpu and ppu per needed timings, cpu and apu every 12, ppu every 4

    // TODO: There s a note on the wiki that mentions having a hard coded clock rate
    // "Emulator authors may wish to emulate the NTSC NES/Famicom CPU at 21441960 Hz ((341???262-0.5)???4???60) to ensure a synchronised/stable 60 frames per second."
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

int main(int argc, char *argv[])
{
    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    cpuFreq = counterFrequency.QuadPart;

    UINT desiredSchedulerMS = 1;
    bool useSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;

    activateDebugMode();
    hideCursor();
    setConsoleSize(110, 40, 8, 16);

    cpu.connect(&cpuBus);
    ppu.connect(&ppuBus);
    cpuBus.connect(&ppu, &apu, &cartridge);
    cpuBus.addWriteCallback(renderMemCell);
    ppuBus.connect(&ppu, &cartridge);
    cartridge.load("data/all_instrs.nes");
    cpu.reset();

    real32 framesPerSecond = 30.0f;
    real32 secondsPerFrame = 1.0f / framesPerSecond;

    frameTime = getClockTime();
    uint32 frameCount = 0;
    isRunning = true;

    bool stepRequested = false;
    while (isRunning)
    {
        // Input handling
        // TODO: joypad inputs, may have to switch input processing methods
        if (_kbhit())
        {
            char input = _getch();
            if (input == 'q')
            {
                isRunning = false;
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

            if (input == '`')
            {
                cpu.reset();
            }
        }

        if (singleStepMode)
        {
            if (stepRequested)
            {
                singleStep();
                while (cpu.isExecuting())
                {
                    cpu.tick();
                }

                stepRequested = false;
            }
        }
        else
        {
            update(secondsPerFrame);
        }

        if (renderMode)
        {
            gameView();
        }
        else
        {
            debugView();
        }

        frameElapsed = getSecondsElapsed(frameTime, getClockTime());
        if (frameElapsed < secondsPerFrame)
        {
            if (useSleep)
            {
                DWORD sleepMs = (DWORD)(1000.0f * (secondsPerFrame - frameElapsed));
                if (sleepMs > 0)
                {
                    Sleep(sleepMs);
                }
            }

            while (frameElapsed < secondsPerFrame)
            {
                frameElapsed = getSecondsElapsed(frameTime, getClockTime());
            }
        }
        else
        {
            OutputDebugStringA("FrameMissed\n");
        }

        if (!renderMode)
        {
            drawFps();
            instCount = 0;
        }

        frameTime = getClockTime();
        ++frameCount;
    }

    return 0;
}