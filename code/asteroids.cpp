#include <windows.h>
#include <xinput.h>


#include "asteroids.h"
#include "platform.h"
#include "input.h"
#include "memory.h"
#include "geometry.h"
#include "entities.h"
#include "game_object.h"
#include "collision.h"
#include "common.cpp"
#include "input.cpp"
#include "memory.cpp"
#include "geometry.cpp"
#include "collision.cpp"
#include "game_object.cpp"
#include "entities.cpp"
#include "model.cpp"
#include "render.cpp"


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
    InitializeGameEntityPool(AsteroidPool, MAX_ASTEROID_COUNT);

    // Initialize the laser memory pool
    game_entity_pool *LaserPool = (game_entity_pool *)GamePermMemory->LaserMemorySegment.BaseStorageLocation;
    LaserPool->Blocks = (memory_block *)GamePermMemory->LaserMemorySegment.BaseStorageLocation + sizeof(game_entity_pool);
    InitializeGameEntityPool(LaserPool, MAX_LASER_COUNT);



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
    GameState->WorldCenter.X = (float)BufferWidth / 2.0f;
    GameState->WorldCenter.Y = (float)BufferHeight / 2.0f;
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
    PlayerInfo.Midpoint = GameState->WorldCenter;

    PlayerInfo.Momentum.X = 0;
    PlayerInfo.Momentum.Y = 0;
    PlayerInfo.OffsetAngle = 0.0f;

    // This concept likely needs adjusted. For lasers, this is always zero. For players, this reflects the speed at
    // which the player will rotate, but only if the rotate controls are pressed. For asteroids, this denotes
    // the actual speed at which an asteroid rotates each frame, and is different for each asteroid.
    PlayerInfo.AngularMomentum = PLAYER_ANGULAR_MOMENTUM;

    InitPlayer(GameState->Player, &PlayerInfo);
    
    Memory->IsInitialized = true;
}

void HandleControllerInput(game_state *GameState)
{
    asteroids_player_input *Input = GameState->Input;
    if (Input->Start_DownThisFrame)
    {
        PostQuitMessage(0);
    }
}

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *InputThisFrame, platform_player_input *InputLastFrame)
{
    game_permanent_memory *GamePermMemory = (game_permanent_memory *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        srand(GetTickCount());
        InitializeGamePermanentMemory(Memory, GamePermMemory, OffscreenBuffer->Width, OffscreenBuffer->Height);
    }

    game_state *GameState = GamePermMemory->GameState;

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

    HandleControllerInput(GameState);

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