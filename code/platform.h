#pragma once

#include <stdint.h>
#include <windows.h>
#include <DSound.h>

// Forward-declarations

#include "platform.fwd.h"

// Macros

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) ((Num) * 1024 * 1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// Constants

#define GAME_PERM_MEMORY_SIZE Megabytes(1024)
#define GAME_TRANSIENT_MEMORY_SIZE Megabytes(1024)


struct platform_bitmap_buffer
{
    BITMAPINFO InfoStruct;
    void *Memory;
    uint32_t Width;
    uint32_t Height;
    uint32_t Pitch;
    uint32_t BytesPerPixel;
};

struct platform_sound_buffer
{
    LPDIRECTSOUNDBUFFER SecondaryBuffer;
    int SamplesPerSecond;
    int SoundHz;
    int16_t Volume;
    uint32_t SampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
};

struct platform_player_input
{
    bool IsControllerActive;
    float NormalizedLX;
    float NormalizedLY;
    float NormalizedMagnitude;
    uint32_t StickDeadzone;
    uint32_t MaxMagnitude;
    bool A_Pressed;
    bool B_Pressed;
    float NormalizedLTrigger;
    float NormalizedRTrigger;
    float TriggerDeadzone;
    float TriggerMax;
    bool Start_Pressed;
};

bool ReadFileIntoBuffer(LPCSTR FileName, void *Buffer, DWORD SizeToRead);
bool WriteBufferIntoFile(LPCSTR FileName, void *Buffer, DWORD SizeToRead);
uint64_t PlatformGetFileSize(LPCSTR FileName);
void PlatformSetPixel(vec_2)