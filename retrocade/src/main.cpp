#include <windows.h>
#include <xinput.h>

#include "common.h"
#include "core\nes.h"

// Dynamic Loading of XInput
typedef DWORD WINAPI XInputGetStateFn(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI XInputSetStateFn(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static XInputGetStateFn* pfnXInputGetState;
static XInputSetStateFn* pfnXInputSetState;
#define XInputGetState pfnXInputGetState
#define XInputSetState pfnXInputSetState

DWORD WINAPI XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetStateStub(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

void LoadXInput()
{
    HMODULE xInput = LoadLibraryA("xinput1_3.dll");
    if (!xInput)
    {
        xInput = LoadLibraryA("xinput1_4.dll");
    }

    if (!xInput)
    {
        xInput = LoadLibraryA("xinput9_1_0.dll");
    }

    if (xInput)
    {
        pfnXInputGetState = (XInputGetStateFn*)GetProcAddress(xInput, "XInputGetState");
        pfnXInputSetState = (XInputSetStateFn*)GetProcAddress(xInput, "XInputSetState");
    }
    else
    {
        pfnXInputGetState = &XInputGetStateStub;
        pfnXInputSetState = &XInputSetStateStub;
    }
}

struct GDIBackBuffer
{
    BITMAPINFO info = {};
    void* memory;
    int width;
    int height;
    int pitch;
};

struct WindowSize
{
    int width;
    int height;
};

WindowSize GetWindowSize(HWND window)
{
    RECT clientRect;
    GetClientRect(window, &clientRect);

    WindowSize result;
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

static bool isRunning = true;
static GDIBackBuffer globalBackBuffer = {};
static LARGE_INTEGER frameTime;
real32 frameElapsed;
static int64 cpuFreq = 1;

void render(GDIBackBuffer buffer, int xOffset, int yOffset)
{
    uint8* row = (uint8*)buffer.memory;
    for (int y = 0; y < buffer.height; ++y)
    {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer.width; ++x)
        {
            uint8 b = x + xOffset;
            uint8 g = y + yOffset;
            *pixel++ = (g << 8) | b;
        }

        row += buffer.pitch;
    }
}

void resizeDIBSection(GDIBackBuffer* buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;

    buffer->memory = VirtualAlloc(0, 4 * width * height, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * 4;
}

void paintToWindow(HDC deviceContext, GDIBackBuffer buffer, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer.width, buffer.height,
        buffer.memory, &buffer.info,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            WindowSize size = GetWindowSize(window);
            paintToWindow(deviceContext, globalBackBuffer, size.width, size.height);
            EndPaint(window, &paint);
        }
        break;

        case WM_DESTROY:
        case WM_CLOSE:
            isRunning = false;
            break;

        default:
            return DefWindowProcA(window, msg, wParam, lParam);
    }

    return 0;
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

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    LoadXInput();

    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    cpuFreq = counterFrequency.QuadPart;

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    windowClass.lpszClassName = "RetrocadeWndClass";
    windowClass.hCursor = LoadCursorA(instance, IDC_ARROW);

    UINT desiredSchedulerMS = 1;
    bool useSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;

    if (!RegisterClassA(&windowClass))
    {
        return 0;
    }

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
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

    resizeDIBSection(&globalBackBuffer, 1280, 720);

    NES nes;
    nes.loadRom("data/nestest.nes");

    real32 framesPerSecond = 30.0f;
    real32 secondsPerFrame = 1.0f / framesPerSecond;

    frameTime = getClockTime();
    uint32 frameCount = 0;

    int xOffset = 0;
    int yOffset = 0;
    
    while (isRunning)
    {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                isRunning = false;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        /*
        if (input == 'q')
        {
            isRunning = false;
        }

        if (input == 'd')
        {
            // debug.nextpage();
        }

        if (input == 'a')
        {
            // debug.prevpage();
        }

        if (input == 'c')
        {
            //debug.toggleAsciiMode();
        }

        if (input == 'p')
        {
            nes.toggleSingleStep();
        }

        if (input == ' ')
        {
            nes.singleStep();
        }

        if (input == '`')
        {
            nes.reset();
        }
        */

        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            XINPUT_STATE controllerState;
            if (XInputGetState(i, &controllerState) == ERROR_SUCCESS)
            {
                // connected
                // TODO: handle packet number
                XINPUT_GAMEPAD pad = controllerState.Gamepad;

                bool up = pad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
                bool down = pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                bool left = pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                bool right = pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                bool start = pad.wButtons & XINPUT_GAMEPAD_START;
                bool back = pad.wButtons & XINPUT_GAMEPAD_BACK;
                bool leftShoulder = pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                bool rightShoulder = pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                bool aButton = pad.wButtons & XINPUT_GAMEPAD_A;
                bool bButton = pad.wButtons & XINPUT_GAMEPAD_B;
                bool xButton = pad.wButtons & XINPUT_GAMEPAD_X;
                bool yButton = pad.wButtons & XINPUT_GAMEPAD_Y;
            }
            else
            {
                // not connected
            }
        }

        nes.update(secondsPerFrame);

        render(globalBackBuffer, xOffset, yOffset);

        HDC deviceContext = GetDC(window);
        WindowSize size = GetWindowSize(window);
        paintToWindow(deviceContext, globalBackBuffer, size.width, size.height);
        ReleaseDC(window, deviceContext);

        ++xOffset;
        ++yOffset;

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

        frameTime = getClockTime();
        ++frameCount;
    }

    return 0;
}
