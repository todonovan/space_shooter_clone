#pragma once

#include "memory.h"
#include "asteroids.h"
#include "common.h"

void InitializeMemoryBlock(memory_block *Block)
{
    Block->IsFree = true;
    Block->Entity = {};
}

void InitializeGameEntityPool(game_entity_pool *EntityPool, uint32_t BlockCount)
{
    EntityPool->PoolInfo.ItemSize = sizeof(game_entity);
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
            Blocks[i].Entity.BlockIndex = i;
            Blocks[i].Entity.Pool = Pool; // Allows other funcs to only need entity ref, no need to track pool ref
            Pool->PoolInfo.NumOccupied++;
            return &Blocks[i].Entity;
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

void ClearPool(game_entity_pool *Pool)
{
    for (uint32_t i = 0; i < Pool->PoolInfo.BlockCount; i++)
    {
        Pool->Blocks[i].IsFree = true;
    }
}

void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size)
{
    void *Result = Segment->BaseStorageLocation + Segment->Used;
    Segment->Used += Size;
    return Result;
}