#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>
#include <dsound.h>
#include <stdexcept>

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static HBRUSH BlackBrush;
static float LineWidth;
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#include "asteroids.cpp"

struct Vec2
{
    int X;
    int Y;
};

struct ColorTriple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

struct PlayerControlInput
{
    bool MoveUp;
    bool MoveDown;
    bool MoveRight;
    bool MoveLeft;
    bool A_Pressed;
    bool B_Pressed;
    bool Start_Pressed;
};

struct PlayerModel
{
    Vec2 Vertices[3];
    ColorTriple Color;
};

static PlayerModel Player;


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

void ResizeDIBSection(int Width, int Height)
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

void ClearBuffer()
{
	ZeroMemory(BitmapMemory, BitmapHeight * BitmapWidth * 4);
}

void DrawPolygon(Vec2 *CoordArray, int NumVertices, ColorTriple *Color)
{
    if (!BitmapMemory) return;
    if (NumVertices < 3) return;

    int cur = 0, next = 1;

    for (int i = 0; i < NumVertices-1; ++i)
    {
        DrawLineWidth(&CoordArray[cur], &CoordArray[next], Color);
        ++cur;
        if (i < NumVertices - 2) ++next;
    }
    DrawLineWidth(&CoordArray[cur], &CoordArray[0], Color);
}

void DrawToWindow(HDC WindowDC, RECT *WindowRect, int Width, int Height)
{   
    ClearBuffer();
    DrawPolygon(Player.Vertices, 3, &(Player.Color));
    StretchDIBits(WindowDC, 0, 0, Width, Height, 0, 0, Width, Height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

void RenderGame(HWND WindHandle, HDC WindowDC)
{
    RECT ClientRect;
    GetClientRect(WindHandle, &ClientRect);
    int Width = ClientRect.right - ClientRect.left, Height = ClientRect.bottom - ClientRect.top;
    DrawToWindow(WindowDC, &ClientRect, Width, Height);
}

XINPUT_GAMEPAD GetControllerInput(DWORD ControllerNumber)
{
    XINPUT_STATE ControllerState;
    if (XInputGetState(ControllerNumber, &ControllerState) == ERROR_SUCCESS)
    {
        return ControllerState.Gamepad;
    }
    else
    {
        throw std::runtime_error("Controller not connected");
    }
}

void UpdateGameState(XINPUT_GAMEPAD *Controller)
{
    PlayerControlInput PlayerInput;
    PlayerInput.MoveUp = Controller->sThumbLY > 10000 ? true : false;
    PlayerInput.MoveDown = Controller->sThumbLY < -10000 ? true : false;
    PlayerInput.MoveRight = Controller->sThumbLX > 10000 ? true : false;
    PlayerInput.MoveLeft = Controller->sThumbLX < -10000 ? true : false;

    PlayerInput.A_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_A);
    PlayerInput.B_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_B);
    PlayerInput.Start_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_START);

    if (PlayerInput.Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput.A_Pressed)
    {
        Player.Color.Red = 200, Player.Color.Blue = 0;
    }
    if (PlayerInput.B_Pressed)
    {
        Player.Color.Red = 0, Player.Color.Blue = 200;
    }
    if (PlayerInput.MoveUp)
    {
        Player.Vertices[0].Y += 1;
        Player.Vertices[1].Y += 1;
        Player.Vertices[2].Y += 1;
    }
    if (PlayerInput.MoveDown)
    {
        Player.Vertices[0].Y -= 1;
        Player.Vertices[1].Y -= 1;
        Player.Vertices[2].Y -= 1;
    }
    if (PlayerInput.MoveRight)
    {
        Player.Vertices[0].X += 1;
        Player.Vertices[1].X += 1;
        Player.Vertices[2].X += 1;
    }
    if (PlayerInput.MoveLeft)
    {
        Player.Vertices[0].X -= 1;
        Player.Vertices[1].X -= 1;
        Player.Vertices[2].X -= 1;
    }
}

void InitDirectSound(HWND Window, int BufferSize, int SamplesPerSecond)
{
    LPDIRECTSOUND DirectSound;

    WAVEFORMATEX WaveFormat = {};
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = 2;
    WaveFormat.nSamplesPerSec = SamplesPerSecond;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
    WaveFormat.cbSize = 0;

    if (SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
    {
        DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY);
        
        // Initialize primary buffer.
        DSBUFFERDESC PrimaryBufferDesc = {};
        PrimaryBufferDesc.dwSize = sizeof(PrimaryBufferDesc);
        PrimaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
        
        
        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&PrimaryBufferDesc, &PrimaryBuffer, 0)))
        {
            PrimaryBuffer->SetFormat(&WaveFormat);
        }

        // Initialize secondary buffer.
        DSBUFFERDESC SecondaryBufferDesc = {};
        SecondaryBufferDesc.dwSize = sizeof(SecondaryBufferDesc);
        SecondaryBufferDesc.dwFlags = 0;
        SecondaryBufferDesc.dwBufferBytes = BufferSize;
        SecondaryBufferDesc.lpwfxFormat = &WaveFormat;

        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDesc, &GlobalSecondaryBuffer, 0)))
        {

        }
    }
}

/* As Windows will only ping us when there are messages to send, we
must set up a game loop outside of the window callback. Each time
through the WinMain loop, we will handle all Windows-related messages
first, then enter GameTick(), where our controller state will be
procured, our game state will be udpated, and the scene will be
rendered/blitted. WinMain must (eventually) implement the concept of a
frame rate/time delta, in order to allow for appropriate updates, as
well as to ensure we don't melt any CPUs. That delta will eventually
be passed to GameTick(), as well as the required HWND for blitting purposes. */
void GameTick(HWND WindHandle)
{
    XINPUT_GAMEPAD Controller1State = GetControllerInput(0);
    UpdateGameState(&Controller1State);
    HDC WindowDC = GetDC(WindHandle);
    RenderGame(WindHandle, WindowDC);
    ReleaseDC(WindHandle, WindowDC);
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
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top; // Negative to ensure top-down DIB
            ResizeDIBSection(Width, Height);
            return 0;
        }
        case WM_PAINT:
        {
			PAINTSTRUCT PaintStruct;
			RECT ClientRect;
			GetClientRect(WindHandle, &ClientRect);
			HDC PaintDC = BeginPaint(WindHandle, &PaintStruct);
			int X = PaintStruct.rcPaint.left;
			int Y = PaintStruct.rcPaint.top;
			int Width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
			int Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;

			DrawToWindow(PaintDC, &ClientRect, Width, Height);
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
            Player = {};
            BlackBrush = CreateSolidBrush(RGB(0,0,0));
            LineWidth = 3.0f;
            
            Vec2 PlayerLeft, PlayerTop, PlayerRight;
            PlayerLeft.X = 100, PlayerLeft.Y = 100;
            PlayerTop.X = 120, PlayerTop.Y = 130;
            PlayerRight.X = 140, PlayerRight.Y = 100;
            Player.Vertices[0] = PlayerLeft, Player.Vertices[1] = PlayerTop, Player.Vertices[2] = PlayerRight;

            ColorTriple PlayerColor;
            PlayerColor.Red = 100, PlayerColor.Blue = 100, PlayerColor.Green = 100;
            Player.Color = PlayerColor;
            int Hertz = 256;
            uint32_t SampleIndex = 0;
            int BytesPerSample = sizeof(int16_t) * 2;
            int SamplesPerSecond = 48000;
            int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;
            int SquareWavePeriod = SamplesPerSecond/Hertz;

            InitDirectSound(WindowHandle, SecondaryBufferSize, SamplesPerSecond);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            for (;;)
            {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        return 0;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                // TODO: Include timedelta/mechanisms for enforcing limited frame rate
                GameTick(WindowHandle);
                
                DWORD PlayCursor, WriteCursor;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD NumBytes;
                    DWORD SampleIndexToLock = (SampleIndex * BytesPerSample) % SecondaryBufferSize;
                    if (SampleIndexToLock > PlayCursor)
                    {
                        NumBytes = SecondaryBufferSize - SampleIndexToLock;
                        NumBytes += PlayCursor;
                    }
                    else
                    {
                        NumBytes = PlayCursor - SampleIndexToLock;
                    }
                    void *AudioPtr1, *AudioPtr2;
                    DWORD AudioBytes1, AudioBytes2;
                
                    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(SampleIndexToLock, NumBytes, &AudioPtr1, &AudioBytes1, &AudioPtr2, &AudioBytes2, 0)))
                    {
                        int16_t *SoundOut = (int16_t *)AudioPtr1;
                        DWORD AudioRegion1SampleCount = AudioBytes1/BytesPerSample;
                        DWORD AudioRegion2SampleCount = AudioBytes2/BytesPerSample;
                        for (DWORD i = 0; i < AudioRegion1SampleCount; ++i)
                        {
                            int16_t Sample = ((SampleIndex / (SquareWavePeriod / 2)) % 2) ? 1000 : -1000;
                            *SoundOut++ = Sample;
                            *SoundOut++ = Sample;
                            SampleIndex++;
                        }
                        for (DWORD i = 0; i < AudioRegion2SampleCount; ++i)
                        {   
                            int16_t Sample = ((SampleIndex / (SquareWavePeriod / 2)) % 2) ? 1000 : -1000;
                            *SoundOut++ = Sample;
                            *SoundOut++ = Sample;
                            SampleIndex++;
                        }
                    }
                    GlobalSecondaryBuffer->Unlock(AudioPtr1, AudioBytes1, AudioPtr2, AudioBytes2);
                }
            }
        }
    }   

    return 0;
}