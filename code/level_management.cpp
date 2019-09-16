#pragma once

#include "level_management.h"
#include "entities.h"
#include "geometry.h"
#include "collision.h"
#include "entities.h"

void InitLevelManager(level_info *LevelInfo)
{
    LevelInfo->CurrentLevel = 0;
    LevelInfo->LastOneUpThreshold = 0;
}

void TriggerGameEnd(game_state *GameState)
{
    // obviously this needs to change!!!
    PostQuitMessage(0);
}

void EndLevel(game_state *GameState)
{

}

void AddAsteroidToLevel(game_state *GameState)
{
    // Care has to be taken to avoid spawning an asteroid directly on top of the player!
    // Naive approach currently is to spawn an asteroid, and perform an AABB/AABB, but with the
    // asteroid's AABB scaled by some amount to also avoid super-near-misses. If this  
    // returns a hit, then simply move the asteroid by the same scaled height and width of its AABB away from player.

    // When we get really fancy, one idea might be to spawn the asteroid wherever, but flash its image
    // for some X number of frames before truly spawning it, allowing the player to avoid it before its
    // CD system turns on.

    game_object_info AsteroidInfo = {};

    AsteroidInfo.Type = LARGE_ASTEROID;
    AsteroidInfo.WorldHeight = GameState->WorldHeight;
    AsteroidInfo.WorldWidth = GameState->WorldWidth;

    RandomizeAsteroidLocationMomentum(&AsteroidInfo);

    game_entity *NewAsteroid = SpawnAsteroid(GameState, &AsteroidInfo);

    ConstructAABB(&NewAsteroid->Master.Model.Polygon);
    AABB ScaledAABB = {};
    ScaledAABB.Lengths = ScaleVector(NewAsteroid->Master.Model.Polygon.BoundingBox.Lengths, 2.0f);
    ScaledAABB.Min = NewAsteroid->Master.Model.Polygon.BoundingBox.Min;
    if (AABBAABB(GameState->Player->Master.Model.Polygon.BoundingBox, ScaledAABB))
    {
        TranslateVector(&NewAsteroid->Master.Midpoint, ScaledAABB.Lengths);
        NewAsteroid->Master.Model.Polygon.C = NewAsteroid->Master.Midpoint;
    }

    GameState->LevelInfo.NumLargeAsteroidsSpawned++;
}

void LvlAsteroidWasKilled(game_state *GameState, asteroid_demote_results *DemoteResults)
{
    level_info *Info = &GameState->LevelInfo;
    if (DemoteResults->WasKilled)
    {
        Info->NumSmallAsteroidsKilled++;
        GameState->PlayerInfo.Kills++;
        GameState->PlayerInfo.Score += Info->ScorePerShot * Info->KillScoreMultiplier;
    }
    else
    {
        GameState->PlayerInfo.Score += Info->ScorePerShot;
    }
}

// This uses an algorithm from Donald Knuth for finding next arrival via a
// Poisson distribution. Thanks, Don!
uint32_t CalculateTimeToNextSpawn(uint32_t MeanArrivalRate)
{
    double L = exp(-((double)MeanArrivalRate));
    uint32_t k = 0;
    double p = 1.0;

    do
    {
        k++;
        double rand = GenerateRandomDouble(0.0, 1.0);
        p *= rand;
    } while (p > L);

    return k - 1;
}

void StartNextLevel(game_state *GameState)
{
    GameState->PlayerInfo.IsLive = true;
    GameState->PlayerInfo.NumLiveLasers = 0;
    level_info *Lvl = &GameState->LevelInfo;

    Lvl->CurrentLevel++;
    Lvl->NumLargeAsteroids += Lvl->AsteroidIncrementPerLevel;
    Lvl->NumLargeAsteroidsSpawned = 0;
    Lvl->NumSmallAsteroidsKilled = 0;
    Lvl->TimeSinceAsteroidSpawn = 0;
    if (Lvl->CurrentLevel % Lvl->LevelsPerSpeedIncrementSegment == 0)
    {
        Lvl->Speed += Lvl->SpeedIncrementPerSegment;
    }

    Lvl->MeanTimeBetweenSpawns -= Lvl->MeanTimeBetweenSpawnDecrement;
    if (Lvl->MeanTimeBetweenSpawns < 0)
    {
        Lvl->MeanTimeBetweenSpawns = 0;
    }

    ClearPool(GameState->AsteroidPool);
    ClearPool(GameState->LaserPool);
    InitializeLaserTimers(&GameState->LaserTimers);

    SpawnPlayer(GameState);
}

void HandlePlayerKilled(game_state *GameState)
{
    GameState->PlayerInfo.IsLive = false;
    GameState->PlayerInfo.Lives--;
    if (GameState->PlayerInfo.Lives == 0)
    {
        TriggerGameEnd(GameState);
    }
    else
    {
        SpawnPlayer(GameState);
    }
}

void TickLevelManagement(game_state *GameState)
{
    level_info *Info = &GameState->LevelInfo;
    
    // Does player get extra life?
    if (GameState->PlayerInfo.Score >= (Info->LastOneUpThreshold + Info->ScoreForOneUp))
    {
        GameState->PlayerInfo.Lives++;
        Info->LastOneUpThreshold += Info->ScoreForOneUp;
    }

    // Did player win level? Check if all asteroids are dead...
    if (Info->NumSmallAsteroidsKilled == Info->NumLargeAsteroids * 4)
    {
        StartNextLevel(GameState);
    }

    // Spawn new asteroid if appropriate
    if ((Info->TimeSinceAsteroidSpawn == Info->TimeToNextSpawn) && Info->NumLargeAsteroidsSpawned != Info->NumLargeAsteroids)
    {
        AddAsteroidToLevel(GameState);
        Info->TimeSinceAsteroidSpawn = 0;
        Info->TimeToNextSpawn = CalculateTimeToNextSpawn(Info->MeanTimeBetweenSpawns);
    }

    Info->TimeSinceAsteroidSpawn++;
}
