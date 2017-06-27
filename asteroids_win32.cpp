#include <windows.h>
#include <stdint.h>

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;

typedef struct __vec2
{
    int X;
    int Y;
} Vec2;

void ResizeDIBSection(int X, int Y, int Width, int Height)
{

    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4; // 4 bytes per pixel to keep dword aligned
    int BitmapMemorySize = BitmapWidth * BitmapHeight * BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void ClearScreen(int ScreenWidth, int ScreenHeight)
{
    int Pitch = BitmapWidth * 4;
    uint8_t Red = 0, Green = 0, Blue = 0;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {
            //Blue
            *Pixel = Blue;
            ++Pixel;
            
            //Green
            *Pixel = Green;
            ++Pixel;
            
            //Red
            *Pixel = Red;
            ++Pixel;

            *Pixel = (uint8_t)0;
            ++Pixel;
        }
        Row += Pitch;
    }
}

void DrawBitmapData(int Width, int Height)
{
    if (!BitmapMemory) return;

    int Pitch = BitmapWidth * 4;
    uint8_t Red = 0, Green = 0, Blue = 0;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {
            //Blue
            *Pixel = Blue;
            ++Pixel;
            
            //Green
            *Pixel = Green;
            ++Pixel;
            
            //Red
            *Pixel = Red;
            ++Pixel;

            *Pixel = (uint8_t)0;
            ++Pixel;
        }
        Row += Pitch;
    }    
}

void DrawRectangle(Vec2 *TL, Vec2 *BR)
{
    if (!BitmapMemory) return;

    int Pitch = BitmapWidth * 4;
    uint8_t Red = 255, Green = 255, Blue = 255;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {
            if (((Y == TL->Y || Y == BR->Y) && X >= TL->X && X <= BR->X) || ((X == TL->X || X == BR->X) && Y <= TL->Y && Y >= BR->Y)) 
            {
                //Blue
                *Pixel = Blue;
                ++Pixel;
            
                //Green
                *Pixel = Green;
                ++Pixel;
            
                //Red
                *Pixel = Red;
                ++Pixel;

                *Pixel = (uint8_t)0;
                ++Pixel;
            }
			else
			{
				Pixel += 4;
			}
        }
        Row += Pitch;
    }
}

void UpdateGameWindow(HDC WindowDC, int X, int Y, int Width, int Height)
{   
    ClearScreen(Width, Height);
    //DrawBitmapData(Width, Height);
    Vec2 TopLeft, BotRight;
    TopLeft.X = 300;
    TopLeft.Y = 300;
    BotRight.X = 500;
    BotRight.Y = 200;
    DrawRectangle(&TopLeft, &BotRight);
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