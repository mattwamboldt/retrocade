#include <windows.h>

typedef unsigned char uint8;
typedef unsigned int uint32;

static bool running = true;
static BITMAPINFO bitmapInfo = {};
static void* backBuffer;
static int backBufferWidth;
static int backBufferHeight;

void render(int xOffset, int yOffset)
{
    int pitch = backBufferWidth * 4;
    uint8* row = (uint8*)backBuffer;
    for (int y = 0; y < backBufferHeight; ++y)
    {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < backBufferWidth; ++x)
        {
            uint8 b = x + xOffset;
            uint8 g = y + yOffset;
            *pixel++ = (g << 8) | b;
        }

        row += pitch;
    }
}

void resizeDIBSection(int width, int height)
{
    if (backBuffer)
    {
        VirtualFree(backBuffer, 0, MEM_RELEASE);
    }

    backBufferWidth = width;
    backBufferHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;

    backBuffer = VirtualAlloc(0, 4 * width * height, MEM_COMMIT, PAGE_READWRITE);
}

void paintToWindow(HDC deviceContext, RECT* windowRect, int x, int y, int width, int height)
{
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;
    StretchDIBits(deviceContext,
        0, 0, backBufferWidth, backBufferHeight,
        0, 0, windowWidth, windowHeight,
        backBuffer, &bitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(window, &clientRect);
            resizeDIBSection(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            RECT clientRect;
            GetClientRect(window, &clientRect);
            paintToWindow(deviceContext, &clientRect, paint.rcPaint.left, paint.rcPaint.right, width, height);
            EndPaint(window, &paint);
        }
        break;

        case WM_DESTROY:
        case WM_CLOSE:
            running = false;
            break;

        default:
            return DefWindowProcA(window, msg, wParam, lParam);
    }

    return 0;
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
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

    int xOffset = 0;
    int yOffset = 0;
    
    while (running)
    {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        render(xOffset, yOffset);

        HDC deviceContext = GetDC(window);
        RECT clientRect;
        GetClientRect(window, &clientRect);
        paintToWindow(deviceContext, &clientRect, 0, 0, backBufferWidth, backBufferHeight);
        ReleaseDC(window, deviceContext);

        ++xOffset;
        ++yOffset;
    }

    return 0;
}
