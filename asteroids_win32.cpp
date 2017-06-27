#include <windows.h>
#include <stdint.h>

typedef struct tagBitmapData {
  int Width;
  int Height;
  int BytesPerPixel;
  int BitmapMemorySize;
  BITMAPINFO BitmapInfo;
  void *BitmapMemory;
} Win32BitmapData;

static Win32BitmapData BitmapData;

void ResizeDIBSection(int X, int Y, int Width, int Height)
{
    if (BitmapData.BitmapMemory)
    {
        VirtualFree(BitmapData.BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapData.Width = Width;
    BitmapData.Height = Height;

    BitmapData.BitmapInfo.bmiHeader.biSize = sizeof(BitmapData.BitmapInfo.bmiHeader);
    BitmapData.BitmapInfo.bmiHeader.biWidth = BitmapData.Width;
    BitmapData.BitmapInfo.bmiHeader.biHeight = BitmapData.Height;
    BitmapData.BitmapInfo.bmiHeader.biPlanes = 1; // Magic number mandated by win32 documentation
    BitmapData.BitmapInfo.bmiHeader.biBitCount = BitmapData.BytesPerPixel * 8;
    BitmapData.BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BitmapData.BitmapMemorySize = BitmapData.Width * BitmapData.Height * BitmapData.BytesPerPixel;
    BitmapData.BitmapMemory = VirtualAlloc(0, BitmapData.BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void RenderBitmapData(int Width, int Height)
{
    if (!BitmapData.BitmapMemory) return;

    int Pitch = BitmapData.Width * 4; // 4 bytes per pixel although not sure I can guarantee that this will always be true..??
    uint8_t Red = 0, Green = 200, Blue = 100;
    uint8_t *Row = (uint8_t *)BitmapData.BitmapMemory;
    for (int Y = 0; Y < BitmapData.Height; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < BitmapData.Width; ++X)
        {
            //Blue
            *Pixel = Blue;
            ++Pixel;
            
            //Green
            *Pixel = Green;
            ++Pixel;
            
            //Red
            *Pixel = Red;
            Red = (Red + 20);
            ++Pixel;

            *Pixel = (uint8_t)0;
            ++Pixel;
        }
        Row += Pitch;
    }    
}

void UpdateGameWindow(HDC WindowDC, int X, int Y, int Width, int Height)
{   
    RenderBitmapData(Width, Height);
    StretchDIBits(WindowDC, X, Y, Width, Height, X, Y, Width, Height, BitmapData.BitmapMemory, &(BitmapData.BitmapInfo), DIB_RGB_COLORS, SRCCOPY);
}

/* The main callback for our window. This function will handle all
   messages passed from Windows. The default case must be maintained
   to ensure that messages not explicitly handled are handled by Windows. */
LRESULT CALLBACK AsteroidsWindowCallback(HWND WindHandle,
                                         UINT Message,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
    LRESULT Result = 0;

    if (!BitmapData.BitmapMemory)
    {
        BitmapData = {};
    }

    switch (Message)
    {
        // To do: handle all painting, resize, etc.
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(WindHandle, &ClientRect);
            int X = ClientRect.left;
            int Y = ClientRect.right;
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top; // Negative to ensure top-down DIB
            ResizeDIBSection(X, Y, Width, Height);
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            RECT ClientRect;
            GetClientRect(WindHandle, &ClientRect);
            int X = ClientRect.left;
            int Y = ClientRect.bottom;
            int Width = ClientRect.right - X;
            int Height = ClientRect.top - Y;
            HDC PaintDC = BeginPaint(WindHandle, &PaintStruct);
            UpdateGameWindow(PaintDC, X, Y, Width, Height);
            EndPaint(WindHandle, &PaintStruct);
            return 0;
        }
        case WM_CLOSE: case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_UP:
                {
                    OutputDebugString("Up arrow");
                    return 0;
                }
                default:
                {
                    OutputDebugString("Other key");
                    return 0;
                }
            }
        }
        default:
        {
            Result = DefWindowProc(WindHandle, Message, wParam, lParam);
        }
    }

    return Result;
}

/* The graphical entry point for Windows. */
int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CommandLine,
                     int CommandShow)
{
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = &AsteroidsWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "AsteroidsWindow";

    // RegisterClass returns an ATOM, which we likely will not need to store.
    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(0, "AsteroidsWindow", "Asteroids", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (WindowHandle)
        {
            for (;;)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
            }
        }
    }   

    return 0;
}