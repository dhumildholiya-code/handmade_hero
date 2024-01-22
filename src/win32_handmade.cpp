#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_var static

struct Win32ScreenBuffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};
struct Win32WindowDim
{
    int width;
    int height;
};

global_var bool GlobalRunning;
global_var Win32ScreenBuffer GlobalBuffer;

internal Win32WindowDim Win32GetWindowDim(HWND window)
{
    Win32WindowDim dim;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    dim.width  = clientRect.right - clientRect.left;
    dim.height = clientRect.bottom - clientRect.top;
    return dim;
}
internal void RenderGradient(Win32ScreenBuffer buffer, int xOffset, int yOffset)
{
    uint8_t* row = (uint8_t*)buffer.memory;
    for(int y=0; y < buffer.height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for(int x=0; x < buffer.width; ++x)
        {
            uint8_t g = y + yOffset;
            uint8_t b = x + xOffset;
            *pixel++ = ((g << 8) | b);
        }
        row += buffer.pitch;
    }
}

// Create bitmap memory for us to write.
internal void Win32ResizeDIBSection(Win32ScreenBuffer* buffer, int width, int height) {
    if(buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    buffer->width = width;
    buffer->height = height;
    buffer->bytesPerPixel = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (buffer->width * buffer->height) * buffer->bytesPerPixel;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * buffer->bytesPerPixel;
}

internal void Win32UpdateWindow(HDC deviceContext,Win32ScreenBuffer buffer,
                                int windowWidth, int windowHeight) 
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory, &buffer.info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam,
                            LPARAM lParam) {
    LRESULT result;
    switch (message) {
        case WM_DESTROY: {
            GlobalRunning = false;
        } break;
        case WM_CLOSE: {
            GlobalRunning = false;
        } break;
        case WM_ACTIVATEAPP: {
        } break;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            Win32WindowDim dim = Win32GetWindowDim(window);
            Win32UpdateWindow(deviceContext, GlobalBuffer, dim.width, dim.height);
            EndPaint(window, &paint);
        }
        default: {
            result = DefWindowProc(window, message, wParam, lParam);
        }
    }
    return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine,
                   int showCode) {
    WNDCLASS windowClass = {};

    Win32ResizeDIBSection(&GlobalBuffer, 1280, 720);

    windowClass.style = CS_VREDRAW|CS_HREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeWindowClass";
    // Register WindowClass
    if (RegisterClass(&windowClass)) {
        // Create a Window
        HWND window = CreateWindowEx(
            0, windowClass.lpszClassName, "Handmade Game",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
        if (window) {
            GlobalRunning = true;
            int xOffset = 0;
            int yOffset = 0;
            while (GlobalRunning) {
                MSG message;
                while(PeekMessageA(&message,0,0,0,PM_REMOVE))
                {
                    if(message.message == WM_QUIT){GlobalRunning = false;}
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                RenderGradient(GlobalBuffer, xOffset, yOffset);
                HDC deviceContext = GetDC(window);
                Win32WindowDim dim = Win32GetWindowDim(window);
                Win32UpdateWindow(deviceContext,GlobalBuffer, 
                                  dim.width, dim.height);
                ReleaseDC(window, deviceContext);

                ++xOffset;
            }
        }
    } else {
    }
    return 0;
}
