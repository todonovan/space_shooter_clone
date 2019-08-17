#pragma once

// Forward-declarations
#include "entities.fwd.h"
#include "common.fwd.h"
#include "game_object.fwd.h"
#include "memory.fwd.h"

#include "common.h"
#include "platform.h"
#include "game_object.h"

#define NUM_OBJECT_CLONES 8
#define MAX_ASTEROID_COUNT 300
#define MAX_LASER_COUNT 10
#define MAX_ENTITY_COUNT (MAX_LASER_COUNT + MAX_ASTEROID_COUNT + 1)
#define LASER_SPEED 25.0f
#define LASER_SPAWN_TIMER 77
#define PLAYER_NUM_LIVES 5 // this is temporary clearly
#define PLAYER_INIT_IFRAMES 120 // 2 secs of invincibility on spawn
#define ASTEROID_V_RANGE_MAX 0.00005f
#define ASTEROID_V_RANGE_MIN -ASTEROID_V_RANGE_MAX
#define ASTEROID_ROT_RANGE_MAX 0.005f
#define ASTEROID_ROT_RANGE_MIN -ASTEROID_ROT_RANGE_MAX


struct game_entity
{
    uint32_t BlockIndex;
    // ensures new asteroids spawned during CD/CR aren't inadvertently checked for subsequent collisions that frame
    bool SpawnedThisFrame;
    game_entity_pool *Pool;
    object_type EntityType;
    game_object Master;
    object_clone CloneSet[NUM_OBJECT_CLONES];
};

struct laser_timing
{
    uint32_t InitialValue;
    uint32_t Timers[MAX_LASER_COUNT];
};

struct player_info
{
    bool IsLive;
    uint32_t Score;
    uint32_t Lives;
    uint32_t Kills;
    uint32_t IFrames;
    uint32_t NumLiveLasers;
};

// The return values from calling 'demote asteroid' function.
// WasKilled is zero if a split occurred, and nonzero if the asteroid
// was instead killed and no split resulted.
struct asteroid_demote_results
{
    bool WasKilled;
    game_entity *A;
    game_entity *B;
};

game_entity * SpawnAsteroid(game_state *GameState, game_object_info *NewAsteroidInfo);
asteroid_demote_results DemoteAsteroid(game_entity *Asteroid, game_state *GameState);
void RandomizeAsteroidLocationMomentum(game_object_info *Params);
// KillAsteroid is made available for end-of-level/player death cleanup purposes. Do not call
// this directly otherwise (i.e., while handling asteroid-laser collisions)
void KillAsteroid(game_entity *Asteroid);

void SpawnPlayer(game_state *GameState);
void InitPlayer(game_state *GameState);

void InitializeLaserTimers(laser_timing *Timers);
void FireLaser(game_state *GameState);
void KillLaser(game_state *GameState, game_entity *Laser);

void ProcessEntitiesForFrame(game_state *GameState, asteroids_player_input *Input, platform_bitmap_buffer *OffscreenBuffer);