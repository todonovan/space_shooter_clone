#include <windows.h>
#include <xinput.h>

#include "asteroids.h"
#include "memory.h"

#define UPDATE_MODELS

void BeginMemorySegment(memory_segment *Segment, uint32_t Size, uint8_t *Storage)
{
    Segment->Size = Size;
    Segment->BaseStorageLocation = Storage;
    Segment->Used = 0;
}

void RequestResourceLoad(LPCSTR FileName, void *Buffer, size_t SizeToRead)
{
    if (!ReadFileIntoBuffer(FileName, Buffer, (DWORD)SizeToRead))
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
#ifdef UPDATE_MODELS
            // Translate vectors to file for storage, run this to change shape of objects' models.
    object_model Player, SmallAst, MediumAst, LargeAst, Laser = {};


    Player.Polygon.BaseVertices[0].X = -20.0f;
    Player.Polygon.BaseVertices[0].Y = -20.0f;
    Player.Polygon.BaseVertices[1].X = 0.0f;
    Player.Polygon.BaseVertices[1].Y = 40.0f;
    Player.Polygon.BaseVertices[2].X = 20.0f;
    Player.Polygon.BaseVertices[2].Y = -20.0f;
    Player.Polygon.BaseVertices[3].X = 0.0f;
    Player.Polygon.BaseVertices[3].Y = 0.0f;

    SmallAst.Polygon.BaseVertices[0].X = -10.0f;
    SmallAst.Polygon.BaseVertices[0].Y = -5.0f;
    SmallAst.Polygon.BaseVertices[1].X = -12.0f;
    SmallAst.Polygon.BaseVertices[1].Y = 8.0f;
    SmallAst.Polygon.BaseVertices[2].X = 0.0f;
    SmallAst.Polygon.BaseVertices[2].Y = 12.0f;
    SmallAst.Polygon.BaseVertices[3].X = 10.0f;
    SmallAst.Polygon.BaseVertices[3].Y = 6.0f;
    SmallAst.Polygon.BaseVertices[4].X = 5.0f;
    SmallAst.Polygon.BaseVertices[4].Y = -4.0f;

    MediumAst.Polygon.BaseVertices[0].X = -20.0f;
    MediumAst.Polygon.BaseVertices[0].Y = -10.0f;
    MediumAst.Polygon.BaseVertices[1].X = -24.0f;
    MediumAst.Polygon.BaseVertices[1].Y = 16.0f;
    MediumAst.Polygon.BaseVertices[2].X = -5.0f;
    MediumAst.Polygon.BaseVertices[2].Y = 25.0f;
    MediumAst.Polygon.BaseVertices[3].X = 12.0f;
    MediumAst.Polygon.BaseVertices[3].Y = 16.0f;
    MediumAst.Polygon.BaseVertices[4].X = 10.0f;
    MediumAst.Polygon.BaseVertices[4].Y = -8.0f;
    MediumAst.Polygon.BaseVertices[5].X = 0.0f;
    MediumAst.Polygon.BaseVertices[5].Y = -15.0f;

    LargeAst.Polygon.BaseVertices[0].X = -8.0f;
    LargeAst.Polygon.BaseVertices[0].Y = -30.0f;
    LargeAst.Polygon.BaseVertices[1].X = -40.0f;
    LargeAst.Polygon.BaseVertices[1].Y = -20.0f;
    LargeAst.Polygon.BaseVertices[2].X = -50.0f;
    LargeAst.Polygon.BaseVertices[2].Y = 16.0f;
    LargeAst.Polygon.BaseVertices[3].X = -35.0f;
    LargeAst.Polygon.BaseVertices[3].Y = 45.0f;
    LargeAst.Polygon.BaseVertices[4].X = 0.0f;
    LargeAst.Polygon.BaseVertices[4].Y = 50.0f;
    LargeAst.Polygon.BaseVertices[5].X = 30.0f;
    LargeAst.Polygon.BaseVertices[5].Y = 40.0f;
    LargeAst.Polygon.BaseVertices[6].X = 40.0f;
    LargeAst.Polygon.BaseVertices[6].Y = 0.0f;
    LargeAst.Polygon.BaseVertices[7].X = 20.0f;
    LargeAst.Polygon.BaseVertices[7].Y = -20.0f;

    Laser.Polygon.BaseVertices[0].X = 0.0f;
    Laser.Polygon.BaseVertices[0].Y = 0.0f;
    Laser.Polygon.BaseVertices[1].X = 0.0f;
    Laser.Polygon.BaseVertices[1].Y = 40.0f;

    RequestResourceWrite((LPCSTR)"C:/space_shooter_clone/build/Debug/player_vertices.dat", Player.Polygon.BaseVertices, PLAYER_NUM_VERTICES * (sizeof(vec_2)));
    RequestResourceWrite((LPCSTR)"C:/space_shooter_clone/build/Debug/small_ast_vertices.dat", SmallAst.Polygon.BaseVertices, SMALL_ASTEROID_NUM_VERTICES * (sizeof(vec_2)));
    RequestResourceWrite((LPCSTR)"C:/space_shooter_clone/build/Debug/med_ast_vertices.dat", MediumAst.Polygon.BaseVertices, MEDIUM_ASTEROID_NUM_VERTICES * (sizeof(vec_2)));
    RequestResourceWrite((LPCSTR)"C:/space_shooter_clone/build/Debug/lg_ast_vertices.dat", LargeAst.Polygon.BaseVertices, LARGE_ASTEROID_NUM_VERTICES * (sizeof(vec_2)));
    RequestResourceWrite((LPCSTR)"C:/space_shooter_clone/build/Debug/laser_vertices.dat", Laser.Polygon.BaseVertices, LASER_NUM_VERTICES * (sizeof(vec_2)));

#endif
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

    BeginMemorySegment(&GamePermMemory->LaserMemorySegment, LASER_POOL_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += LASER_POOL_SIZE;

    BeginMemorySegment(&GamePermMemory->SceneMemorySegment, SCENE_MEMORY_SIZE, (uint8_t *)Memory->PermanentStorage + memory_used);
    memory_used += SCENE_MEMORY_SIZE;




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
    // Initialize the asteroid memory pool
    GameState->AsteroidPool = (game_entity_pool *)GamePermMemory->AsteroidMemorySegment.BaseStorageLocation;
    InitializeGameEntityPool(GameState->AsteroidPool, MAX_ASTEROID_COUNT);

    // Initialize the laser memory pool
    GameState->LaserPool = (game_entity_pool *)GamePermMemory->LaserMemorySegment.BaseStorageLocation;
    InitializeGameEntityPool(GameState->LaserPool, MAX_LASER_COUNT);
    InitializePlayerInput(GameState->Input);
    InitializeGameEntityPool(GameState->AsteroidPool, MAX_ASTEROID_COUNT);
    InitializeGameEntityPool(GameState->LaserPool, MAX_LASER_COUNT);
    InitializeLaserTimers(&GameState->LaserTimers);
    LoadReferencePolygons(GameState);

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

    InitPlayer(GameState, &PlayerInfo);
    
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

    ProcessEntitiesForFrame(GameState, GameState->Input, OffscreenBuffer);
    
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