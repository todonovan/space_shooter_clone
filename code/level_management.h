#pragma once

// Forward-declarations
#include "level_management.fwd.h"
#include "common.fwd.h"
#include "memory.fwd.h"

#include "common.h"

/* Levels are structured thusly:
      - Each level has a total number of asteroids that will spawn
      - These all spawn as large asteroids
      - Once all the asteroids have been reduced to smalls and killed, the level advances
      - Each level has a 'speed' value, which acts as a scalar for scaling asteroid move speed
      - This speed value increases for each level.
      - Player gets a certain amount of score for each hit, and a multiple of that score for each kill
      - Player gets an extra life after each N points
      - Asteroids are not all spawned at the start. Instead, they're spawned based upon a 'mean 
        time to next arrival' stochatic algorithm. This mean time can be tweaked for 
        implementing difficulty levels, or simply increased a certain amount each level, or each N levels
      - One idea is to break game into segments, say 5 levels. Each level in segment, # of asteroids increases
        and mean spawn time decreases but speed stays the same. At the start of the next segment,
        the speed increases, while the # of asteroids decrease and the mean spawn time increases.
        *10 ast -> 13 ast -> 16 ast -> 19 -> 22 ast -> [speed up] 13 ast -> 16 ast -> ......
 */


struct level_info
{
    uint32_t CurrentLevel;
    uint32_t NumLargeAsteroids;
    uint32_t NumLargeAsteroidsSpawned;
    uint32_t NumSmallAsteroidsKilled;
    // currently linear, might mess later with finding ways to make this 'graph' have
    // a custom, non-linear shape. Exponential difficulty? Etc.
    uint32_t AsteroidIncrementPerLevel;
    uint32_t Speed;
    uint32_t SpeedIncrementPerSegment;
    uint32_t LevelsPerSpeedIncrementSegment;
    uint32_t ScorePerShot;
    uint32_t KillScoreMultiplier;
    uint32_t ScoreForOneUp;
    uint32_t LastOneUpThreshold;
    uint32_t TimeSinceAsteroidSpawn;
    uint32_t TimeToNextSpawn;
    uint32_t MeanTimeBetweenSpawns;
    // Same as above, find ways to make this nonlinear if requested.
    uint32_t MeanTimeBetweenSpawnIncrement;
};

void InitLevelManager(level_info *LevelInfo);
void StartNextLevel(game_state *GameState);
void HandlePlayerKilled(game_state *GameState);

// Every frame, check whether the level-end conditions have been met, and update player scores/lives.
void TickLevelManagement(game_state *GameState);

// This function is provided publicly to allow for menu commands (e.g., quit to menu) to gracefully
// handle level end -- e.g., perhaps allowing for "save and quit," or updating top scores, etc., and
// to then trigger the (eventual) menu system.
void EndLevel(game_state *GameState);
