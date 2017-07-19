#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <xinput.h>
#include <dsound.h>
#include <stdexcept>

#include "platform.h"

struct internal_game_window
{
    HWND WindowHandle;
    RECT ClientRect;
    HDC DeviceContext;
    uint16_t Width;
    uint16_t Height;
};

struct platform_bitmap_buffer
{
    BITMAPINFO InfoStruct;
    void *Memory;
    uint32_t Width;
    uint32_t Height;
    uint32_t Pitch;
    uint32_t BytesPerPixel;
};

struct platform_player_input
{
    float NormalizedMagnitude;
    float NormalizedLX;
    float NormalizedLY;
    bool A_Pressed;
    bool B_Pressed;
    bool LTrigger_Pressed;
    bool RTrigger_Pressed;
    bool Start_Pressed;
};

struct platform_sound_buffer
{
    LPDIRECTSOUNDBUFFER Memory;
    int SamplesPerSecond;
    int SoundHz;
    int16_t Volume;
    uint32_t SampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
};

#include "asteroids.cpp"

static int64_t PerfCountFrequency;

static platform_sound_buffer GlobalSecondaryBuffer;
static platform_bitmap_buffer GlobalBackbuffer;
static internal_game_window GameWindow;

void UpdateWindowDimensions(HWND WindowHandle)
{
    GameWindow.WindowHandle = WindowHandle;
    GameWindow.ClientRect = {};
    GetClientRect(GameWindow.WindowHandow, &(GameWindow.ClientRect));
    GameWindow.Width = GameWindow.ClientRect.right - GameWindow.ClientRect.left;
    GameWindow.Height = GameWindow.ClientRect.bottom - GameWindow.ClientRect.top; // Negative to ensure top-down DIB
}

void ResizeDIBSection(int Width, int Height)
{
    if (GlobalBackbuffer.BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    GlobalBackbuffer.Width = Width;
    GlobalBackbuffer.Height = Height;

    GlobalBackbuffer.InfoStruct.bmiHeader.biSize = sizeof(GlobalBackbuffer.InfoStruct.bmiHeader);
    GlobalBackbuffer.InfoStruct.bmiHeader.biWidth = BitmapWidth;
    GlobalBackbuffer.InfoStruct.bmiHeader.biHeight = -BitmapHeight;
    GlobalBackbuffer.InfoStruct.bmiHeader.biPlanes = 1;
    GlobalBackbuffer.InfoStruct.bmiHeader.biBitCount = 32;
    GlobalBackbuffer.InfoStruct.bmiHeader.biCompression = BI_RGB;
    GlobalBackbuffer.BytesPerPixel = 4; // 4 bytes per pixel to keep dword aligned
    GlobalBackbuffer.Pitch = Width * GlobalBackbuffer.BytesPerPixel;
    int BitmapMemorySize = GlobalBackbuffer.BitmapWidth * GlobalBackbuffer.Height * GlobalBackbuffer.BytesPerPixel;

    GlobalBackbuffer.BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

inline void ClearOffscreenBuffer()
{
    if (!(GlobalBackbuffer->Memory)) return;
	ZeroMemory(GlobalBackbuffer->BitmapMemory, GlobalBackbuffer->Height * GlobalBackbuffer->Width * 4);
}

void DrawToWindow()
{
    StretchDIBits(GameWindow.DeviceContext, 0, 0, GameWindow.Width, GameWindow.Height, 0, 0, GlobalBackbuffer.Width,
                  GlobalBackbuffer.Height, GlobalBackbuffer->Memory, &(GlobalBackbuffer->BitmapInfo), DIB_RGB_COLORS, SRCCOPY);
}

platform_player_input GetPlayerInput(DWORD ControllerNumber)
{
    XINPUT_STATE ControllerState;
    platform_player_input PlayerInput = {};

    if (XInputGetState(ControllerNumber, &ControllerState) == ERROR_SUCCESS)
    {
        float LX = ControllerState.Gamepad.sThumbLX;
        float LY = ControllerState.Gamepad.sThumbLY;
        float magnitude = sqrt((LX * LX) + (LY * LY));

        float normalizedMagnitude;
        if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            if (magnitude > 32767)
            {
                magnitude = 32767;
            }
            magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
            normalizedMagnitude = magnitude / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        }
        else
        {
            magnitude = 0.0f;
            normalizedMagnitude = 0.0f;
        }

        PlayerInput.NormalizedMagnitude = normalizedMagnitude;
        PlayerInput.NormalizedLX = LX / magnitude;
        PlayerInput.NormalizedLY = LY / magnitude;

        PlayerInput.A_Pressed = (ControllerState.Gamepad.wButtons & XINPUT_GAMEPAD_A);
        PlayerInput.B_Pressed = (ControllerState.Gamepad.wButtons & XINPUT_GAMEPAD_B);
        PlayerInput.LTrigger_Pressed = (Controller->bLeftTrigger > 50);
        PlayerInput.RTrigger_Pressed = (Controller->bRightTrigger > 50);
        PlayerInput.Start_Pressed = (ControllerState.Gamepad.wButtons & XINPUT_GAMEPAD_START);
    }
    
    return PlayerInput;
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


void FillSoundBuffer(PlatformSoundOutputParams *SoundParams, DWORD ByteToLock, DWORD BytesToWrite)
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
        case WM_SIZE:
        {
            UpdateWindowDimensions();
        }
        case WM_PAINT:
        {
			PAINTSTRUCT PaintStruct;
			UpdateWindowDimensions(WindHandle);
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
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = &AsteroidsWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "AsteroidsWindowClass";

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
        HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Space Shooter", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (WindowHandle)
        {
            GameWindow = {};
            GameWindow.WindowHandle = WindowHandle;
            
            ResizeDIBSection(1280, 720);
            Player = {};
            player_model Model = {};
            Model.LineWidth = 3.0f;

            vec_2 Midpoint = {};
            Midpoint.X = WinRect.right / 2;
            Midpoint.Y = WinRect.bottom / 10;
            Player.Midpoint = Midpoint;
            vec_2 PlayerLeft, PlayerTop, PlayerRight, PlayerBottom;
            PlayerLeft.X = -20.0f, PlayerLeft.Y = -20.0f;
            PlayerTop.X = 0.0f, PlayerTop.Y = 40.0f;
            PlayerRight.X = 20.0f, PlayerRight.Y = -20.0f;
            PlayerBottom.X = 0.0f, PlayerBottom.Y = 0.0f;
            Model.StartVertices[0] = PlayerLeft, Model.StartVertices[1] = PlayerTop, Model.StartVertices[2] = PlayerRight, Model.StartVertices[3] = PlayerBottom;
            Model.DrawVertices[0] = PlayerLeft, Model.DrawVertices[1] = PlayerTop, Model.DrawVertices[2] = PlayerRight, Model.DrawVertices[3] = PlayerBottom;

            color_triple PlayerColor;
            PlayerColor.Red = 100, PlayerColor.Blue = 100, PlayerColor.Green = 100;
            Model.Color = PlayerColor;
            Player.Model = &Model;

            // This value currently is fixed and does not change
            Player.AngularMomentum = 0.1f;

            PlatformSoundOutputParams SoundParams = {};

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

            platform_player_input LastInput = {};
            platform_player_input CurrentInput = GetPlayerInput(0);

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
                ClearOffscreenBuffer(platform_bitmap_buffer *Buffer);
                LastInput = CurrentInput;
                CurrentInput = GetPlayerInput(0);
                    UpdateGameState(&Controller1State, WinRect.right, WinRect.bottom);
                    HDC WindowDC = GetDC(WindHandle);
                    RenderGame(WindHandle, WindowDC);
                    ReleaseDC(WindHandle, WindowDC);

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