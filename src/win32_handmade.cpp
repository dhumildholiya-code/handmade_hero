#include <windows.h>

#define internal static
#define local_persist static
#define global_var static

global_var bool Running;
global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var HBITMAP BitmapHandle;
global_var HDC BitmapDC;

internal void Win32ResizeDIBSection(int width, int height)
{
  if (BitmapHandle)
  {
    DeleteObject(BitmapHandle);
  }
  if (!BitmapDC)
  {
    BitmapDC = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader = {0};
  BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BitmapInfo.bmiHeader.biWidth = width;
  BitmapInfo.bmiHeader.biHeight = height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle = CreateDIBSection(BitmapDC, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

internal void Win32UpdateWindow(HDC deviceCtx, int x, int y, int width,
                                int height)
{
  StretchDIBits(deviceCtx,
                x, y, width, height,
                x, y, width, height,
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
    Win32UpdateWindow(deviceContext, x, y, width, height);
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
      while (Running)
      {
        MSG message;
        BOOL messageResult = GetMessage(&message, 0, 0, 0);
        if (messageResult > 0)
        {
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        else
        {
          break;
        }
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
