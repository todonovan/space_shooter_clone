#ifndef ENTITIES_H
#define ENTITIES_H

#include "common.h"
#include "asteroids.h"
#include "game_object.h"
#include "model.h"

#define NUM_OBJECT_CLONES 8
#define MAX_ASTEROID_COUNT 300
#define MAX_LASER_COUNT 10
#define MAX_ENTITY_COUNT (MAX_LASER_COUNT + MAX_ASTEROID_COUNT + 1)
#define LASER_SPEED 25.0f
#define LASER_SPAWN_TIMER 77

struct game_entity
{
    uint32_t BlockIndex;
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
    bool32_t IsLive;
    uint32_t Score;
    uint32_t Lives;
    uint32_t Kills;
};

// The return values from calling 'demote asteroid' function.
// WasSplit is nonzero if a split occurred, and zero if the asteroid
// was instead killed and no split resulted.
struct asteroid_split_results
{
    bool32_t WasSplit;
    game_entity *A;
    game_entity *B;
};

game_entity * SpawnAsteroid(game_state *GameState, game_object_info *NewAsteroidInfo);
asteroid_split_results DemoteAsteroid(game_entity *Asteroid);

// KillAsteroid is made available for end-of-level/player death cleanup purposes. Do not call
// this directly otherwise (i.e., while handling asteroid-laser collisions)
void KillAsteroid(game_entity *Asteroid);

void InitPlayer(game_entity *Player, game_object_info *PlayerInfo);

void InitializeLaserTimers(laser_timing *Timers);
void SpawnLaser(game_entity_pool *Pool, game_object_info *NewLaserInfo, laser_timing *Timers);
void KillLaser(game_entity *Laser, laser_timing *Timers);

void ProcessEntitiesForFrame(game_state *GameState, asteroids_player_input *Input);

/*
void TickAllEntities(game_state *GameState, asteroids_player_input *Input);
void CollideAllEntities(game_state *GameState);
void HandleEntityEdgeWarping(game_entity *Entity, int ScreenWidth, int ScreenHeight);
*/
#endif