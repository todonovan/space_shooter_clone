#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>

#include "platform.h"

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

struct vert_set
{
    vec_2 *Verts;
};

typedef enum object_type {
    PLAYER,
    ASTEROID_LARGE,
    ASTEROID_MEDIUM,
    ASTEROID_SMALL
} object_type;

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
    float MaxMomentum;
    float OffsetAngle;
    float AngularMomentum;
    bool IsVisible;
};

struct asteroid_set
{
    game_object *Asteroids;
};

struct game_state
{
    memory_segment SceneMemorySegment;
    game_object *Player;
    uint32_t NumSpawnedAsteroids;
    asteroid_set *SpawnedAsteroids;
};

struct loaded_resource_memory
{
    memory_segment Segment;
    uint32_t Test;
};

struct game_permanent_memory
{
    game_state *GameState;
    loaded_resource_memory *Resources;
};

// Note that this may not be portable, as it relies upon the way Windows structures
// bitmap data in memory.
void SetPixelInBuffer(platform_bitmap_buffer *Buffer, vec_2 *Coords, color_triple *Colors)
{
    if (!(Buffer->Memory)) return;
    int X = (Coords->X < 0) ? ((int)Coords->X + Buffer->Width) : ((int)Coords->X % Buffer->Width);
    int Y = (Coords->Y < 0) ? ((int)Coords->Y + Buffer->Height) : ((int)Coords->Y % Buffer->Height);

    uint8_t *Pixel = (uint8_t *)((char *)(Buffer->Memory) + (Y * Buffer->Width * 4) + (X * 4));

    *Pixel = (Colors->Blue);
    Pixel++;
    *Pixel = (Colors->Green);
    Pixel++;
    *Pixel = (Colors->Red);
}

/// TODO!! This implementation of Bresenham, courtesy of the internet, is busted. Must find better line-drawing
/// alg that handles width properly. All Bresenham implementations seem to be poorly suited to width handling.
void DrawLineWidth(platform_bitmap_buffer *Buffer, vec_2 *Point1, vec_2 *Point2, color_triple *Color, float LineWidth)
{
    int x0 = (int)(Point1->X);
    int x1 = (int)(Point2->X);
    int y0 = (int)(Point1->Y);
    int y1 = (int)(Point2->Y);
    float wd = LineWidth;
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = dx-dy, e2, x2, y2;
    float ed = (dx+dy == 0) ? 1.0f : sqrtf(((float)dx*(float)dx)+((float)dy*(float)dy));


    for (wd = (wd+1)/2; ; )
    {
        vec_2 Coords;
        Coords.X = (float)(x0);
        Coords.Y = (float)(y0);
        SetPixelInBuffer(Buffer, &Coords, Color);
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                vec_2 Coords2;
                Coords2.X = (float)(x0);
                Coords2.Y = (float)(y2 += sy);

                SetPixelInBuffer(Buffer, &Coords2, Color);
            }
            if (x0 == x1) break;
            e2 = err; err -= dy; x0 += sx;
        }
        if (2*e2 <= dy)
        {
            for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
            {
                vec_2 Coords2;
                Coords2.X = (float)(x2 += sx);
                Coords2.Y = (float)(y0);
                SetPixelInBuffer(Buffer, &Coords2, Color);
            }
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }
}

void SetObjectModelForDraw(game_object *Object)
{
    object_model *Model = Object->Model;
    float Theta = Object->OffsetAngle;
    vec_2 *StartVerts = Model->StartVerts->Verts;
    vec_2 *DrawVerts = Model->DrawVerts->Verts;

    for (int i = 0; i < Model->NumVertices; ++i)
    {
        float X_Orig = StartVerts[i].X;
        float Y_Orig = StartVerts[i].Y;
        DrawVerts[i].X = (X_Orig * cosf(Theta)) - (Y_Orig * sinf(Theta));
        DrawVerts[i].Y = (X_Orig * sinf(Theta)) + (Y_Orig * cosf(Theta));
    }
}

inline void HandleObjectEdgeWarping(game_object *Object, int Width, int Height)
{
    if (Object->Midpoint.X < 0)
    {
        Object->Midpoint.X += Width;
    }
    else if (Object->Midpoint.X >= Width)
    {
        Object->Midpoint.X -= Width;
    }
    if (Object->Midpoint.Y < 0)
    {
        Object->Midpoint.Y += Height;
    }
    else if (Object->Midpoint.Y >= Height)
    {
        Object->Midpoint.Y -= Height;
    }
}

// TODO: This proc just doesn't feel right when moving the player ship. Will have to revise.
// Will go back to unchecked top speed for now.
/*void AdjustMomentumValuesAgainstMax(game_object *Object, float Input_X, float Input_Y)
{
    float raw_x = Object->X_Momentum + Input_X;
    float raw_y = Object->Y_Momentum + Input_Y;
    float magnitude = sqrt((raw_x * raw_x) + (raw_y * raw_y));
    if (magnitude > Object->MaxMomentum)
    {
        float ratio_x = raw_x / magnitude;
        float ratio_y = raw_y / magnitude;
        float adjusted_x_momentum = Object->MaxMomentum * ratio_x;
        float adjusted_y_momentum = Object->MaxMomentum * ratio_y;

        Object->X_Momentum = adjusted_x_momentum;
        Object->Y_Momentum =  adjusted_y_momentum;
    }
    else
    {
        Object->X_Momentum = raw_x;
        Object->Y_Momentum = raw_y;
    }
}
*/

void DrawObjectModelIntoBuffer(platform_bitmap_buffer *Buffer, game_object *Object)
{
    if (!(Buffer->Memory)) return;

    int cur = 0, next = 1;
    vec_2 Cur, Next;
    object_model *Model = Object->Model;
    vert_set *DrawVerts = Model->DrawVerts;

    for (int i = 0; i < Model->NumVertices - 1; ++i)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[next].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[next].Y + Object->Midpoint.Y;
        DrawLineWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
        ++cur;
        if (i < Model->NumVertices - 2) ++next;
    }
    Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
    Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
    Next.X = DrawVerts->Verts[0].X + Object->Midpoint.X;
    Next.Y = DrawVerts->Verts[0].Y + Object->Midpoint.Y;
    DrawLineWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
}

void BeginMemorySegment(memory_segment *Segment, uint32_t Size, uint8_t *Storage)
{
    Segment->Size = Size;
    Segment->BaseStorageLocation = Storage;
    Segment->Used = 0;
}

#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size)
{
    void *Result = Segment->BaseStorageLocation + Segment->Used;
    Segment->Used += Size;
    return Result;
}

void SetVertValue(vert_set *VertSet, uint32_t VertIndex, float XVal, float YVal)
{
    VertSet->Verts[VertIndex].X = XVal;
    VertSet->Verts[VertIndex].Y = YVal;
}

void SpawnAsteroid(game_state *GameState, memory_segment *MemorySegment, object_type AsteroidType, float X_Spawn, float Y_Spawn, float X_Mo, float Y_Mo, float AngularMomentum)
{
    game_object *NewAsteroid = &GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids];
    NewAsteroid->Type = AsteroidType;
    NewAsteroid->Model = PushToMemorySegment(MemorySegment, object_model);
    object_model *Model = NewAsteroid->Model;
    if (Model)
    {
        // Yes, as currently coded, this could easily be replaced by a look up table. But, I am assuming that
        // in the future, the spawn code will be slightly different in other ways for the different asteroid types.
        switch (NewAsteroid->Type)
        {
            case ASTEROID_LARGE:
            {
                Model->NumVertices = ASTEROID_LARGE_NUM_VERTICES;
            } break;
            case ASTEROID_MEDIUM:
            {
                Model->NumVertices = ASTEROID_MEDIUM_NUM_VERTICES;
            } break;
            case ASTEROID_SMALL:
            {
                Model->NumVertices = ASTEROID_SMALL_NUM_VERTICES;
            } break;
        }

        Model->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
        Model->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
        Model->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
        Model->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
        Model->Color.Red = ASTEROID_RED;
        Model->Color.Blue = ASTEROID_BLUE;
        Model->Color.Green = ASTEROID_GREEN;
        Model->LineWidth = ASTEROID_LINE_WIDTH;
        NewAsteroid->Midpoint.X = X_Spawn;
        NewAsteroid->Midpoint.Y = Y_Spawn;
        NewAsteroid->X_Momentum = X_Mo;
        NewAsteroid->Y_Momentum = Y_Mo;
        NewAsteroid->OffsetAngle = 0.0f;
        NewAsteroid->AngularMomentum = AngularMomentum;
        NewAsteroid->IsVisible = true;
    }
    else
    {
        // Out of memory; handle error -- logging? etc?
    }
    GameState->NumSpawnedAsteroids += 1;
}

void HandleSceneEdgeWarping(game_state *GameState, int Width, int Height)
{
    HandleObjectEdgeWarping(GameState->Player, Width, Height);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        HandleObjectEdgeWarping(&GameState->SpawnedAsteroids->Asteroids[i], Width, Height);
    }
}

void SetSceneModelsForDraw(game_state *GameState)
{
    SetObjectModelForDraw(GameState->Player);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        SetObjectModelForDraw(&GameState->SpawnedAsteroids->Asteroids[i]);
    }
}

void DrawSceneModelsIntoBuffer(platform_bitmap_buffer *Buffer, game_state *GameState)
{
    if (GameState->Player->IsVisible) DrawObjectModelIntoBuffer(Buffer, GameState->Player);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        game_object Asteroid = GameState->SpawnedAsteroids->Asteroids[i];
        if (Asteroid.IsVisible) DrawObjectModelIntoBuffer(Buffer, &Asteroid);
    }
}

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *PlayerInput)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        // Set up game memory here!

        BeginMemorySegment(&GameState->SceneMemorySegment, (Memory->PermanentStorageSize - sizeof(game_state)) / 2,
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_state));


        GameState->Player = PushToMemorySegment(&GameState->SceneMemorySegment, game_object);
        GameState->Player->Type = PLAYER;
        game_object *Player = GameState->Player;
        Player->Midpoint.X = (float)(OffscreenBuffer->Width / 2);
        Player->Midpoint.Y = (float)(OffscreenBuffer->Height / 10);

        Player->Model = PushToMemorySegment(&GameState->SceneMemorySegment, object_model);
        object_model *P_Model = Player->Model;
        P_Model->NumVertices = PLAYER_NUM_VERTICES;
        P_Model->StartVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
        P_Model->StartVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);

		P_Model->DrawVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
        P_Model->DrawVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);

        // NOTE! These values will need to be stored in a 'resource' file -- a config text file, whatever.
        SetVertValue(GameState->Player->Model->StartVerts, 0, -20.0f, -20.0f);
        SetVertValue(GameState->Player->Model->StartVerts, 1, 0.0f, 40.0f);
        SetVertValue(GameState->Player->Model->StartVerts, 2, 20.0f, -20.0f);
        SetVertValue(GameState->Player->Model->StartVerts, 3, 0.0f, 0.0f);

        P_Model->LineWidth = PLAYER_LINE_WIDTH;

		P_Model->Color.Red = 100, P_Model->Color.Blue = 100, P_Model->Color.Green = 100;
        Player->X_Momentum = 0.0f, Player->Y_Momentum = 0.0f;
        Player->OffsetAngle = 0.0f;
        Player->AngularMomentum = 0.1f;
        Player->MaxMomentum = 30.0f;
        Player->IsVisible = true;

        GameState->NumSpawnedAsteroids = 0;
        GameState->SpawnedAsteroids = PushToMemorySegment(&GameState->SceneMemorySegment, asteroid_set);
        GameState->SpawnedAsteroids->Asteroids = PushArrayToMemorySegment(&GameState->SceneMemorySegment, MAX_NUM_SPAWNED_ASTEROIDS, game_object);

        Memory->IsInitialized = true;
    }

    game_object *Player = GameState->Player;
    object_model *PlayerModel = Player->Model;

    if (PlayerInput->Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput->A_Pressed)
    {
        SpawnAsteroid(GameState, &GameState->SceneMemorySegment, ASTEROID_MEDIUM, (float)(OffscreenBuffer->Width / 10),
                                            (float)((OffscreenBuffer->Height / 10) * 7), -.25f, -1.0f, .005f);
        vert_set *A_S_Verts = GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids - 1].Model->StartVerts;
        SetVertValue(A_S_Verts, 0, -80.0f, 0.0f);
        SetVertValue(A_S_Verts, 1, -25.0f, 100.0f);
        SetVertValue(A_S_Verts, 2, 30.0f, 85.0f);
        SetVertValue(A_S_Verts, 3, 45.0f, 2.0f);
        SetVertValue(A_S_Verts, 4, 20.0f, -35.0f);
        SetVertValue(A_S_Verts, 5, -60.0f, -70.0f);
    }
    if (PlayerInput->B_Pressed)
    {
        PlayerModel->Color.Red = 0, PlayerModel->Color.Blue = 200;
    }

    game_object *Asteroids = GameState->SpawnedAsteroids->Asteroids;
    Player->OffsetAngle += Player->AngularMomentum * (PlayerInput->LTrigger + PlayerInput->RTrigger);

    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        Asteroids[i].OffsetAngle += Asteroids[i].AngularMomentum;
    }

    // Note: This procedure currently gives bad control feel; must be reworked.
    //AdjustMomentumValuesAgainstMax(Player, PlayerInput->NormalizedLX * PlayerInput->Magnitude,
    //                              PlayerInput->NormalizedLY * PlayerInput->Magnitude);

    Player->X_Momentum += (PlayerInput->NormalizedLX * PlayerInput->Magnitude * 0.8f);
    Player->Y_Momentum += (PlayerInput->NormalizedLY * PlayerInput->Magnitude * 0.8f);

    Player->Midpoint.X += Player->X_Momentum;
    Player->Midpoint.Y += Player->Y_Momentum;

    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        Asteroids[i].Midpoint.X += Asteroids[i].X_Momentum;
        Asteroids[i].Midpoint.Y += Asteroids[i].Y_Momentum;        
    }

    HandleSceneEdgeWarping(GameState, OffscreenBuffer->Width, OffscreenBuffer->Height);
    SetSceneModelsForDraw(GameState);
    DrawSceneModelsIntoBuffer(OffscreenBuffer, GameState);
}