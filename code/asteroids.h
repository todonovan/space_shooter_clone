#pragma once

#include "geometry.h"
#include "memory.h"
#include "platform.h"

// Simple rect struct, useful for AABBs
struct game_rect
{
    vec_2 TopLeft;
    vec_2 BotRight;
};

#define PERM_STORAGE_STRUCT_SIZE ((100 * (sizeof(memory_segment))) + (sizeof(game_state)))
#define LASER_POOL_SIZE (sizeof(game_entity_pool) + (MAX_LASER_COUNT * sizeof(memory_block)))
#define RESOURCE_MEMORY_SIZE Megabytes(1)
#define ASTEROID_POOL_SIZE (sizeof(game_entity_pool) + (MAX_ASTEROID_COUNT * sizeof(memory_block)))
#define SCENE_MEMORY_SIZE (GAME_PERM_MEMORY_SIZE - LASER_POOL_SIZE - RESOURCE_MEMORY_SIZE - ASTEROID_POOL_SIZE)

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *Cur, platform_player_input *Last);

// Provide these here so other portions of the game layer don't have to talk directly to platform layer.
void RequestResourceLoad(LPCSTR FileName, void *Buffer, size_t SizeToRead);
void RequestResourceWrite(LPCSTR FileName, void *Buffer, size_t SizeToWrite);