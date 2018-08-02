#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <windows.h>
#include <DSound.h>

// Macros

#define HackyAssert(Expr) if(!(Expr)) { *(uint8_t *)0 = 0;}

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) ((Num) * 1024 * 1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// Constants

#define GAME_PERM_MEMORY_SIZE Megabytes(256)
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
    float LX;
    float LY;
    uint32_t StickDeadzone;
    uint32_t MaxMagnitude;
    bool A_Pressed;
    bool B_Pressed;
    float LTrigger;
    float RTrigger;
    float TriggerDeadzone;
    float TriggerMax;
    bool Start_Pressed;
};

struct game_memory
{
    bool IsInitialized;
    uint32_t PermanentStorageSize;
    void *PermanentStorage;
    uint32_t TransientStorageSize;
    void *TransientStorage;
};

struct memory_segment
{
    uint32_t Size;
    uint8_t *BaseStorageLocation;
    uint32_t Used;
};

bool ReadFileIntoBuffer(LPCTSTR FileName, void *Buffer, DWORD SizeToRead);
void UpdateGameAndRender(game_memory *, platform_bitmap_buffer *, platform_sound_buffer *, platform_player_input *);

#endif /* PLATFORM_H */