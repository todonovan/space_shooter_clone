#ifndef PLATFORM_H
#define PLATFORM_H

// Macros

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) ((Num) * 1024 * 1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// What structs do we need here?
/*

- Bitmap/buffer
- Sound/buffer
- Input handling
- IO functions
- Game window info? probably not
*/

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
    float Magnitude;
    float NormalizedLX;
    float NormalizedLY;
    bool A_Pressed;
    bool B_Pressed;
    float LTrigger;
    float RTrigger;
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

void UpdateGameAndRender(game_memory *, platform_bitmap_buffer *, platform_sound_buffer *, platform_player_input *);

#endif /* PLATFORM_H */