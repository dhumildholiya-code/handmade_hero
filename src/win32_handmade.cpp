#include <windows.h>
#include <cstdint>

#define internal static
#define local_persist static
#define global_var static

global_var bool Running;
global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var int BitmapWidth;
global_var int BitmapHeight;
global_var int BytesPerPixel = 4;

internal void RenderGradient(int xOffset, int yOffset)
{
  int pitch = BitmapWidth * BytesPerPixel;
  uint8_t *row = (uint8_t *)BitmapMemory;
  for (int y = 0; y < BitmapHeight; ++y)
  {
    uint32_t *pixel = (uint32_t *)row;
    for (int x = 0; x < BitmapWidth; ++x)
    {
      uint8_t blue = x + xOffset;
      uint8_t green = y + yOffset;
      *pixel++ = (green << 8 | blue);
    }

    row += pitch;
  }
}

internal void Win32ResizeDIBSection(int width, int height)
{
  if (BitmapMemory)
  {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = width;
  BitmapHeight = height;

  BitmapInfo.bmiHeader = {0};
  BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int bitmapSize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
  BitmapMemory = VirtualAlloc(0, bitmapSize, MEM_COMMIT, PAGE_READWRITE);

  // TODO : Clear this to black
}

internal void Win32UpdateWindow(HDC deviceCtx, RECT *clientRect, int x, int y, int width,
                                int height)
{
  int windowWidth = clientRect->right - clientRect->left;
  int windowHeight = clientRect->bottom - clientRect->top;
  StretchDIBits(deviceCtx,
                /*                x, y, width, height,
                                x, y, width, height,
                */
                0, 0, BitmapWidth, BitmapHeight,
                0, 0, windowWidth, windowHeight,
                BitmapMemory, &BitmapInfo,
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
    Win32ResizeDIBSection(width, height);
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
    Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
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
  windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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

        RenderGradient(xOffset, yOffset);
        HDC deviceContext = GetDC(window);
        RECT clientRect;
        GetClientRect(window, &clientRect);
        Win32UpdateWindow(deviceContext, &clientRect, 0, 0, 0, 0);
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
