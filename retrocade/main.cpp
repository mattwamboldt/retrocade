#include <windows.h>

bool running = true;

LRESULT WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
            running = false;
    }

    return DefWindowProcA(window, msg, wParam, lParam);
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    windowClass.lpszClassName = "RetrocadeWndClass";

    if (!RegisterClassA(&windowClass))
    {
        return 0;
    }

    HWND window = CreateWindowExA(
        WS_EX_OVERLAPPEDWINDOW,
        "RetrocadeWndClass",
        "RetrocadeWindow",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, instance, 0
    );

    if (!window)
    {
        return 0;
    }

    ShowWindow(window, showCmd);
    UpdateWindow(window);

    while (running)
    {
        MSG msg;
        while (GetMessageA(&msg, window, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return 0;
}
