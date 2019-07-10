#ifndef ASTEROIDS_H
#define ASTEROIDS_H


#include "common.h"
#include "platform.h"
#include "geometry.h"
#include "entities.h"
#include "input.cpp"

#define PLAYER_NUM_VERTICES 4
#define PLAYER_LINE_WIDTH 2.0f
#define PLAYER_RED 200
#define PLAYER_GREEN 200
#define PLAYER_BLUE 200
#define PLAYER_ANGULAR_MOMENTUM 0.05f
#define PLAYER_MAX_MOMENTUM 30.0f

#define LARGE_ASTEROID_NUM_VERTICES 8
#define MEDIUM_ASTEROID_NUM_VERTICES 6
#define SMALL_ASTEROID_NUM_VERTICES 5

#define ASTEROID_LINE_WIDTH 1.5f
#define ASTEROID_RED 125
#define ASTEROID_GREEN 125
#define ASTEROID_BLUE 125

#define LASER_LINE_WIDTH 1.0f
#define LASER_NUM_VERTICES 2
#define LASER_RED 163
#define LASER_GREEN 42
#define LASER_BLUE 21

#define LASER_SPEED_MAG 25.0f
#define LASER_SPAWN_TIMER 77


// Simple rect struct, useful for AABBs
struct game_rect
{
    vec_2 TopLeft;
    vec_2 BotRight;
};

struct asteroid_set
{
    game_entity *Asteroids;
};

struct laser_set
{
    game_entity *Lasers;
    uint32_t *LifeTimers;
};

struct game_state
{
    int WorldWidth;
    int WorldHeight;
    asteroids_player_input *Input;
    uint32_t EntityCount;   // this gives us the next usable index when spawning new entities
    game_entity *Player;
    uint32_t MaxNumAsteroids;
    uint32_t NumSpawnedAsteroids;
    asteroid_set *SpawnedAsteroids;
    uint32_t MaxNumLasers;
    uint32_t NumSpawnedLasers;
    laser_set *LaserSet;
};

struct loaded_resource_memory
{
    vec_2 *PlayerVertices;
    vec_2 *SmallAsteroidVertices;
    vec_2 *MediumAsteroidVertices;
    vec_2 *LargeAsteroidVertices;
    vec_2 *LaserVertices;
};

struct game_permanent_memory
{
    memory_segment PermMemSegment;
    game_state *GameState;
    loaded_resource_memory *Resources;
    memory_segment ResourceMemorySegment;
    memory_segment SceneMemorySegment;        
    memory_segment LaserMemorySegment;
    memory_segment AsteroidMemorySegment;
};

#define PERM_STORAGE_STRUCT_SIZE ((5 * (sizeof(memory_segment))) + (sizeof(game_state)) + (sizeof(loaded_resource_memory)))
#define LASER_POOL_SIZE (sizeof(game_entity_pool) + (MAX_LASER_COUNT * sizeof(memory_block)))
#define RESOURCE_MEMORY_SIZE Megabytes(1)
#define ASTEROID_POOL_SIZE (sizeof(game_entity_pool) + (MAX_ASTEROID_COUNT * sizeof(memory_block)))
#define SCENE_MEMORY_SIZE ((GAME_PERM_MEMORY_SIZE - LASER_POOL_SIZE - RESOURCE_MEMORY_SIZE - ASTEROID_POOL_SIZE))


#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size);

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *Cur, platform_player_input *Last);

#endif // asteroids.h