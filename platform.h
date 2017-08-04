#ifndef PLATFORM_H
#define PLATFORM_H

// Macros

#define HackyAssert(Expr) if(!(Expr)) { *(uint8_t *)0 = 0;}

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) ((Num) * 1024 * 1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// Constants

#define PLAYER_NUM_VERTICES 4
#define PLAYER_LINE_WIDTH 2.0f
#define PLAYER_RED 200
#define PLAYER_GREEN 200
#define PLAYER_BLUE 200

#define LARGE_ASTEROID_NUM_VERTICES 8
#define MEDIUM_ASTEROID_NUM_VERTICES 6
#define SMALL_ASTEROID_NUM_VERTICES 4

#define ASTEROID_LINE_WIDTH 1.5f
#define ASTEROID_RED 125
#define ASTEROID_GREEN 125
#define ASTEROID_BLUE 125

#define MAX_NUM_SPAWNED_ASTEROIDS 100

#define LASER_LINE_WIDTH 1.0f
#define LASER_NUM_VERTICES 2
#define LASER_RED 163
#define LASER_GREEN 42
#define LASER_BLUE 21

#define MAX_NUM_SPAWNED_LASERS 3

#define LASER_SPEED_MAG 20.0f

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
    bool A_Was_Pressed;
    bool B_Pressed;
    bool B_Was_Pressed;
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

struct memory_segment
{
    uint32_t Size;
    uint8_t *BaseStorageLocation;
    uint32_t Used;
};

bool ReadFileIntoBuffer(LPCTSTR FileName, void *Buffer, DWORD SizeToRead);
void UpdateGameAndRender(game_memory *, platform_bitmap_buffer *, platform_sound_buffer *, platform_player_input *);

#endif /* PLATFORM_H */