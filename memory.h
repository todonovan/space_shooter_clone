#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
#include "entities.h"

#define MAX_BLOCK_COUNT 300

struct memory_block
{
    bool32_t IsFree;
    game_entity Memory;
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
    uint32_t WorldWidth;
    uint32_t WorldHeight;
    asteroids_player_input *Input;
    game_entity *Player;
    player_info PlayerInfo;
    uint32_t CurrentLevel;
    game_entity_pool *AsteroidPool;
    game_entity_pool *LaserPool;
    laser_timing LaserTimers;
    reference_model_polygons ReferenceModelPolygons;
};

struct game_memory
{
    bool32_t IsInitialized;
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

    memory_segment ResourceMemorySegment;
    memory_segment SceneMemorySegment;        
    memory_segment LaserMemorySegment;
    memory_segment AsteroidMemorySegment;
};

void InitializeMemoryBlock(memory_block *Block);
void InitializeGameEntityPool(game_entity_pool *EntityPool, uint32_t BlockCount);
game_entity * AllocateEntity(game_entity_pool *Pool);
void FreeEntity(game_entity *Entity);

#endif