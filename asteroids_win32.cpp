#include <windows.h>

static BITMAPINFO BitmapInfo;
static HBITMAP DibSection;
static void *BitmapMemory;
static HDC BitmapDeviceContext;

void ResizeDIBSection(int X, int Y, int Width, int Height)
{
    if (DibSection)
    {
        DeleteObject((HGDIOBJ)DibSection);
    }

    if (!BitmapDeviceContext)
    {
        BitmapDeviceContext = CreateCompatibleDC(0);
    }

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    DibSection = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);    

}

void UpdateGameWindow(HDC WindowDC, int X, int Y, int Width, int Height)
{
    StretchDIBits(WindowDC, X, Y, Width, Height, X, Y, Width, Height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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

    switch (Message)
    {
        // To do: handle all painting, resize, etc.
        case WM_ACTIVATEAPP:
        {
            return 0;            
        }
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
    WNDCLASS WindowClass = {}; // Ensure the struct is zeroed.
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