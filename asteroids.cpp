#include <windows.h>
#include <xinput.h>


#include "platform.h"
#include "input.h"
#include "memory.h"
#include "asteroids.h"
#include "geometry.h"
#include "game_entities.h"
#include "collision.h"
#include "game_object.h"
#include "entities.h"

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
/*
void SetObjectModelForDraw(game_object *Object)
{
    polygon *Poly = Object->Model->Polygon;
    float Theta = Object->OffsetAngle;
    vec_2 *StartVerts = Poly->StartVerts->Verts;
    vec_2 *DrawVerts = Poly->DrawVerts->Verts;

    for (uint32_t i = 0; i < Poly->N; ++i)
    {
        float X_Orig = StartVerts[i].X;
        float Y_Orig = StartVerts[i].Y;
        DrawVerts[i].X = (X_Orig * cosf(Theta)) - (Y_Orig * sinf(Theta));
        DrawVerts[i].Y = (X_Orig * sinf(Theta)) + (Y_Orig * cosf(Theta));
    }
}

void DrawObjectModelIntoBuffer(platform_bitmap_buffer *Buffer, game_object *Object)
{
    if (!(Buffer->Memory)) return;

    int cur = 0, next = 1;
    vec_2 Cur, Next;
    object_model *Model = Object->Model;
    polygon *Poly = Model->Polygon;
    vert_set *DrawVerts = Poly->DrawVerts;

    for (uint32_t i = 0; i < Poly->N - 1; ++i)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[next].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[next].Y + Object->Midpoint.Y;
        DrawLineSegmentWithWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
        ++cur;
        if (i < Poly->N - 2) ++next;
    }
    if (Object->Type != LASER)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[0].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[0].Y + Object->Midpoint.Y;
        DrawLineSegmentWithWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
    }
}*/

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

void TriggerEndGame()
{
    
}

void HandleSceneEdgeWarping(game_state *GameState, int Width, int Height)
{
    HandleEntityEdgeWarping(GameState->Player, Width, Height);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        HandleEntityEdgeWarping(&GameState->SpawnedAsteroids->Asteroids[i], Width, Height);
    }
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        if (GameState->LaserSet->Lasers[i].Master->IsVisible)
        {
            HandleEntityEdgeWarping(&GameState->LaserSet->Lasers[i], Width, Height);
        }
    }
}

void SetSceneModelsForDraw(game_state *GameState)
{
    SetObjectModelForDraw(GameState->Player->Master);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        SetObjectModelForDraw(GameState->SpawnedAsteroids->Asteroids[i].Master);
    }
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        if (GameState->LaserSet->Lasers[i].Master->IsVisible)
        {
            SetObjectModelForDraw(GameState->LaserSet->Lasers[i].Master);
        }
    }
}

void DrawSceneModelsIntoBuffer(platform_bitmap_buffer *Buffer, game_state *GameState)
{
    if (GameState->Player->Master->IsVisible) DrawObjectModelIntoBuffer(Buffer, GameState->Player->Master);
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        game_object *Asteroid = GameState->SpawnedAsteroids->Asteroids[i].Master;
        if (Asteroid->IsVisible) DrawObjectModelIntoBuffer(Buffer, Asteroid);
    }
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        game_object *Laser = GameState->LaserSet->Lasers[i].Master;
        if (Laser->IsVisible) DrawObjectModelIntoBuffer(Buffer, Laser);
    }
}

void LoadResources(memory_segment *ResourceMemorySegment, loaded_resource_memory *Resources)
{
    Resources->PlayerVertices = PushArrayToMemorySegment(ResourceMemorySegment, PLAYER_NUM_VERTICES, vec_2);
    Resources->SmallAsteroidVertices = PushArrayToMemorySegment(ResourceMemorySegment, SMALL_ASTEROID_NUM_VERTICES, vec_2);
    Resources->MediumAsteroidVertices = PushArrayToMemorySegment(ResourceMemorySegment, MEDIUM_ASTEROID_NUM_VERTICES, vec_2);
    Resources->LargeAsteroidVertices = PushArrayToMemorySegment(ResourceMemorySegment, LARGE_ASTEROID_NUM_VERTICES, vec_2);
    Resources->LaserVertices = PushArrayToMemorySegment(ResourceMemorySegment, LASER_NUM_VERTICES, vec_2);

    // Player
    DWORD SizeToRead = (DWORD)(sizeof(vec_2) * PLAYER_NUM_VERTICES);
    if (!ReadFileIntoBuffer((LPCTSTR)"C:/Asteroids/build/Debug/player_vertices.dat", (void *)Resources->PlayerVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Small
    SizeToRead = (DWORD)(sizeof(vec_2) * SMALL_ASTEROID_NUM_VERTICES);
    if (!ReadFileIntoBuffer((LPCTSTR)"C:/Asteroids/build/Debug/sm_ast_vertices.dat", (void *)Resources->SmallAsteroidVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Medium
    SizeToRead = (DWORD)(sizeof(vec_2) * MEDIUM_ASTEROID_NUM_VERTICES);
    if (!ReadFileIntoBuffer((LPCTSTR)"C:/Asteroids/build/Debug/med_ast_vertices.dat", (void *)Resources->MediumAsteroidVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Large
    SizeToRead = (DWORD)(sizeof(vec_2) * LARGE_ASTEROID_NUM_VERTICES);
    if (!ReadFileIntoBuffer((LPCTSTR)"C:/Asteroids/build/Debug/lg_ast_vertices.dat", (void *)Resources->LargeAsteroidVertices, SizeToRead))
    {
        HackyAssert(false);
    }

    // Laser
    SizeToRead = (DWORD)(sizeof(vec_2) * LASER_NUM_VERTICES);
    if (!ReadFileIntoBuffer((LPCTSTR)"C:/Asteroids/build/Debug/laser_vertices.dat", (void *)Resources->LaserVertices, SizeToRead))
    {
        HackyAssert(false);
    }
}

void RequestResourceLoad(LPCSTR FileName, void *Buffer, size_t SizeToRead)
{
    DWORD SizeToRead = (DWORD)(SizeToRead);
    if (!ReadFileIntoBuffer(FileName, Buffer, SizeToRead))
    {
        HackyAssert(false);
    }    
}

void RequestResourceWrite(LPCSTR FileName, void *Buffer, size_t SizeToWrite)
{
    if (!WriteBufferIntoFile(FileName, Buffer, (DWORD)SizeToWrite))
    {
        HackyAssert(false);
    }
}

void InitializeGamePermanentMemory(game_memory *Memory, game_permanent_memory *GamePermMemory, int BufferWidth, int BufferHeight)
{
    // *******                         INITIALIZE MEMORY SEGMENTS                             ************

    // The memory segments subdivide the game permanent memory into separate silos.
    // Dynamic 'allocation' is then performed out of these silos as needed while the game runs. Having the
    // memory siloed allows for clean 'reset' of certain memory chunks when, e.g., a level transition occurs.
    // Additionally, having such silos allows for, e.g., laser memory to be handled differently from asteroid memory.
    // The various listed memory sizes are rough estimates (with the exception of PERM_STORAGE_STRUCT_SIZE),
    // and are currently overkill in most cases.
    
    // As the game loop performs a cold cast of the perm memory passed to it by the game layer (to game_permanent_memory),
    // we have to first account for the size of this struct before beginning the silo process.
    // Each memory segment requires a pointer to the enclosing segment, the size of the segment, and a pointer to the beginning
    // of the segment in OS memory. The base is simply the pointer to the start of the permanent storage passed by the platform
    // layer, cast to bytes, and the offset is tracked via memory_used.

    int memory_used = sizeof(game_permanent_memory);
    BeginMemorySegment(&GamePermMemory->PermMemSegment, PERM_STORAGE_STRUCT_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += PERM_STORAGE_STRUCT_SIZE;

    BeginMemorySegment(&GamePermMemory->AsteroidMemorySegment, ASTEROID_POOL_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += ASTEROID_POOL_SIZE;
    
    BeginMemorySegment(&GamePermMemory->ResourceMemorySegment, RESOURCE_MEMORY_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += RESOURCE_MEMORY_SIZE;

    BeginMemorySegment(&GamePermMemory->LaserMemorySegment, LASER_POOL_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += LASER_POOL_SIZE;

    BeginMemorySegment(&GamePermMemory->SceneMemorySegment, SCENE_MEMORY_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += SCENE_MEMORY_SIZE;

    // The vertex data for the game objects is requested from the platform layer here.
    GamePermMemory->Resources = PushToMemorySegment(&GamePermMemory->PermMemSegment, loaded_resource_memory);
    loaded_resource_memory *ResourceMemory = GamePermMemory->Resources;
    LoadResources(&GamePermMemory->ResourceMemorySegment, ResourceMemory);

    // Initialize the asteroid memory pool
    game_entity_pool *AsteroidPool = (game_entity_pool *)GamePermMemory->AsteroidMemorySegment.BaseStorageLocation;
    AsteroidPool->Blocks = (memory_block *)GamePermMemory->AsteroidMemorySegment.BaseStorageLocation + sizeof(game_entity_pool);
    InitializeGameEntityPool(AsteroidPool, sizeof(game_entity), MAX_ASTEROID_COUNT);

    // Initialize the laser memory pool
    game_entity_pool *LaserPool = (game_entity_pool *)GamePermMemory->LaserMemorySegment.BaseStorageLocation;
    LaserPool->Blocks = (memory_block *)GamePermMemory->LaserMemorySegment.BaseStorageLocation + sizeof(game_entity_pool);
    InitializeGameEntityPool(LaserPool, sizeof(game_entity), MAX_LASER_COUNT);



    // *******                         INITIALIZE GAME STATE                             ************ 

    // The game_state struct tracks data about the game, such as WorldWidth/Height, and game entities themselves.
    // Any allocation needed for the game state is done into the GamePermMemory->SceneMemorySegment.
    // This pattern of "pushing" a struct to a memory segment is a common pattern throughout the code.
    // N.B.: Although I conceived of the broad strokes of this system independently, credit to Casey Muratori &
    // his Handmade Hero stream for some of the ideas behind the QOL improvements I made to the system,
    // including the associated macros. The rest of the memory management code I fully conceived of myself,
    // so blame for any and all gnarly code smells and 'temporary' hacks that never get fixed rests solely with myself.

    GamePermMemory->GameState = PushToMemorySegment(&GamePermMemory->PermMemSegment, game_state);
    
    
    game_state *GameState = GamePermMemory->GameState;
    GameState->WorldWidth = BufferWidth;
    GameState->WorldHeight = BufferHeight;
    GameState->Player = PushToMemorySegment(&GamePermMemory->PermMemSegment, game_entity);
    GameState->Input = PushToMemorySegment(&GamePermMemory->PermMemSegment, asteroids_player_input);
    InitializePlayerInput(GameState->Input);
    InitializeGameEntityPool(GameState->AsteroidPool, MAX_ASTEROID_COUNT);
    InitializeGameEntityPool(GameState->LaserPool, MAX_LASER_COUNT);
    InitializeLaserTimers(&GameState->LaserTimers);    

    // The game_entity struct combines the underlying game_object itself, the 'type' of the object, a boolean stating
    // whether the object is live (for memory cleaning purposes), and the set of object clones needed for
    // collision detection -- specifically, detection at the screen borders to account for screen wrapping.
    // Creation of an entity makes use of a game_object_info struct. Said struct need not be fully detailed
    // for each object. E.g., the code to spawn a new laser entity handles, internally, the calculation of its
    // midpoint and momentum, as the code is somewhat too complex for a simple argument-wrapper object.

    game_object_info PlayerInfo = {};
    PlayerInfo.Type = PLAYER;

    // Spawn the player at the midpoint of the world space.
    PlayerInfo.Midpoint.X = (float)GameState->WorldWidth / 2.0f;
    PlayerInfo.Midpoint.Y = (float)GameState->WorldHeight / 2.0f;

    PlayerInfo.Momentum.X = 0;
    PlayerInfo.Momentum.Y = 0;
    PlayerInfo.OffsetAngle = 0.0f;

    // This concept likely needs adjusted. For lasers, this is always zero. For players, this reflects the speed at
    // which the player will rotate, but only if the rotate controls are pressed. For asteroids, this denotes
    // the actual speed at which an asteroid rotates each frame, and is different for each asteroid.
    PlayerInfo.AngularMomentum = PLAYER_ANGULAR_MOMENTUM;

    InitPlayer(GameState->Player, &PlayerInfo);

    // No asteroids are spawned at game startup, but instead at level startup, which occurs later in the process.

    GameState->MaxNumAsteroids = MAX_ASTEROID_COUNT;
    GameState->NumSpawnedAsteroids = 0;
    
    GameState->MaxNumLasers = MAX_LASER_COUNT;
    GameState->NumSpawnedLasers = 0;
    GameState->LaserSet = PushToMemorySegment(&GamePermMemory->LaserMemorySegment, laser_set);
    GameState->LaserSet->Lasers = PushArrayToMemorySegment(&GamePermMemory->LaserMemorySegment, MAX_LASER_COUNT, game_entity);
    GameState->LaserSet->LifeTimers = PushArrayToMemorySegment(&GamePermMemory->LaserMemorySegment, MAX_LASER_COUNT, uint32_t);
    
    game_object_info BlankLaserInfo = {};
    BlankLaserInfo.Type = LASER;
    BlankLaserInfo.InitVisible = false;
    InitializeLaserEntities(GameState->LaserSet->Lasers, GameState, &GamePermMemory->LaserMemorySegment, ResourceMemory, &BlankLaserInfo);
    Memory->IsInitialized = true;
}

void HandleControllerInput(game_state *GameState, game_permanent_memory *GamePermMemory, loaded_resource_memory *Resources)
{
    asteroids_player_input *Input = GameState->Input;
    if (Input->Start_DownThisFrame)
    {
        PostQuitMessage(0);
    }
    if (Input->B_DownThisFrame)
    {
        if (GameState->NumSpawnedLasers < GameState->MaxNumLasers) FireLaser(GameState, &GamePermMemory->LaserMemorySegment, Resources, GameState->Player);
    }
    
    if (Input->A_DownThisFrame)
    {
        game_object_info NewAstInfo = {};
        NewAstInfo.Midpoint.X = (float) (rand() % GameState->WorldWidth);
        NewAstInfo.Midpoint.Y = (float) (rand() % GameState->WorldHeight);
        NewAstInfo.Momentum.X = GenerateRandomFloat(-100.0f, 100.0f);
        NewAstInfo.Momentum.Y = GenerateRandomFloat(-100.0f, 100.0f);
        NewAstInfo.AngularMomentum = GenerateRandomFloat(-5.0f, 5.0f);
        
        int astIndex = rand() % 3;
        if (astIndex == 0) NewAstInfo.Type = ASTEROID_SMALL;
        else if (astIndex == 1) NewAstInfo.Type = ASTEROID_MEDIUM;
        else NewAstInfo.Type = ASTEROID_LARGE;

        InitializeAsteroidEntity(GameState, &GamePermMemory->AsteroidMemorySegment, Resources, &NewAstInfo);
    }

    vec_2 MomentumAdjustment = {};

    // Note: This procedure currently gives bad control feel; must be reworked.
    //AdjustMomentumValuesAgainstMax(Player, PlayerInput->NormalizedLX * PlayerInput->Magnitude,
    //                              PlayerInput->NormalizedLY * PlayerInput->Magnitude);
    //MomentumAdjustment = ScaleVector(Input->LeftStick.StickVector_Normalized, (Input->LeftStick.Magnitude * 0.8f));
    //UpdateGameEntityMomentumAndAngle(GameState, MomentumAdjustment, (Input->LTrigger + Input->RTrigger));    
}

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *InputThisFrame, platform_player_input *InputLastFrame)
{
    game_permanent_memory *GamePermMemory = (game_permanent_memory *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        srand(GetTickCount());
        InitializeGamePermanentMemory(Memory, GamePermMemory, OffscreenBuffer->Width, OffscreenBuffer->Height);
    }

    game_state *GameState = &GamePermMemory->GameState;

    asteroids_player_input Last_Input
    {
        GameState->Input->A_Down,
        GameState->Input->A_DownThisFrame,
        GameState->Input->B_Down,
        GameState->Input->B_DownThisFrame,
        GameState->Input->Start_Down,
        GameState->Input->Start_DownThisFrame,
        GameState->Input->LTrigger,
        GameState->Input->RTrigger,
        GameState->Input->LeftStick,
        GameState->Input->LeftStickHistory
    };

    TranslatePlatformInputToGame(GameState->Input, InputThisFrame, &Last_Input);

    HandleControllerInput(GameState, GamePermMemory, LoadedResources);

    ProcessEntitiesForFrame(GameState, GameState->Input);
    
    /*
    // Collision handling
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        game_entity *CurAsteroid = &GameState->SpawnedAsteroids->Asteroids[i];
        if (CheckCollisionEntities(PlayerEntity, CurAsteroid))
        {
            HandleCollision(GameState->Player, CurAsteroid, GameState, LoadedResources, GamePermMemory, 0);
        }
    }

    Player->Midpoint = PlayerDesiredEnd;
    object_clone *PlayerClones = GameState->Player->CloneSet->Clones;
    for (int i = 0; i < NUM_OBJ_CLONES; ++i)
    {
        PlayerClones[i].ClonedObject->Midpoint = AddVectors(PlayerClones[i].ClonedObject->Midpoint, PlayerClones[i].ClonedObject->Momentum);
    }

    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        Asteroids[i].Master->Midpoint = AddVectors(Asteroids[i].Master->Midpoint, Asteroids[i].Master->Momentum);
        object_clone *CloneSet = Asteroids[i].CloneSet->Clones;
        for (int j = 0; j < NUM_OBJ_CLONES; ++j)
        {
            CloneSet[j].ClonedObject->Midpoint = AddVectors(CloneSet[j].ClonedObject->Midpoint, CloneSet[j].ClonedObject->Momentum);
        }
    }

    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        game_entity *CurrentLaser = &GameState->LaserSet->Lasers[i];
        if (GameState->LaserSet->LifeTimers[i] > 0)
        {    
            CurrentLaser->Master->Midpoint = AddVectors(CurrentLaser->Master->Midpoint, CurrentLaser->Master->Momentum);
            for (uint32_t j = 0; j < NUM_OBJ_CLONES; ++j)
            {
                game_object *Clone = CurrentLaser->CloneSet->Clones[j].ClonedObject;
                Clone->Midpoint = AddVectors(Clone->Midpoint, Clone->Momentum);
            }
            GameState->LaserSet->LifeTimers[i] -= 1;
        }
        else if (CurrentLaser->Master->IsVisible && GameState->LaserSet->LifeTimers[i] == 0)
        {
            KillLaser(GameState, CurrentLaser);
        }

        for (uint32_t a = 0; a < GameState->NumSpawnedAsteroids; ++a)
        {
            game_entity *CurrentAsteroid = &GameState->SpawnedAsteroids->Asteroids[a];
            if (CheckCollisionEntities(CurrentLaser, CurrentAsteroid))
            {
                HandleCollision(CurrentLaser, CurrentAsteroid, GameState, LoadedResources, GamePermMemory, i);
            }
        }
    }

    HandleSceneEdgeWarping(GameState, OffscreenBuffer->Width, OffscreenBuffer->Height);
    SetSceneModelsForDraw(GameState);
    DrawSceneModelsIntoBuffer(OffscreenBuffer, GameState);
    */
}