#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>
#include <stdlib.h>

#include "platform.h"

/* Thoughts...
    - Do I need some sort of 'entity' system?
    - How will I keep track of "spawned" / alive entities w/ my memory model?
    - This will be important for collision detection, etc.
    - Speaking of which, how will collision detection be done?
    - Use pairwise (for each vector in model) line/line intersection between close objects to determine if collision occurs
    - How to determine 'closeness' of objects? One way is to do a radius check
    - Note also that 'lines' may cross the screen boundaries
    - May need to transition "draw vertices" away from a simple copy of start vertices w/ rotation applied, to a different structure
    - A procedure to generate draw vertices could start at Vert[0], then check Vert[1]; if a boundary cross occurs, then instead of
        simply moving on to Vert[1], add two intermediary vertices -- one on each side of the screen boundary
    - So if a screen-crossing occurs between RawVert[0] and RawVert[1], the draw verts might look like:
        DrawVert[0] = RawVert[0]
        DrawVert[1] = ScreenBoundary1
        DrawVert[2] = ScreenBoundary2
        DrawVert[3] = RawVert[1]
    - Note that some concept of a screen boundary would have to be added to draw code to ensure the proc doesn't try to draw a line connecting
        the two screen boundaries
    - Perhaps DrawVert is a separate struct, with a member that states whether the vert occurs on a screen boundary

    - May have to ACTUALLY split up DrawVerts, CollidingVerts, etc.

*/

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
    ASTEROID_SMALL,
    LASER
} object_type;

struct object_model
{
    uint32_t NumVertices;
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

struct laser_set
{
    game_object *Lasers;
    uint32_t *LifeTimers;
};

struct game_state
{
    memory_segment SceneMemorySegment;
    game_object *Player;
    uint32_t NumSpawnedAsteroids;
    asteroid_set *SpawnedAsteroids;
    uint32_t MaxNumLasers;
    uint32_t NumSpawnedLasers;
    laser_set *LaserSet;
};

struct loaded_resource_memory
{
    memory_segment ResourceMemorySegment;
    vec_2 *PlayerVertices;
    vec_2 *SmallAsteroidVertices;
    vec_2 *MediumAsteroidVertices;
    vec_2 *LargeAsteroidVertices;
    vec_2 *LaserVertices;
};

struct game_permanent_memory
{
    memory_segment PermMemSegment;
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

    for (uint32_t i = 0; i < Model->NumVertices; ++i)
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

    for (uint32_t i = 0; i < Model->NumVertices - 1; ++i)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[next].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[next].Y + Object->Midpoint.Y;
        DrawLineWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
        ++cur;
        if (i < Model->NumVertices - 2) ++next;
    }
    if (Object->Type != LASER)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[0].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[0].Y + Object->Midpoint.Y;
        DrawLineWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
    }
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

void SpawnAsteroid(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, object_type AsteroidType, float X_Spawn, float Y_Spawn, float X_Mo, float Y_Mo, float AngularMomentum)
{
    if (GameState->NumSpawnedAsteroids < MAX_NUM_SPAWNED_ASTEROIDS)
    {
        game_object *NewAsteroid = &GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids];
        NewAsteroid->Type = AsteroidType;
        NewAsteroid->Model = PushToMemorySegment(MemorySegment, object_model);
        object_model *Model = NewAsteroid->Model;
        vec_2 *ResourceVertices = 0;
        if (Model)
        {
            switch (NewAsteroid->Type)
            {
                case ASTEROID_LARGE:
                {
                    Model->NumVertices = LARGE_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->LargeAsteroidVertices;
                } break;
                case ASTEROID_MEDIUM:
                {
                    Model->NumVertices = MEDIUM_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->MediumAsteroidVertices;
                } break;
                case ASTEROID_SMALL:
                {
                    Model->NumVertices = SMALL_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->SmallAsteroidVertices;
                } break;
            }

            Model->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            Model->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            GameState->NumSpawnedAsteroids += 1;
            Model->Color.Red = ASTEROID_RED;
            Model->Color.Green = ASTEROID_GREEN;
            Model->Color.Blue = ASTEROID_BLUE;
            Model->LineWidth = ASTEROID_LINE_WIDTH;
            NewAsteroid->Midpoint.X = X_Spawn;
            NewAsteroid->Midpoint.Y = Y_Spawn;
            NewAsteroid->X_Momentum = X_Mo;
            NewAsteroid->Y_Momentum = Y_Mo;
            NewAsteroid->OffsetAngle = 0.0f;
            NewAsteroid->AngularMomentum = AngularMomentum;
            NewAsteroid->IsVisible = true;
            vert_set *Verts = GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids - 1].Model->StartVerts;
            for (uint32_t i = 0; i < NewAsteroid->Model->NumVertices; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }
        }
    }
    else
    {
        // Out of space to store asteroids; handle here?
    }
}

void SpawnLaser(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object *Player)
{
    if (GameState->NumSpawnedLasers < GameState->MaxNumLasers)
    {
        uint32_t NewLaserIndex = 0;
        while (GameState->LaserSet->Lasers[NewLaserIndex].IsVisible) NewLaserIndex++;

        game_object *NewLaser = &GameState->LaserSet->Lasers[NewLaserIndex];
        NewLaser->Model = PushToMemorySegment(MemorySegment, object_model);
        object_model *Model = NewLaser->Model;
        vec_2 *ResourceVertices = Resources->LaserVertices;
        if (Model)
        {
            Model->NumVertices = LASER_NUM_VERTICES;
            Model->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            Model->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            GameState->NumSpawnedLasers += 1;
            Model->Color.Red = LASER_RED;
            Model->Color.Green = LASER_GREEN;
            Model->Color.Blue = LASER_BLUE;
            Model->LineWidth = LASER_LINE_WIDTH;

            float mag = LASER_SPEED_MAG;
            float theta = Player->OffsetAngle;
            NewLaser->OffsetAngle = theta;
            NewLaser->X_Momentum = mag * cosf(theta + (3.14159f / 2.0f));
            NewLaser->Y_Momentum = mag * sinf(theta + (3.14159f / 2.0f));
            NewLaser->AngularMomentum = 0.0f;
            NewLaser->IsVisible = true;

            vert_set *Verts = Model->StartVerts;
            for (uint32_t i = 0; i < Model->NumVertices; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }

            // Laser midpoint calculated by halving the laser model vector, then rotating that vector
            // by the offset angle, then adding those values to the Player object's midpoint.
            float X = 0.0f;
            float Y = (Resources->LaserVertices[1].Y / 2.0f);
            float X_Rot = (X * cosf(theta)) - (Y * sinf(theta));
            float Y_Rot = (X * sinf(theta)) + (Y * cosf(theta));
            NewLaser->Midpoint.X = Player->Midpoint.X + X_Rot;
            NewLaser->Midpoint.Y = Player->Midpoint.Y + Y_Rot;

            GameState->LaserSet->LifeTimers[NewLaserIndex] = 77; // enough to return to spawn position when going horizontally across the screen
        }
        else
        {
            // Error creating model; handle here
        }
    }
    else
    {
        // Can't spawn more lasers; do any necessary handling here (sound effects, etc.)
        // But likely none needed.
    }
}

void DespawnLaser(game_state *GameState, uint32_t LaserIndex)
{
    GameState->LaserSet->Lasers[LaserIndex].IsVisible = false;
    GameState->LaserSet->LifeTimers[LaserIndex] = 0;
    GameState->NumSpawnedLasers -= 1;
}

void HandleSceneEdgeWarping(game_state *GameState, int Width, int Height)
{
    HandleObjectEdgeWarping(GameState->Player, Width, Height);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        HandleObjectEdgeWarping(&GameState->SpawnedAsteroids->Asteroids[i], Width, Height);
    }
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        if (GameState->LaserSet->Lasers[i].IsVisible)
        {
            HandleObjectEdgeWarping(&GameState->LaserSet->Lasers[i], Width, Height);
        }
    }
}

void SetSceneModelsForDraw(game_state *GameState)
{
    SetObjectModelForDraw(GameState->Player);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        SetObjectModelForDraw(&GameState->SpawnedAsteroids->Asteroids[i]);
    }
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        if (GameState->LaserSet->Lasers[i].IsVisible)
        {
            SetObjectModelForDraw(&GameState->LaserSet->Lasers[i]);
        }
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
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        game_object Laser = GameState->LaserSet->Lasers[i];
        if (Laser.IsVisible) DrawObjectModelIntoBuffer(Buffer, &Laser);
    }
}

void InitializePlayer(memory_segment *MemSegment, game_state *GameState, loaded_resource_memory *Resources, float X_Coord, float Y_Coord)
{
    GameState->Player = PushToMemorySegment(&GameState->SceneMemorySegment, game_object);
    game_object *Player = GameState->Player;
    Player->Type = PLAYER;
    Player->Midpoint.X = X_Coord;
    Player->Midpoint.Y = Y_Coord;

    Player->Model = PushToMemorySegment(&GameState->SceneMemorySegment, object_model);
    object_model *P_Model = Player->Model;
    P_Model->NumVertices = PLAYER_NUM_VERTICES;
    P_Model->StartVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
    P_Model->StartVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);

    P_Model->DrawVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
    P_Model->DrawVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);

    // NOTE! These values will need to be stored in a 'resource' file -- a config text file, whatever.
    for (uint32_t i = 0; i < PLAYER_NUM_VERTICES; ++i)
    {
        SetVertValue(GameState->Player->Model->StartVerts, i, Resources->PlayerVertices[i].X, Resources->PlayerVertices[i].Y);
    }

    P_Model->LineWidth = PLAYER_LINE_WIDTH;

    P_Model->Color.Red = PLAYER_RED;
    P_Model->Color.Green = PLAYER_GREEN;
    P_Model->Color.Blue = PLAYER_BLUE;
    Player->X_Momentum = 0.0f, Player->Y_Momentum = 0.0f;
    Player->OffsetAngle = 0.0f;
    Player->AngularMomentum = PLAYER_ANGULAR_MOMENTUM;
    Player->MaxMomentum = PLAYER_MAX_MOMENTUM;
    Player->IsVisible = true;
}

void LoadResources(loaded_resource_memory *ResourceMemory)
{
    ResourceMemory->PlayerVertices = PushArrayToMemorySegment(&ResourceMemory->ResourceMemorySegment, PLAYER_NUM_VERTICES, vec_2);
    ResourceMemory->SmallAsteroidVertices = PushArrayToMemorySegment(&ResourceMemory->ResourceMemorySegment, SMALL_ASTEROID_NUM_VERTICES, vec_2);
    ResourceMemory->MediumAsteroidVertices = PushArrayToMemorySegment(&ResourceMemory->ResourceMemorySegment, MEDIUM_ASTEROID_NUM_VERTICES, vec_2);
    ResourceMemory->LargeAsteroidVertices = PushArrayToMemorySegment(&ResourceMemory->ResourceMemorySegment, LARGE_ASTEROID_NUM_VERTICES, vec_2);
    ResourceMemory->LaserVertices = PushArrayToMemorySegment(&ResourceMemory->ResourceMemorySegment, LASER_NUM_VERTICES, vec_2);

    // Player
    DWORD SizeToRead = (DWORD)(sizeof(vec_2) * PLAYER_NUM_VERTICES);
    if (!ReadFileIntoBuffer("C:/Asteroids/build/Debug/player_vertices.dat", (void *)ResourceMemory->PlayerVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Small
    SizeToRead = (DWORD)(sizeof(vec_2) * SMALL_ASTEROID_NUM_VERTICES);
    if (!ReadFileIntoBuffer("C:/Asteroids/build/Debug/sm_ast_vertices.dat", (void *)ResourceMemory->SmallAsteroidVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Medium
    SizeToRead = (DWORD)(sizeof(vec_2) * MEDIUM_ASTEROID_NUM_VERTICES);
    if (!ReadFileIntoBuffer("C:/Asteroids/build/Debug/med_ast_vertices.dat", (void *)ResourceMemory->MediumAsteroidVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Large
    SizeToRead = (DWORD)(sizeof(vec_2) * LARGE_ASTEROID_NUM_VERTICES);
    if (!ReadFileIntoBuffer("C:/Asteroids/build/Debug/lg_ast_vertices.dat", (void *)ResourceMemory->LargeAsteroidVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Laser
    SizeToRead = (DWORD)(sizeof(vec_2) * LASER_NUM_VERTICES);
    if (!ReadFileIntoBuffer("C:/Asteroids/build/Debug/laser_vertices.dat", (void *)ResourceMemory->LaserVertices, SizeToRead))
    {
        HackyAssert(false);
    }
}

void InitializeGamePermanentMemory(game_memory *Memory, game_permanent_memory *GamePermMemory, int BufferWidth, int BufferHeight)
{
    int perm_storage_struct_size = sizeof(memory_segment) + sizeof(game_state) + sizeof(loaded_resource_memory);

    BeginMemorySegment(&GamePermMemory->PermMemSegment, perm_storage_struct_size,
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_permanent_memory));

    GamePermMemory->GameState = PushToMemorySegment(&GamePermMemory->PermMemSegment, game_state);
    GamePermMemory->Resources = PushToMemorySegment(&GamePermMemory->PermMemSegment, loaded_resource_memory);

    game_state *GameState = GamePermMemory->GameState;
    BeginMemorySegment(&GameState->SceneMemorySegment, (Memory->PermanentStorageSize - perm_storage_struct_size) / 2,
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_permanent_memory) + perm_storage_struct_size);

    GameState->NumSpawnedAsteroids = 0;
    GameState->SpawnedAsteroids = PushToMemorySegment(&GameState->SceneMemorySegment, asteroid_set);
    GameState->SpawnedAsteroids->Asteroids = PushArrayToMemorySegment(&GameState->SceneMemorySegment, MAX_NUM_SPAWNED_ASTEROIDS, game_object);
    GameState->MaxNumLasers = MAX_NUM_SPAWNED_LASERS;
    GameState->NumSpawnedLasers = 0;
    GameState->LaserSet = PushToMemorySegment(&GameState->SceneMemorySegment, laser_set);
    GameState->LaserSet->Lasers = PushArrayToMemorySegment(&GameState->SceneMemorySegment, MAX_NUM_SPAWNED_LASERS, game_object);
    GameState->LaserSet->LifeTimers = PushArrayToMemorySegment(&GameState->SceneMemorySegment, MAX_NUM_SPAWNED_LASERS, uint32_t);
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        GameState->LaserSet->Lasers[i] = {};
        GameState->LaserSet->LifeTimers[i] = 0;
    }

    loaded_resource_memory *ResourceMemory = GamePermMemory->Resources;
    BeginMemorySegment(&ResourceMemory->ResourceMemorySegment, (Memory->PermanentStorageSize - perm_storage_struct_size) / 2,
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_permanent_memory) + perm_storage_struct_size + ((Memory->PermanentStorageSize - perm_storage_struct_size) / 2));
    LoadResources(ResourceMemory);

    InitializePlayer(&GameState->SceneMemorySegment, GameState, ResourceMemory, (float)(BufferWidth / 2), (float)(BufferHeight / 10));

    Memory->IsInitialized = true;
}

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *PlayerInput)
{
    game_permanent_memory *GamePermMemory = (game_permanent_memory *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        srand(GetTickCount());
        InitializeGamePermanentMemory(Memory, GamePermMemory, OffscreenBuffer->Width, OffscreenBuffer->Height);
    }

    game_state *GameState = GamePermMemory->GameState;
    loaded_resource_memory *LoadedResources = GamePermMemory->Resources;
    game_object *Player = GameState->Player;
    object_model *PlayerModel = Player->Model;

    if (PlayerInput->Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput->A_Pressed && !PlayerInput->A_Was_Pressed)
    {
        int astIndex = rand() % 3;
        int X = rand() % OffscreenBuffer->Width;
        int Y = rand() % OffscreenBuffer->Height;
        switch (astIndex)
        {
            case 0:
            {
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, LoadedResources, ASTEROID_SMALL, (float)X, (float)Y, -.25f, -1.0f, .005f);
            } break;
            case 1:
            {
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, LoadedResources, ASTEROID_MEDIUM, (float)X, (float)Y, -.25f, -1.0f, .005f);
            } break;
            case 2:
            {
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, LoadedResources, ASTEROID_LARGE, (float)X, (float)Y, -.25f, -1.0f, .005f);
            } break;
        }
    }
    if (PlayerInput->B_Pressed && !PlayerInput->B_Was_Pressed)
    {
        SpawnLaser(GameState, &GameState->SceneMemorySegment, LoadedResources, Player);
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

    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        if (GameState->LaserSet->LifeTimers[i] > 0)
        {
            GameState->LaserSet->Lasers[i].Midpoint.X += GameState->LaserSet->Lasers[i].X_Momentum;
            GameState->LaserSet->Lasers[i].Midpoint.Y += GameState->LaserSet->Lasers[i].Y_Momentum;
            GameState->LaserSet->LifeTimers[i] -= 1;
        }
        else if (GameState->LaserSet->Lasers[i].IsVisible && GameState->LaserSet->LifeTimers[i] == 0)
        {
            DespawnLaser(GameState, i);
        }
    }

    HandleSceneEdgeWarping(GameState, OffscreenBuffer->Width, OffscreenBuffer->Height);
    SetSceneModelsForDraw(GameState);
    DrawSceneModelsIntoBuffer(OffscreenBuffer, GameState);
}