#include <windows.h>
#include <stdint.h>
#include <math.h>

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static HBRUSH BlackBrush;
static float LineWidth;

typedef struct __vec2
{
    int X;
    int Y;
} Vec2;

typedef struct __colorTriple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
} ColorTriple;


void SetPixelInBuffer(Vec2 *Coords, ColorTriple *Colors)
{
    if (!BitmapMemory) return;
    uint8_t *Pixel = (uint8_t *)((char *)BitmapMemory + (Coords->Y * BitmapWidth * 4) + (Coords->X * 4));

    *Pixel = Colors->Blue;
    Pixel++;
    *Pixel = Colors->Green;
    Pixel++;
    *Pixel = Colors->Red;
}

/// TODO!! Restore the actual width capabilities (e.g., reducing brightness incrementally further away pixel is from center of line)
void DrawLineWidth(Vec2 *Point1, Vec2 *Point2, ColorTriple *Color)
{ 
    int x0 = Point1->X;
    int x1 = Point2->X;
    int y0 = Point1->Y;
    int y1 = Point2->Y;
    float wd = LineWidth;
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1; 
    int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1; 
    int err = dx-dy, e2, x2, y2;  
    float ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);


    for (wd = (wd+1)/2; ; )
    {                                   
        Vec2 Coords;
        Coords.X = x0;
        Coords.Y = y0;
        SetPixelInBuffer(&Coords, Color);
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                Vec2 Coords2;
                Coords2.X = x0;
                Coords2.Y = y2 += sy;
                
                SetPixelInBuffer(&Coords2, Color);
            }            
            if (x0 == x1) break;
            e2 = err; err -= dy; x0 += sx; 
        } 
        if (2*e2 <= dy)
        {
            for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
            {
                Vec2 Coords2;
                Coords2.X = x2 += sx;
                Coords2.Y = y0;
                SetPixelInBuffer(&Coords2, Color);
            }
            if (y0 == y1) break;
            err += dx; y0 += sy; 
        }
    }
}

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

void ClearScreen(HDC DC, RECT *WinRect)
{
    FillRect(DC, WinRect, BlackBrush);
}

void DrawQuad(Vec2 *TL, Vec2 *BR, ColorTriple *Color)
{
    if (!BitmapMemory) return;
    
	/*  p0----p1   /
    /*  |     |    /
    /*  |     |    /
    /*  p2----p3  */

    Vec2 P0, P1, P2, P3;
    P0.X = TL->X, P0.Y = TL->Y;
    P1.X = BR->X+100, P1.Y = TL->Y;
    P2.X = TL->X-50, P2.Y = BR->Y+200;
    P3.X = BR->X+75, P3.Y = BR->Y+250;
    DrawLineWidth(&P0, &P1, Color);
    DrawLineWidth(&P1, &P3, Color);
    DrawLineWidth(&P3, &P2, Color);
    DrawLineWidth(&P2, &P0, Color);

    /*int Pitch = BitmapWidth * 4;
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
    }*/
}

void UpdateGameWindow(HDC WindowDC, RECT *WindowRect, int X, int Y, int Width, int Height)
{   
    ClearScreen(WindowDC, WindowRect);
    //DrawBitmapData(Width, Height);
    Vec2 TopLeft, BotRight;
    TopLeft.X = 300;
    TopLeft.Y = 300;
    BotRight.X = 500;
    BotRight.Y = 200;
    ColorTriple Colors;
    Colors.Red = 255;
    Colors.Blue = 0;
    Colors.Green = 0;
    DrawQuad(&TopLeft, &BotRight, &Colors);
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
            UpdateGameWindow(PaintDC, &ClientRect, X, Y, Width, Height);
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
            BlackBrush = CreateSolidBrush(RGB(0,0,0));
            LineWidth = 3.0f;
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