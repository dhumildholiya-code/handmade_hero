#include <windows.h>
#include <cstdint>

#define internal static
#define local_persist static
#define global_var static

struct Win32OffscreenBuffer
{
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
  int bytesPerPixel;
};

global_var bool Running;
global_var Win32OffscreenBuffer GlobalBackBuffer;

internal void RenderGradient(Win32OffscreenBuffer buffer, int xOffset, int yOffset)
{
  uint8_t *row = (uint8_t *)buffer.memory;
  for (int y = 0; y < buffer.height; ++y)
  {
    uint32_t *pixel = (uint32_t *)row;
    for (int x = 0; x < buffer.width; ++x)
    {
      uint8_t blue = x + xOffset;
      uint8_t green = y + yOffset;
      *pixel++ = (green << 8 | blue);
    }

    row += buffer.pitch;
  }
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height)
{
  if (buffer->memory)
  {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;
  buffer->bytesPerPixel = 4;

  buffer->info.bmiHeader = {0};
  buffer->info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = -buffer->height;
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmapSize = (buffer->width * buffer->height) * buffer->bytesPerPixel;
  buffer->memory = VirtualAlloc(0, bitmapSize, MEM_COMMIT, PAGE_READWRITE);
  buffer->pitch = buffer->width * buffer->bytesPerPixel;

  // TODO : Clear this to black
}

internal void Win32BufferToWindow(HDC deviceCtx, RECT clientRect,
                                  Win32OffscreenBuffer buffer,
                                  int x, int y, int width, int height)
{
  int windowWidth = clientRect.right - clientRect.left;
  int windowHeight = clientRect.bottom - clientRect.top;
  StretchDIBits(deviceCtx,
                0, 0, buffer.width, buffer.height,
                0, 0, windowWidth, windowHeight,
                buffer.memory, &buffer.info,
                DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT Win32WindowProc(HWND window, UINT message, WPARAM wParam,
                                 LPARAM lParam)
{
  LRESULT result = 0;
  switch (message)
  {
  case WM_SIZE:
  {
    RECT clientRect;
    GetClientRect(window, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    Win32ResizeDIBSection(&GlobalBackBuffer, width, height);
  }
  break;
  case WM_DESTROY:
  {
    Running = false;
  }
  break;
  case WM_CLOSE:
  {
    Running = false;
  }
  break;
  case WM_PAINT:
  {
    PAINTSTRUCT paint;
    HDC deviceContext = BeginPaint(window, &paint);
    int x = paint.rcPaint.left;
    int y = paint.rcPaint.top;
    int width = paint.rcPaint.right - x;
    int height = paint.rcPaint.bottom - y;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    Win32BufferToWindow(deviceContext, clientRect, GlobalBackBuffer, x, y, width, height);
    EndPaint(window, &paint);
  }
  break;
  default:
    result = DefWindowProc(window, message, wParam, lParam);
    break;
  }
  return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine,
                   int showCode)
{

  // Register Window class
  WNDCLASS windowClass = {};
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = Win32WindowProc;
  windowClass.hInstance = instance;
  windowClass.lpszClassName = "HandmadeWindowClass";

  // Create the window
  if (RegisterClass(&windowClass))
  {
    HWND window = CreateWindowEx(0, windowClass.lpszClassName, "Handmade Hero",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 0, 0, instance, 0);
    if (window)
    {
      Running = true;

      int xOffset = 0;
      int yOffset = 0;
      while (Running)
      {
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
          if (msg.message == WM_QUIT)
          {
            Running = false;
          }
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }

        RenderGradient(GlobalBackBuffer, xOffset, yOffset);
        HDC deviceContext = GetDC(window);
        RECT clientRect;
        GetClientRect(window, &clientRect);
        Win32BufferToWindow(deviceContext, clientRect, GlobalBackBuffer, 0, 0, 0, 0);
        ReleaseDC(window, deviceContext);
        ++xOffset;
      }
    }
    else
    {
      // TODO : Handle error
    }
  }
  else
  {
    // TODO : Handle error
  }

  return 0;
}
