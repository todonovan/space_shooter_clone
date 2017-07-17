#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <xinput.h>
#include <dsound.h>
#include <stdexcept>

static int64_t PerfCountFrequency;
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
    float X;
    float Y;
};

struct ColorTriple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

struct PlayerControlInput
{
    float Magnitude;
    float NormalizedLX;
    float NormalizedLY;
    bool A_Pressed;
    bool B_Pressed;
    bool LTrigger_Pressed;
    bool RTrigger_Pressed;
    bool Start_Pressed;
};

struct PlayerModel
{
    Vec2 Vertices[4];
    ColorTriple Color;
};

struct PlayerObject
{
    PlayerModel *Model;
    Vec2 Midpoint;
    float X_Momentum;
    float Y_Momentum;
    float AngularMomentum;
};

struct SoundOutputParams
{
    int SamplesPerSecond;
    int SoundHz;
    int16_t Volume;
    uint32_t SampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
};

static PlayerObject Player;

void SetPixelInBuffer(Vec2 *Coords, ColorTriple *Colors, int WindowWidth, int WindowHeight)
{
    if (!BitmapMemory) return;
    int X = (Coords->X < 0) ? ((int)Coords->X + WindowWidth) : ((int)Coords->X % WindowWidth);
    int Y = (Coords->Y < 0) ? ((int)Coords->Y + WindowHeight) : ((int)Coords->Y % WindowHeight);

    uint8_t *Pixel = (uint8_t *)((char *)BitmapMemory + (Y * BitmapWidth * 4) + (X * 4));

    *Pixel = Colors->Blue;
    Pixel++;
    *Pixel = Colors->Green;
    Pixel++;
    *Pixel = Colors->Red;
}

/// TODO!! Restore the actual width capabilities (e.g., reducing brightness incrementally further away pixel is from center of line)
void DrawLineWidth(Vec2 *Point1, Vec2 *Point2, ColorTriple *Color, int WindowWidth, int WindowHeight)
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
        // To restore the anti-aliasing, this proc would have to take a param
        // that represented the amount to scale down the 'brightness' of the pixel.
        SetPixelInBuffer(&Coords, Color, WindowWidth, WindowHeight);
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                Vec2 Coords2;
                Coords2.X = x0;
                Coords2.Y = y2 += sy;
                
                SetPixelInBuffer(&Coords2, Color, WindowWidth, WindowHeight);
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
                SetPixelInBuffer(&Coords2, Color, WindowWidth, WindowHeight);
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
    if (!BitmapMemory) return;
	ZeroMemory(BitmapMemory, BitmapHeight * BitmapWidth * 4);
}

void DrawPlayer(int WindowWidth, int WindowHeight)
{
    if (!BitmapMemory) return;

    int cur = 0, next = 1;
    Vec2 Cur, Next;

    for (int i = 0; i < 3; ++i)
    {
        Cur.X = Player.Model->Vertices[cur].X + Player.Midpoint.X;
        Cur.Y = Player.Model->Vertices[cur].Y + Player.Midpoint.Y;
        Next.X = Player.Model->Vertices[next].X + Player.Midpoint.X;
        Next.Y = Player.Model->Vertices[next].Y + Player.Midpoint.Y;
        DrawLineWidth(&Cur, &Next, &(Player.Model->Color), WindowWidth, WindowHeight);
        ++cur;
        if (i < 2) ++next;
    }
    Cur.X = Player.Model->Vertices[cur].X + Player.Midpoint.X;
    Cur.Y = Player.Model->Vertices[cur].Y + Player.Midpoint.Y;
    Next.X = Player.Model->Vertices[0].X + Player.Midpoint.X;
    Next.Y = Player.Model->Vertices[0].Y + Player.Midpoint.Y;
    DrawLineWidth(&Cur, &Next, &(Player.Model->Color), WindowWidth, WindowHeight);
}

void DrawToWindow(HDC WindowDC, RECT *WindowRect, int Width, int Height)
{   
    ClearBuffer();
    DrawPlayer(Width, Height);
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

void RotatePlayerModel(bool CounterClockwise)
{
    char RotateSign = 1;
    if (!CounterClockwise) RotateSign *= -1;

    for (int i = 0; i < 4; ++i)
    {
        float Theta = Player.AngularMomentum * RotateSign;
        float X_Orig = Player.Model->Vertices[i].X;
        float Y_Orig = Player.Model->Vertices[i].Y;
        Player.Model->Vertices[i].X = (X_Orig * cos(Theta)) - (Y_Orig * sin(Theta));
        Player.Model->Vertices[i].Y = (X_Orig * sin(Theta)) + (Y_Orig * cos(Theta));
    }
}

void AdjustPlayerCoordinates(int WindowWidth, int WindowHeight)
{
    if (Player.Midpoint.X < 0)
    {
        Player.Midpoint.X += WindowWidth;
    }
    else if (Player.Midpoint.X >= WindowWidth)
    {
        Player.Midpoint.X -= WindowWidth;
    }
    if (Player.Midpoint.Y < 0)
    {
        Player.Midpoint.Y += WindowHeight;
    }
    else if (Player.Midpoint.Y >= WindowHeight)
    {
        Player.Midpoint.Y -= WindowHeight;
    }
}

void UpdateGameState(XINPUT_GAMEPAD *Controller, long WindowWidth, long WindowHeight)
{
    PlayerControlInput PlayerInput;
    float LX = Controller->sThumbLX;
    float LY = Controller->sThumbLY;
    float magnitude = sqrt(LX*LX + LY*LY);

    PlayerInput.NormalizedLX = LX / magnitude;
    PlayerInput.NormalizedLY = LY / magnitude;

    float normalizedMagnitude = 0;

    if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        if (magnitude > 32767) magnitude = 32767;
        magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        normalizedMagnitude = magnitude / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }
    else
    {
        magnitude = 0.0;
        normalizedMagnitude = 0.0;
    }
    PlayerInput.Magnitude = normalizedMagnitude;

    PlayerInput.A_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_A);
    PlayerInput.B_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_B);
    PlayerInput.LTrigger_Pressed = Controller->bLeftTrigger > 50;
    PlayerInput.RTrigger_Pressed = Controller->bRightTrigger > 50;
    PlayerInput.Start_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_START);

    if (PlayerInput.Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput.B_Pressed)
    {
        Player.Model->Color.Red = 0, Player.Model->Color.Blue = 200;
    }
    if (PlayerInput.LTrigger_Pressed)
    {
        RotatePlayerModel(true);
    }
    if (PlayerInput.RTrigger_Pressed)
    {
        RotatePlayerModel(false);
    }
    
    Player.X_Momentum += (PlayerInput.NormalizedLX * PlayerInput.Magnitude) * .5f;
    Player.Y_Momentum += (PlayerInput.NormalizedLY * PlayerInput.Magnitude) * .5f;

    Player.Midpoint.X += Player.X_Momentum;
    Player.Midpoint.Y += Player.Y_Momentum;
    AdjustPlayerCoordinates(WindowWidth, WindowHeight);
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
    RECT WinRect;
    GetClientRect(WindHandle, &WinRect);
    XINPUT_GAMEPAD Controller1State = GetControllerInput(0);
    UpdateGameState(&Controller1State, WinRect.right, WinRect.bottom);
    HDC WindowDC = GetDC(WindHandle);
    RenderGame(WindHandle, WindowDC);
    ReleaseDC(WindHandle, WindowDC);
}

void FillSoundBuffer(SoundOutputParams *SoundParams, DWORD ByteToLock, DWORD BytesToWrite)
{
    void *AudioPtr1, *AudioPtr2;
    DWORD AudioBytes1, AudioBytes2;

    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &AudioPtr1, &AudioBytes1, &AudioPtr2, &AudioBytes2, 0)))
    {
        int16_t *SoundOut = (int16_t *)AudioPtr1;
        DWORD AudioRegion1SampleCount = AudioBytes1/SoundParams->BytesPerSample;
        DWORD AudioRegion2SampleCount = AudioBytes2/SoundParams->BytesPerSample;
        for (DWORD i = 0; i < AudioRegion1SampleCount; ++i)
        {
            float t = 2.0f * 3.14159 * (static_cast<float>(SoundParams->SampleIndex) / static_cast<float>(SoundParams->WavePeriod));
            float SineValue = sinf(t);
            int16_t Sample = (int16_t)(SineValue * SoundParams->Volume);
            *SoundOut++ = Sample;
            *SoundOut++ = Sample;
            SoundParams->SampleIndex++;
        }

        SoundOut = (int16_t *)AudioPtr2;
        for (DWORD i = 0; i < AudioRegion2SampleCount; ++i)
        {   
            float t = 2.0f * 3.14159 * (static_cast<float>(SoundParams->SampleIndex) / static_cast<float>(SoundParams->WavePeriod));
            float SineValue = sinf(t);
            int16_t Sample = (int16_t)(SineValue * SoundParams->Volume);
            *SoundOut++ = Sample;
            *SoundOut++ = Sample;
            SoundParams->SampleIndex++;
        }
        
        GlobalSecondaryBuffer->Unlock(AudioPtr1, AudioBytes1, AudioPtr2, AudioBytes2);
    }
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

LARGE_INTEGER GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

float GetSecondsElapsed(LARGE_INTEGER StartTime, LARGE_INTEGER EndTime)
{
    float Result = (static_cast<float>(EndTime.QuadPart - StartTime.QuadPart) / static_cast<float>(PerfCountFrequency));
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

    int MonitorRefreshRate = 60;
    int GameUpdateRate = 60;
    float ExpectedSecondsPerFrame = 1.0f / static_cast<float>(GameUpdateRate);

    // Set scheduler granularity to 1ms for Sleep() purposes
    timeBeginPeriod(1);
    LARGE_INTEGER PerfCountFrequencyUnion;
    QueryPerformanceFrequency(&PerfCountFrequencyUnion);
    PerfCountFrequency = PerfCountFrequencyUnion.QuadPart;

    // RegisterClass returns an ATOM, which we likely will not need to store.
    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(0, "AsteroidsWindow", "Asteroids", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (WindowHandle)
        {
            Player = {};
            PlayerModel Model = {};
            BlackBrush = CreateSolidBrush(RGB(0,0,0));
            LineWidth = 3.0f;
            RECT WinRect;
            GetClientRect(WindowHandle, &WinRect);
            
            Vec2 Midpoint = {};
            Midpoint.X = WinRect.right / 2;
            Midpoint.Y = WinRect.bottom / 10;
            Player.Midpoint = Midpoint;
            Vec2 PlayerLeft, PlayerTop, PlayerRight, PlayerBottom;
            PlayerLeft.X = -20.0f, PlayerLeft.Y = -20.0f;
            PlayerTop.X = 0.0f, PlayerTop.Y = 40.0f;
            PlayerRight.X = 20.0f, PlayerRight.Y = -20.0f;
            PlayerBottom.X = 0.0f, PlayerBottom.Y = 0.0f;
            Model.Vertices[0] = PlayerLeft, Model.Vertices[1] = PlayerTop, Model.Vertices[2] = PlayerRight, Model.Vertices[3] = PlayerBottom;

            ColorTriple PlayerColor;
            PlayerColor.Red = 100, PlayerColor.Blue = 100, PlayerColor.Green = 100;
            Model.Color = PlayerColor;
            Player.Model = &Model;

            // This value currently is fixed and does not change
            Player.AngularMomentum = 0.1f; 
            
            SoundOutputParams SoundParams = {};

            SoundParams.SoundHz = 256;
            SoundParams.Volume = 1000;
            SoundParams.SampleIndex = 0;
            SoundParams.BytesPerSample = sizeof(int16_t) * 2;
            SoundParams.SamplesPerSecond = 48000;
            SoundParams.SecondaryBufferSize = SoundParams.SamplesPerSecond * SoundParams.BytesPerSample;
            SoundParams.WavePeriod = SoundParams.SamplesPerSecond/SoundParams.SoundHz;
            SoundParams.LatencySampleCount = SoundParams.SamplesPerSecond / 15;

            InitDirectSound(WindowHandle, SoundParams.SecondaryBufferSize, SoundParams.SamplesPerSecond);
            FillSoundBuffer(&SoundParams, 0, SoundParams.LatencySampleCount * SoundParams.BytesPerSample);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            LARGE_INTEGER LastCounter = GetWallClock();
            
			uint64_t LastCPUCycleCount = __rdtsc();

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
                    DWORD TargetCursor = (PlayCursor + (SoundParams.LatencySampleCount * SoundParams.BytesPerSample)) % SoundParams.SecondaryBufferSize;
                    DWORD SampleIndexToLock = (SoundParams.SampleIndex * SoundParams.BytesPerSample) % SoundParams.SecondaryBufferSize;
                    if (SampleIndexToLock > TargetCursor)
                    {
                        NumBytes = SoundParams.SecondaryBufferSize - SampleIndexToLock;
                        NumBytes += TargetCursor;
                    }
                    else
                    {
                        NumBytes = TargetCursor - SampleIndexToLock;
                    }
                    FillSoundBuffer(&SoundParams, SampleIndexToLock, NumBytes);                    
                }                
                
				uint64_t EndCPUCycleCount = __rdtsc();

				LARGE_INTEGER WorkCounter = GetWallClock();			

				int64_t CPUCyclesElapsed = EndCPUCycleCount - LastCPUCycleCount;
                
                float SecondsElapsedForCompute = GetSecondsElapsed(LastCounter, WorkCounter);

                float SecondsElapsedForFrame = SecondsElapsedForCompute;
                if (SecondsElapsedForFrame < ExpectedSecondsPerFrame)
                {
                    while (SecondsElapsedForFrame < ExpectedSecondsPerFrame)
                    {
                        DWORD SleepLength = static_cast<DWORD>((1000.0f * (ExpectedSecondsPerFrame - SecondsElapsedForFrame)));
						if (SleepLength > 0)
						{
							Sleep(SleepLength);
						}
                        SecondsElapsedForFrame = GetSecondsElapsed(LastCounter, GetWallClock());
                    }
                }
                else
                {
                    // Handle missed frame
                }

                LARGE_INTEGER EndCounter = GetWallClock();
                float MilliSecsPerFrame = 1000.0f * (GetSecondsElapsed(LastCounter, EndCounter));
				float MegaCyclesPerFrame = static_cast<float>(CPUCyclesElapsed / (1000.0f * 1000.0f));

				char Buffer[256];
				sprintf(Buffer, "%.02fms/f, %.02fmc/f\n", MilliSecsPerFrame, MegaCyclesPerFrame);
                OutputDebugStringA(Buffer);

                LastCounter = GetWallClock();
				LastCPUCycleCount = EndCPUCycleCount;
            }
        }
    }   

    return 0;
}