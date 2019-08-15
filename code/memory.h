#pragma once

// Forward-declarations
#include "memory.fwd.h"
#include "common.fwd.h"
#include "entities.fwd.h"
#include "level_management.fwd.h"
#include "geometry.fwd.h"
#include "input.fwd.h"
#include "model.fwd.h"
#include "game_object.fwd.h"

#include "common.h"
#include "entities.h"
#include "level_management.h"
#include "geometry.h"
#include "input.h"
#include "model.h"
#include "game_object.h"

#define MAX_BLOCK_COUNT 300

#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))

struct memory_block
{
    bool IsFree;
    game_entity Entity;
};

struct memory_pool_info
{
    uint32_t ItemSize;
    uint32_t BlockSize;
    uint32_t BlockCount;
    uint32_t SizeInBytes;
    uint32_t NumOccupied;
};

struct game_entity_pool
{
    memory_pool_info PoolInfo;
    memory_block Blocks[MAX_BLOCK_COUNT];
};

struct game_state
{
    game_entity *Player;
    player_info PlayerInfo;
    level_info LevelInfo;
    uint32_t WorldWidth;
    uint32_t WorldHeight;
    vec_2 WorldCenter;
    asteroids_player_input *Input;
    game_entity_pool *AsteroidPool;
    game_entity_pool *LaserPool;
    laser_timing LaserTimers;
    reference_model_polygons ReferenceModelPolygons;
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

struct game_permanent_memory
{
    memory_segment PermMemSegment;
    game_state *GameState;

    memory_segment SceneMemorySegment;        
    memory_segment LaserMemorySegment;
    memory_segment AsteroidMemorySegment;
};

void InitializeMemoryBlock(memory_block *Block);
void InitializeGameEntityPool(game_entity_pool *EntityPool, uint32_t BlockCount);
game_entity * AllocateEntity(game_entity_pool *Pool);
void FreeEntity(game_entity *Entity);
void ClearPool(game_entity_pool *Pool);
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size);