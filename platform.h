#ifndef PLATFORM_H
#define PLATFORM_H

// Macros

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) ((Num) * 1024 * 1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define PLAYER_NUM_VERTICES 4
#define PLAYER_RED 100
#define PLAYER_BLUE 100
#define PLAYER_GREEN 100

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

struct vec_2
{
    float X;
    float Y;
};

struct color_triple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

typedef enum object_type {
    PLAYER,
    ASTEROID_LARGE,
    ASTEROID_MEDIUM,
    ASTEROID_SMALL
} object_type;

struct vert_set
{
    vec_2 *Verts;
};

struct object_model
{
    int NumVertices;
    vert_set *StartVerts;
    vert_set *DrawVerts;
    color_triple Color;
    float LineWidth;
};

struct game_object
{
    object_type Type;
    object_model *Model;
    vec_2 Midpoint;
    float X_Momentum;
    float Y_Momentum;
    float OffsetAngle;
    float AngularMomentum;
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

struct game_state
{
    memory_segment SceneMemorySegment;
    game_object *Player;
};

void UpdateGameAndRender(game_memory *, platform_bitmap_buffer *, platform_sound_buffer *, platform_player_input *);

#endif /* PLATFORM_H */