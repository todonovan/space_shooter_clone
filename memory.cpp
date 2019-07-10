#include "asteroids.h"
#include "common.h"
#include "memory.h"

void InitializeMemoryBlock(memory_block *Block)
{
    Block->IsFree = true;
    Block->Memory = {};
}

void InitializeGameEntityPool(game_entity_pool *EntityPool, uint32_t EntitySize, uint32_t BlockCount)
{
    EntityPool->PoolInfo.ItemSize = EntitySize;
    EntityPool->PoolInfo.BlockSize = sizeof(memory_block);
    EntityPool->PoolInfo.NumOccupied = 0;
    EntityPool->PoolInfo.BlockCount = BlockCount;
    EntityPool->PoolInfo.SizeInBytes = sizeof(memory_block) * BlockCount;

    for (uint32_t i = 0; i < BlockCount; i++)
    {
        InitializeMemoryBlock(&EntityPool->Blocks[i]);
    }
}

// Returns NULL if no available space
game_entity * AllocateEntity(game_entity_pool *Pool)
{
    if (Pool->PoolInfo.NumOccupied == Pool->PoolInfo.BlockCount)
    {
        return NULL;
    }

    memory_block *Blocks = Pool->Blocks;
    for (uint32_t i = 0; i < Pool->PoolInfo.BlockCount; i++)
    {
        if (Blocks[i].IsFree)
        {
            Blocks[i].IsFree = false;
            Blocks[i].Memory.BlockIndex = i;
            Blocks[i].Memory.Pool = Pool; // Allows other funcs to only need entity ref, no need to track pool ref
            Pool->PoolInfo.NumOccupied++;
            return &Blocks[i].Memory;
        }
    }

    return NULL;
}

void FreeEntity(game_entity *Entity)
{
    HackyAssert(Entity->Pool->PoolInfo.NumOccupied > 0);
    HackyAssert(!(Entity->Pool->Blocks[Entity->BlockIndex].IsFree));

    game_entity_pool *Pool = Entity->Pool;
    Pool->Blocks[Entity->BlockIndex].IsFree = true;
    Pool->PoolInfo.NumOccupied--;
    Entity = NULL;
}