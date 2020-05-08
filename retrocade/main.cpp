#include <windows.h>

bool running = true;

//NOtes from research:
// nes file format
// http://wiki.nesdev.com/w/index.php/INES

// System memory
// CPU 2KB, memory mapped 4x
// PPU 8 registers repeated many times
// APU and IO registers, 24 bytes
// bytes for APU and IO test functionality

// 6502 has 16 byte bus, hence the "mappers" depending on hardware on the cartridge certain address' may go off device onto extra cart memory

// set up a main clock to be stepped manually and validate our stepping is working correctly
// set up cpu update clock
// set up ppu update clock

// cpu - 0000 - 07ff
// APU - 4000 - 4017
// cartridge 4020 - ffff

// ppu separate 16kb address space
// VRAM 2k
// pallettes 3f00 3fff
// oam 256 bytes on ppu addressed by cpu through mapped registers

// cpu dma to oam that stalls

// implement cpu. manually run certain code to make sure it's updating the mapped memory correctly

// Each device has an address space available to it and the cartridge determines how that address space is mapped to physical chips, this allows the cart to expand the capabilities based on the manufacturer


/*
1 - Try to write a manually stepped 6502 emulator connected to a flat 64k ram module.
since it is kind of standalone and has the most documentation. was used in several machines so lots to pull from
may be better to do this in a console app with simpler debugging measures than a windows one and then pull it in after
http://wiki.nesdev.com/w/index.php/CPU_ALL

2 - Try loading and running some of the cpu test roms
http://wiki.nesdev.com/w/index.php/Emulator_tests

3 - Try getting the ppu to work and run the relevant tests
http://wiki.nesdev.com/w/index.php/PPU_programmer_reference

4 - Hook up the master clock
5 - Connect the ppu to an actual display using gdi to blit a backbuffer filled by the GPU
6 - Add the APU functions to the CPU emulator (play back using directsound, since I've used it before)
7 - Add a basic mapper that follows the standard layout
8 - Add keyboard input
9 - Try more test roms and clean up anything remaining
10 - Test out some commercial games that I have
11 - Start to add an interface, record modes, save states, (etc/whatever strikes my fancy)
*/

LRESULT WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            FillRect(deviceContext, &paint.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(window, &paint);
        }
        break;

        case WM_DESTROY:
        case WM_CLOSE:
            running = false;
            PostMessageA(window, WM_QUIT, 0, 0);
            break;

        default:
            return DefWindowProcA(window, msg, wParam, lParam);
    }

    return 0;
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    windowClass.lpszClassName = "RetrocadeWndClass";

    if (!RegisterClassA(&windowClass))
    {
        return 0;
    }

    HWND window = CreateWindowExA(
        0,
        "RetrocadeWndClass",
        "Retrocade",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, instance, 0
    );

    if (!window)
    {
        return 0;
    }

    while (running)
    {
        MSG msg;
        while (GetMessageA(&msg, 0, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return 0;
}
