#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>
#include <stdlib.h>

#include "platform.h"
#include "asteroids.h"
#include "game_entities.h"
#include "collision.h"

#include "game_entities.cpp"
#include "collision.cpp"

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
void DrawLineSegmentWithWidth(platform_bitmap_buffer *Buffer, vec_2 *StartPoint, vec_2 *EndPoint, color_triple *Color, float LineWidth)
{
    int x0 = (int)(StartPoint->X);
    int x1 = (int)(EndPoint->X);
    int y0 = (int)(StartPoint->Y);
    int y1 = (int)(EndPoint->Y);
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
        DrawLineSegmentWithWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
        ++cur;
        if (i < Model->NumVertices - 2) ++next;
    }
    if (Object->Type != LASER)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[0].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[0].Y + Object->Midpoint.Y;
        DrawLineSegmentWithWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
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

inline float CalculateVectorDistance(vec_2 P1, vec_2 P2)
{
    float d = sqrtf(((P2.X - P1.X) * (P2.X - P1.X)) + ((P2.Y - P1.Y) * (P2.Y - P1.Y)));
    return d;
}

line_segment * MakeLineSegment(vec_2 *Start, vec_2 *End)
{
    line_segment *segment = PushToMemorySegment();
}

void TriggerEndGame()
{
    
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

    InitializePlayer(GameState, &GameState->SceneMemorySegment, ResourceMemory, (float)(BufferWidth / 2), (float)(BufferHeight / 10));

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

    vec_2 PlayerDesiredEnd = {};
    PlayerDesiredEnd.X = Player->Midpoint.X + Player->X_Momentum;
    PlayerDesiredEnd.Y = Player->Midpoint.Y + Player->Y_Momentum;

    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        game_object *CurAsteroid = &GameState->SpawnedAsteroids->Asteroids[i];
        bool collision = CheckCollision(PlayerDesiredEnd, Player->Radius, CurAsteroid->Midpoint, CurAsteroid->Radius);

        if (collision && CurAsteroid->IsVisible)
        {
            HandleCollision(GameState, LoadedResources, Player, CurAsteroid);
        }
    }

    Player->Midpoint.X = PlayerDesiredEnd.X;
    Player->Midpoint.Y = PlayerDesiredEnd.Y;

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

        for (uint32_t a = 0; a < GameState->NumSpawnedAsteroids; ++a)
        {
            game_object *CurAsteroid = &GameState->SpawnedAsteroids->Asteroids[a];
            bool collision = CheckCollision(GameState->LaserSet->Lasers[i].Midpoint, GameState->LaserSet->Lasers[i].Radius, CurAsteroid->Midpoint, CurAsteroid->Radius);
            if (collision && CurAsteroid->IsVisible && GameState->LaserSet->Lasers[i].IsVisible)
            {
                HandleCollision(GameState, LoadedResources, &GameState->LaserSet->Lasers[i], CurAsteroid, i);
            }
        }
    }

    HandleSceneEdgeWarping(GameState, OffscreenBuffer->Width, OffscreenBuffer->Height);
    SetSceneModelsForDraw(GameState);
    DrawSceneModelsIntoBuffer(OffscreenBuffer, GameState);
}