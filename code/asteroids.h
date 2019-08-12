#pragma once

#include "common.h"
#include "platform.h"
#include "geometry.h"
#include "entities.h"
#include "input.h"
#include "model.h"
#include "memory.h"
#include "game_object.h"


// Simple rect struct, useful for AABBs
struct game_rect
{
    vec_2 TopLeft;
    vec_2 BotRight;
};

#define PERM_STORAGE_STRUCT_SIZE ((5 * (sizeof(memory_segment))) + (sizeof(game_state)) + (sizeof(loaded_resource_memory)))
#define LASER_POOL_SIZE (sizeof(game_entity_pool) + (MAX_LASER_COUNT * sizeof(memory_block)))
#define RESOURCE_MEMORY_SIZE Megabytes(1)
#define ASTEROID_POOL_SIZE (sizeof(game_entity_pool) + (MAX_ASTEROID_COUNT * sizeof(memory_block)))
#define SCENE_MEMORY_SIZE (GAME_PERM_MEMORY_SIZE - LASER_POOL_SIZE - RESOURCE_MEMORY_SIZE - ASTEROID_POOL_SIZE)


#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size);

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *Cur, platform_player_input *Last);

// Provide these here so other portions of the game layer don't have to talk directly to platform layer.
void RequestResourceLoad(LPCSTR FileName, void *Buffer, size_t SizeToRead);
void RequestResourceWrite(LPCSTR FileName, void *Buffer, size_t SizeToWrite);