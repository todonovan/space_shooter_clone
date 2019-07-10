#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
#include "entities.h"

struct memory_block
{
    bool IsFree;
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
    memory_block *Blocks;
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

void InitializeMemoryBlock(memory_block *Block);
void InitializeGameEntityPool(game_entity_pool *EntityPool, uint32_t EntitySize, uint32_t BlockCount);
game_entity * AllocateEntity(game_entity_pool *Pool);
void FreeEntity(game_entity *Entity);

#endif