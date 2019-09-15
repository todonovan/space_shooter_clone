#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>
#include <tchar.h>
#include <stdint.h>

#include "platform.h"
#include "asteroids.h"

struct internal_game_window
{
    HWND WindowHandle;
    RECT ClientRect;
    HDC DeviceContext;
    uint64_t Width;
    uint64_t Height;
};

static int64_t PerfCountFrequency;

static platform_sound_buffer GlobalSoundBuffer;
static platform_bitmap_buffer GlobalBackbuffer;
static internal_game_window GameWindow;

void UpdateWindowDimensions(HWND WindowHandle)
{
    GameWindow.WindowHandle = WindowHandle;
    GameWindow.ClientRect = {};
    GetClientRect(GameWindow.WindowHandle, &(GameWindow.ClientRect));
    GameWindow.Width = GameWindow.ClientRect.right;
    GameWindow.Height = GameWindow.ClientRect.bottom;
}

void ResizeDIBSection(int Width, int Height)
{
    if (GlobalBackbuffer.Memory)
    {
        VirtualFree(GlobalBackbuffer.Memory, 0, MEM_RELEASE);
    }

    GlobalBackbuffer.Width = Width;
    GlobalBackbuffer.Height = Height;

    GlobalBackbuffer.InfoStruct.bmiHeader.biSize = sizeof(GlobalBackbuffer.InfoStruct.bmiHeader);
    GlobalBackbuffer.InfoStruct.bmiHeader.biWidth = GlobalBackbuffer.Width;
    GlobalBackbuffer.InfoStruct.bmiHeader.biHeight = GlobalBackbuffer.Height; // Negative to ensure top-down DIB
    GlobalBackbuffer.InfoStruct.bmiHeader.biPlanes = 1;
    GlobalBackbuffer.InfoStruct.bmiHeader.biBitCount = 32;
    GlobalBackbuffer.InfoStruct.bmiHeader.biCompression = BI_RGB;
    GlobalBackbuffer.BytesPerPixel = 4; // 4 bytes per pixel to keep dword aligned
    GlobalBackbuffer.Pitch = Width * GlobalBackbuffer.BytesPerPixel;
    int BitmapMemorySize = GlobalBackbuffer.Width * GlobalBackbuffer.Height * GlobalBackbuffer.BytesPerPixel;

    GlobalBackbuffer.Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

inline void ClearOffscreenBuffer()
{
    if (!(GlobalBackbuffer.Memory)) return;
	ZeroMemory(GlobalBackbuffer.Memory, GlobalBackbuffer.Height * GlobalBackbuffer.Width * 4);
}

void DrawToWindow(HDC DrawDC)
{
    StretchDIBits(DrawDC, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, 0, 0, GlobalBackbuffer.Width,
                  GlobalBackbuffer.Height, GlobalBackbuffer.Memory, &(GlobalBackbuffer.InfoStruct), DIB_RGB_COLORS, SRCCOPY);
}

platform_player_input GetPlayerInput(DWORD ControllerNumber, platform_player_input *LastInputState)
{
    XINPUT_STATE ControllerState;
    platform_player_input PlayerInput = {};

    if (XInputGetState(ControllerNumber, &ControllerState) == ERROR_SUCCESS)
    {
        PlayerInput.IsControllerActive = true;
        float LX = ControllerState.Gamepad.sThumbLX;
        float LY = ControllerState.Gamepad.sThumbLY;
        float Magnitude = sqrtf((LX * LX) + (LY * LY));
        PlayerInput.MaxMagnitude = 32767;
        PlayerInput.NormalizedLX = LX / Magnitude;
        PlayerInput.NormalizedLY = LY / Magnitude;

        float normalizedMagnitude = 0.0f;
        if (Magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            if (Magnitude > 32767)
            {
                Magnitude = 32767;
            }
            Magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
            PlayerInput.NormalizedMagnitude = Magnitude / ((float)(32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        }
        else
        {
            Magnitude = 0.0f;
            PlayerInput.NormalizedMagnitude = 0.0f;
        }

        uint8_t LeftTrig = (ControllerState.Gamepad.bLeftTrigger > 50) ? ControllerState.Gamepad.bLeftTrigger : 0;
        uint8_t RightTrig = (ControllerState.Gamepad.bRightTrigger > 50) ? ControllerState.Gamepad.bRightTrigger : 0;

        if (LeftTrig > 0)
        {
            PlayerInput.NormalizedLTrigger = ((float)LeftTrig - 50.0f) / (255.0f - 50.0f);
        }
        else
        {
            PlayerInput.NormalizedLTrigger = 0.0f;
        }

        if (RightTrig > 0)
        {
            PlayerInput.NormalizedRTrigger = -1.0f * (((float)RightTrig - 50.0f) / (255.0f - 50.0f));
        }
        else
        {
            PlayerInput.NormalizedRTrigger = 0.0f;
        }

        PlayerInput.TriggerDeadzone = 50;
        PlayerInput.A_Pressed = (ControllerState.Gamepad.wButtons & XINPUT_GAMEPAD_A);
        PlayerInput.B_Pressed = (ControllerState.Gamepad.wButtons & XINPUT_GAMEPAD_B);
        PlayerInput.Start_Pressed = (ControllerState.Gamepad.wButtons & XINPUT_GAMEPAD_START);
    }
    else
    {
        // A controller is not connected. This is necessary in order to not pass junk data
        // to rest of input handling code.
        PlayerInput.IsControllerActive = false;
        PlayerInput.NormalizedLX = 0.0f;
        PlayerInput.NormalizedLY = 0.0f;
        PlayerInput.NormalizedMagnitude = 0.0f;
        PlayerInput.StickDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        PlayerInput.A_Pressed = false;
        PlayerInput.B_Pressed = false;
        PlayerInput.NormalizedLTrigger = 0.0f;
        PlayerInput.NormalizedRTrigger = 0.0f;
        PlayerInput.TriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        PlayerInput.Start_Pressed = false;
    }

    return PlayerInput;
}

void InitDirectSound(int BufferSize, int SamplesPerSecond)
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
        DirectSound->SetCooperativeLevel(GameWindow.WindowHandle, DSSCL_PRIORITY);

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

        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDesc, &(GlobalSoundBuffer.SecondaryBuffer), 0)))
        {

        }
    }
}

void FillSoundBuffer(DWORD ByteToLock, DWORD BytesToWrite)
{
    void *AudioPtr1, *AudioPtr2;
    DWORD AudioBytes1, AudioBytes2;

    if (SUCCEEDED(GlobalSoundBuffer.SecondaryBuffer->Lock(ByteToLock, BytesToWrite, &AudioPtr1, &AudioBytes1, &AudioPtr2, &AudioBytes2, 0)))
    {
        int16_t *SoundOut = (int16_t *)AudioPtr1;
        DWORD AudioRegion1SampleCount = AudioBytes1/GlobalSoundBuffer.BytesPerSample;
        DWORD AudioRegion2SampleCount = AudioBytes2/GlobalSoundBuffer.BytesPerSample;
        for (DWORD i = 0; i < AudioRegion1SampleCount; ++i)
        {
            float t = 2.0f * 3.14159f * ((float)(GlobalSoundBuffer.SampleIndex) / (float)(GlobalSoundBuffer.WavePeriod));
            float SineValue = sinf(t);
            int16_t Sample = (int16_t)(SineValue * GlobalSoundBuffer.Volume);
            *SoundOut++ = Sample;
            *SoundOut++ = Sample;
            GlobalSoundBuffer.SampleIndex++;
        }

        SoundOut = (int16_t *)AudioPtr2;
        for (DWORD i = 0; i < AudioRegion2SampleCount; ++i)
        {
            float t = 2.0f * 3.14159f * ((float)(GlobalSoundBuffer.SampleIndex) / (float)(GlobalSoundBuffer.WavePeriod));
            float SineValue = sinf(t);
            int16_t Sample = (int16_t)(SineValue * GlobalSoundBuffer.Volume);
            *SoundOut++ = Sample;
            *SoundOut++ = Sample;
            GlobalSoundBuffer.SampleIndex++;
        }

        GlobalSoundBuffer.SecondaryBuffer->Unlock(AudioPtr1, AudioBytes1, AudioPtr2, AudioBytes2);
    }
}

// Returns true if read was successful, false otherwise
bool ReadFileIntoBuffer(LPCSTR FileName, void *Buffer, DWORD BytesToRead)
{
    DWORD NumBytesRead = 0;
    HANDLE File = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (ReadFile(File, Buffer, BytesToRead, &NumBytesRead, 0))
    {
        CloseHandle(File);
        return NumBytesRead == BytesToRead;
    }
    else
    {
        CloseHandle(File);
        return false;
    }
}

// Returns true if the write was successful, false otherwise
// Note that calling this function will truncate the associated file
bool WriteBufferIntoFile(LPCSTR FileName, void *Buffer, DWORD BytesToWrite)
{
    DWORD NumBytesWritten = 0;
    HANDLE File = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (WriteFile(File, Buffer, BytesToWrite, &NumBytesWritten, 0))
    {
        CloseHandle(File);
        return NumBytesWritten == BytesToWrite;
    }
    else
    {
        CloseHandle(File);
        return false;
    }
}

uint64_t PlatformGetFileSize(LPCSTR FileName)
{
    LARGE_INTEGER Size;

    HANDLE File = CreateFileA(FileName, FILE_READ_ATTRIBUTES, 0, 0, OPEN_EXISTING, 0, 0);
    GetFileSizeEx(File, &Size);
    CloseHandle(File);
    return Size.QuadPart;
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
        case WM_PAINT:
        {
			PAINTSTRUCT PaintStruct;
			UpdateWindowDimensions(WindHandle);
			HDC PaintDC = BeginPaint(WindHandle, &PaintStruct);
			DrawToWindow(PaintDC);
			EndPaint(WindHandle, &PaintStruct);
        } break;
        /*case WM_SIZE:
        {
            UpdateWindowDimensions(WindHandle);
            ResizeDIBSection((int)GameWindow.Width, (int)GameWindow.Height);
        } break;*/
        case WM_CLOSE: case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_UP:
                {
                    OutputDebugStringA("Up arrow");
                } break;
                default:
                {
                    OutputDebugStringA("Other key");
                } break;
            }
        } break;
        default:
        {
            Result = DefWindowProc(WindHandle, Message, wParam, lParam);
        } break;
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
    float Result = ((float)(EndTime.QuadPart - StartTime.QuadPart) / (float)(PerfCountFrequency));
    return Result;
}

// The graphical entry point for Windows.
int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CommandLine,
                     int CommandShow)
{
    WNDCLASS WindowClass = {};

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = &AsteroidsWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = (LPCSTR)"AsteroidsWindowClass";

    int MonitorRefreshRate = 60;
    int GameUpdateRate = 60;
    float ExpectedSecondsPerFrame = 1.0f / (float)(GameUpdateRate);

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
            // Initialize window and bitmap buffer.
            GameWindow = {};
            GameWindow.WindowHandle = WindowHandle;
			GameWindow.ClientRect = {};
			UpdateWindowDimensions(WindowHandle);
            ResizeDIBSection(1980, 1020);


            // Initialize DirectSound (secondary) buffer.
            GlobalSoundBuffer = {};

            GlobalSoundBuffer.SoundHz = 256;
            GlobalSoundBuffer.Volume = 1000;
            GlobalSoundBuffer.SampleIndex = 0;
            GlobalSoundBuffer.BytesPerSample = sizeof(int16_t) * 2;
            GlobalSoundBuffer.SamplesPerSecond = 48000;
            GlobalSoundBuffer.SecondaryBufferSize = GlobalSoundBuffer.SamplesPerSecond * GlobalSoundBuffer.BytesPerSample;
            GlobalSoundBuffer.WavePeriod = GlobalSoundBuffer.SamplesPerSecond/GlobalSoundBuffer.SoundHz;
            GlobalSoundBuffer.LatencySampleCount = GlobalSoundBuffer.SamplesPerSecond / 15;

            InitDirectSound(GlobalSoundBuffer.SecondaryBufferSize, GlobalSoundBuffer.SamplesPerSecond);
            FillSoundBuffer(0, GlobalSoundBuffer.LatencySampleCount * GlobalSoundBuffer.BytesPerSample);
            GlobalSoundBuffer.SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            // Allocate game memory.

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = GAME_PERM_MEMORY_SIZE;
            GameMemory.PermanentStorage = VirtualAlloc(0, GameMemory.PermanentStorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorageSize = GAME_TRANSIENT_MEMORY_SIZE;
            GameMemory.TransientStorage = VirtualAlloc(0, GameMemory.TransientStorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);


            // Initialize timing code.
            LARGE_INTEGER LastCounter = GetWallClock();
			uint64_t LastCPUCycleCount = __rdtsc();

            // Initial input retrieval.
            platform_player_input LastInput = {};
            platform_player_input CurrentInput = {};

            /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
             * Game loop:                                              *
             *    1. Handle all Windows messages from queue            *
             *    2. Retrieve input from controller                    *
             *    3. Clear the offscreen buffer                        *
             *    4. Call into game code with appropriate parameters   *
             *    5. Flip the bitmap buffer to Windows                 *
             *    6. Update timing monitors                            *
             * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
            for (;;)
            {
                // Loop to process all Windows messages. PeekMessage pulls
                // messages off the hidden queue. The messages are translated
                // and dispatched via standard Win32 calls. Note, however, that
                // Windows reserves the right to call the window callback
                // directly, skipping the message queue system entirely.
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        return 0;
                    }

                    // Per MSDN: Translates virtual-key messages into character messages.
                    // If the msg is a virtual-key msg, it is translated and reposted to
                    // the queue. The current msg, however, is NOT modified -- a new one
                    // is created.
                    TranslateMessage(&Message);

                    // Sends the message to the defined window callback.
                    DispatchMessage(&Message);
                }

                ClearOffscreenBuffer();

                LastInput = CurrentInput;
                CurrentInput = GetPlayerInput(0, &LastInput);

                UpdateGameAndRender(&GameMemory, &GlobalBackbuffer, &GlobalSoundBuffer, &CurrentInput, &LastInput);

                HDC WindowDC = GetDC(WindowHandle);
                DrawToWindow(WindowDC);
                ReleaseDC(WindowHandle, WindowDC);

                DWORD PlayCursor, WriteCursor;
                if (SUCCEEDED(GlobalSoundBuffer.SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD NumBytes;
                    DWORD TargetCursor = (PlayCursor + (GlobalSoundBuffer.LatencySampleCount * GlobalSoundBuffer.BytesPerSample)) % GlobalSoundBuffer.SecondaryBufferSize;
                    DWORD SampleIndexToLock = (GlobalSoundBuffer.SampleIndex * GlobalSoundBuffer.BytesPerSample) % GlobalSoundBuffer.SecondaryBufferSize;
                    if (SampleIndexToLock > TargetCursor)
                    {
                        NumBytes = GlobalSoundBuffer.SecondaryBufferSize - SampleIndexToLock;
                        NumBytes += TargetCursor;
                    }
                    else
                    {
                        NumBytes = TargetCursor - SampleIndexToLock;
                    }
                    FillSoundBuffer(SampleIndexToLock, NumBytes);
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
                        DWORD SleepLength = (DWORD)((1000.0f * (ExpectedSecondsPerFrame - SecondsElapsedForFrame)));
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
				float MegaCyclesPerFrame = (float)(CPUCyclesElapsed / (1000.0f * 1000.0f));

				char Buffer[256];
				sprintf_s(Buffer, "%.02fms/f, %.02fmc/f\n", MilliSecsPerFrame, MegaCyclesPerFrame);
                OutputDebugStringA(Buffer);

                LastCounter = GetWallClock();
				LastCPUCycleCount = EndCPUCycleCount;
            }
        }
    }

    return 0;
}