#pragma once

#include "asteroids.h"
#include "entities.h"
#include "common.h"
#include "memory.h"
#include "geometry.h"
#include "model.h"
#include "input.h"
#include "render.h"
#include "collision.h"
#include "level_management.h"

void InitObjectClones(game_entity *Entity, uint32_t WorldWidth, uint32_t WorldHeight)
{
    /* There are eight sectors plus the center, so eight clones. The clones are ordered from
     * lower left, clockwise. To obtain a clone's position, we simply add or subtract the 
     * world width or height (or both) as necessary. Unfortunately, there's no real way to 
     * abstract this process into a loop, so this function ends up looking a little messy.
     */
    object_clone *Clones = Entity->CloneSet;
    
    for (uint32_t i = 0; i < NUM_OBJECT_CLONES; i++)
    {
        Clones[i].Master = &Entity->Master;
        Clones[i].Polygon.N = Entity->Master.Model.Polygon.N;
        Clones[i].Polygon.C = Entity->Master.Model.Polygon.C;
    }

    Clones[0].Offset.X = -(float)WorldWidth;
    Clones[0].Offset.Y = -(float)WorldHeight;

    Clones[1].Offset.X = -(float)WorldWidth;
    Clones[1].Offset.Y = 0.0f;

    Clones[2].Offset.X = -(float)WorldWidth;
    Clones[2].Offset.Y = (float)WorldHeight;

    Clones[3].Offset.X = 0.0f;
    Clones[3].Offset.Y = (float)WorldHeight;

    Clones[4].Offset.X = (float)WorldWidth;
    Clones[4].Offset.Y = (float)WorldHeight;

    Clones[5].Offset.X = (float)WorldWidth;
    Clones[5].Offset.Y = 0.0f;

    Clones[6].Offset.X = (float)WorldWidth;
    Clones[6].Offset.Y = -(float)WorldHeight;

    Clones[7].Offset.X = 0.0f;
    Clones[7].Offset.Y = -(float)WorldHeight;
}

game_entity * SpawnNonPlayerEntity(game_entity_pool *Pool, game_object_info *ObjInfoStruct)
{
    game_entity *New = AllocateEntity(Pool);
    New->SpawnedThisFrame = true;
    New->EntityType = ObjInfoStruct->Type;
    
    game_object *Master = &New->Master;
    Master->Type = ObjInfoStruct->Type;
    Master->Midpoint = ObjInfoStruct->Midpoint;
    Master->Momentum = ObjInfoStruct->Momentum;
    Master->AngularMomentum = ObjInfoStruct->AngularMomentum;
    Master->OffsetAngle = ObjInfoStruct->OffsetAngle;
    InitObjectModel(New);

    InitObjectClones(New, ObjInfoStruct->WorldWidth, ObjInfoStruct->WorldHeight);
    return New;
}

game_entity * SpawnAsteroid(game_state *GameState, game_object_info *ObjInfoStruct)
{
    return SpawnNonPlayerEntity(GameState->AsteroidPool, ObjInfoStruct);
}


void RandomizeAsteroidLocationMomentum(game_object_info *Params)
{
    Params->Midpoint.X = (float) (GenerateRandomUnsignedIntFromZeroTo(Params->WorldWidth));
    Params->Midpoint.Y = (float) (GenerateRandomUnsignedIntFromZeroTo(Params->WorldHeight));
    Params->Momentum.X = GenerateRandomFloat(ASTEROID_V_RANGE_MIN, ASTEROID_V_RANGE_MAX);
    Params->Momentum.Y = GenerateRandomFloat(ASTEROID_V_RANGE_MIN, ASTEROID_V_RANGE_MAX);
    Params->AngularMomentum = GenerateRandomFloat(ASTEROID_ROT_RANGE_MIN, ASTEROID_ROT_RANGE_MAX);
}

asteroid_demote_results SplitAsteroid(game_entity *Asteroid, game_state *GameState)
{
    HackyAssert(!(Asteroid->EntityType == SMALL_ASTEROID));
    HackyAssert(!(Asteroid->Pool->Blocks[Asteroid->BlockIndex].IsFree));

    asteroid_demote_results results = {};
    // This asteroid was split into two instead of simply killed.
    results.WasKilled = false;

    // Copy fields over from current asteroid so we can free its space up before
    // trying to allocate new asteroids; otherwise we would artificially be restricting
    // the # of possible asteroids to MAX_ASTEROID_COUNT - 1 upon asteroid splits.
    game_entity OldAsteroid = *Asteroid;

    KillAsteroid(Asteroid);

    // Prepare two new asteroid objects.
    game_object_info Asteroid_A_Info, Asteroid_B_Info = {};
    object_type NewAsteroidType = (OldAsteroid.EntityType == LARGE_ASTEROID) ? MEDIUM_ASTEROID : SMALL_ASTEROID;

    Asteroid_A_Info.Type = NewAsteroidType;
    Asteroid_A_Info.Midpoint = OldAsteroid.Master.Midpoint;
    Asteroid_A_Info.OffsetAngle = OldAsteroid.Master.OffsetAngle;
    Asteroid_A_Info.AngularMomentum = OldAsteroid.Master.AngularMomentum + GenerateRandomFloat((ASTEROID_ROT_RANGE_MIN / 10.0f), (ASTEROID_ROT_RANGE_MAX / 10.0f));
    
    Asteroid_A_Info.Momentum = Perpendicularize(OldAsteroid.Master.Momentum);
    Asteroid_A_Info.Momentum.X += GenerateRandomFloat((ASTEROID_V_RANGE_MIN / 10.0f), (ASTEROID_V_RANGE_MAX / 10.0f));
    Asteroid_A_Info.Momentum.X += GenerateRandomFloat((ASTEROID_V_RANGE_MIN / 10.0f), (ASTEROID_V_RANGE_MAX / 10.0f));

    Asteroid_A_Info.WorldHeight = GameState->WorldHeight;
    Asteroid_A_Info.WorldWidth = GameState->WorldWidth;

    results.A = SpawnNonPlayerEntity(OldAsteroid.Pool, &Asteroid_A_Info);

    Asteroid_B_Info.Type = NewAsteroidType;
    Asteroid_B_Info.Midpoint = OldAsteroid.Master.Midpoint;
    Asteroid_B_Info.OffsetAngle = OldAsteroid.Master.OffsetAngle;
    Asteroid_A_Info.AngularMomentum = OldAsteroid.Master.AngularMomentum + GenerateRandomFloat((ASTEROID_ROT_RANGE_MIN / 10.0f), (ASTEROID_ROT_RANGE_MAX / 10.0f));

    Asteroid_B_Info.WorldHeight = GameState->WorldHeight;
    Asteroid_B_Info.WorldWidth = GameState->WorldWidth;

    Asteroid_B_Info.Momentum = Perpendicularize(Perpendicularize(Perpendicularize(OldAsteroid.Master.Momentum)));
    Asteroid_B_Info.Momentum.X += GenerateRandomFloat((ASTEROID_V_RANGE_MIN / 10.0f), (ASTEROID_V_RANGE_MAX / 10.0f));
    Asteroid_B_Info.Momentum.X += GenerateRandomFloat((ASTEROID_V_RANGE_MIN / 10.0f), (ASTEROID_V_RANGE_MAX / 10.0f));

    results.B = SpawnNonPlayerEntity(OldAsteroid.Pool, &Asteroid_B_Info);

    return results;
}

void KillAsteroid(game_entity *Asteroid)
{
    FreeEntity(Asteroid);  
}

asteroid_demote_results DemoteAsteroid(game_entity *Asteroid, game_state *GameState)
{
    asteroid_demote_results results = {};

    if (Asteroid->EntityType == SMALL_ASTEROID)
    {
        KillAsteroid(Asteroid);
        results.WasKilled = true;
        return results;
    }
    else
    {
        return SplitAsteroid(Asteroid, GameState);
    }
}

void SpawnPlayer(game_state *GameState)
{
    game_entity *PlayerEntity = GameState->Player;
    PlayerEntity->EntityType = PLAYER;
    PlayerEntity->SpawnedThisFrame = false; // notion of first-frame CD avoidance doesn't apply to player entity
    PlayerEntity->BlockIndex = 0;
    PlayerEntity->Pool = 0;

    game_object *Player = &PlayerEntity->Master;
    Player->Type = PLAYER;
    Player->Midpoint.X = GameState->WorldCenter.X;
    Player->Midpoint.Y = GameState->WorldCenter.Y;

    Player->Momentum.X = 0;
    Player->Momentum.Y = 0;

    Player->OffsetAngle = 0;
    Player->AngularMomentum = PLAYER_ANGULAR_MOMENTUM;

    GameState->PlayerInfo.IFrames = PLAYER_INIT_IFRAMES;
    GameState->PlayerInfo.IsLive = true;

    InitObjectModel(PlayerEntity);
    InitObjectClones(PlayerEntity, GameState->WorldWidth, GameState->WorldHeight);
}

void InitPlayer(game_state *GameState)
{
    SpawnPlayer(GameState);

    GameState->PlayerInfo.Kills = 0;
    GameState->PlayerInfo.Lives = PLAYER_NUM_LIVES;
    GameState->PlayerInfo.Score = 0;
}

void InitializeLaserTimers(laser_timing *Timers)
{
    Timers->InitialValue = LASER_SPAWN_TIMER;
    for (uint32_t i = 0; i < MAX_LASER_COUNT; i++)
    {
        Timers->Timers[i] = 0;    
    }
}

void SpawnLaser(game_state *GameState, game_object_info *ObjInfoStruct)
{
    if (GameState->PlayerInfo.NumLiveLasers == MAX_LASER_COUNT)
    {
        return;
    }

    game_entity *New = SpawnNonPlayerEntity(GameState->LaserPool, ObjInfoStruct);
    laser_timing *Timers = &GameState->LaserTimers;
    Timers->Timers[New->BlockIndex] = Timers->InitialValue;
    GameState->PlayerInfo.NumLiveLasers++;
}

void KillLaser(game_state *GameState, game_entity *Laser)
{
    GameState->LaserTimers.Timers[Laser->BlockIndex] = 0;
    FreeEntity(Laser);
    GameState->PlayerInfo.NumLiveLasers--;
}

void TickLaserTimers(game_state *GameState)
{
    game_entity_pool *Lasers = GameState->LaserPool;
    laser_timing *Timers = &GameState->LaserTimers;
    for (uint32_t i = 0; i < MAX_LASER_COUNT; i++)
    {
        if (!GameState->LaserPool->Blocks[i].IsFree)
        {
            Timers->Timers[i] -= 1;
            if (Timers->Timers[i] == 0)
            {
                KillLaser(GameState, &Lasers->Blocks[i].Entity);
            }
        }
    }
}

void RecalcClonePos(game_entity *Entity)
{
    object_clone *Clones = Entity->CloneSet;
    for (uint32_t i = 0; i < NUM_OBJECT_CLONES; i++)
    {
        object_clone *Current = &Clones[i];
        Current->Polygon.C = AddVectors(Current->Master->Midpoint, Current->Offset);
        for (uint32_t j = 0; j < Current->Polygon.N; j++)
        {
            Current->Polygon.Vertices[j] = AddVectors(Current->Master->Model.Polygon.Vertices[j], Current->Offset);
        }
    }
}

void FireLaser(game_state *GameState)
{
    game_object *Player = &GameState->Player->Master;
    game_object_info LaserInfo = {};
    LaserInfo.Type = LASER;

    // Laser shares the same offset angle as player's ship.
    float theta = Player->OffsetAngle;
    LaserInfo.AngularMomentum = theta;
    LaserInfo.OffsetAngle = Player->OffsetAngle;

    // Appropriately decompose the laser's speed into its vector components, but first rotate by 90deg
    LaserInfo.Momentum.X = LASER_SPEED * cosf(LaserInfo.OffsetAngle + (3.14159f / 2.0f));
    LaserInfo.Momentum.Y = LASER_SPEED * sinf(LaserInfo.OffsetAngle + (3.14159f / 2.0f));
    
    // Laser midpoint calculated by halving the laser model vector, then rotating that vector
    // by the offset angle, then adding those values to the Player object's midpoint.
    float X = 0.0f;
    float Y = (GameState->ReferenceModelPolygons.Laser.BaseVertices[1].Y / 2.0f);

    float X_Rot = (X * cosf(theta)) - (Y * sinf(theta));
    float Y_Rot = (X * sinf(theta)) + (Y * cosf(theta));

    LaserInfo.Midpoint.X = Player->Midpoint.X + X_Rot;
    LaserInfo.Midpoint.Y = Player->Midpoint.Y + Y_Rot;

    LaserInfo.WorldHeight = GameState->WorldHeight;
    LaserInfo.WorldWidth = GameState->WorldWidth;

    SpawnLaser(GameState, &LaserInfo);    
}

void TickPlayerEntity(game_state *GameState, asteroids_player_input *Input)
{
    // State changes (not incl. collisions), translation/rotation
    if (Input->A_DownThisFrame)
    {
        FireLaser(GameState);
    }
    
    TickPlayerObject(GameState, Input);
    
    if (GameState->PlayerInfo.IFrames > 0)
    {
        GameState->PlayerInfo.IFrames--;
    }
    
    RecalcClonePos(GameState->Player);
}

void TickLaserEntity(game_entity *Entity)
{
    Entity->SpawnedThisFrame = false;
    TickLaserObject(&Entity->Master);
    RecalcClonePos(Entity);
}

void TickAsteroidEntity(game_entity *Entity)
{
    Entity->SpawnedThisFrame = false;
    TickAsteroidObject(&Entity->Master);
    RecalcClonePos(Entity);
}

void HandleEntityEdgeWarping(game_entity *Entity, int ScreenWidth, int ScreenHeight)
{
    game_object *Master = &Entity->Master;
    object_clone *Clones = Entity->CloneSet;
    if (Master->Midpoint.X < 0)
    {
        Master->Midpoint.X += ScreenWidth;
        Master->Model.Polygon.C.X += ScreenWidth;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].Polygon.C.X += ScreenWidth;
        }
    }
    else if (Master->Midpoint.X >= ScreenWidth)
    {
        Master->Midpoint.X -= ScreenWidth;
        Master->Model.Polygon.C.X -= ScreenWidth;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].Polygon.C.X -= ScreenWidth;
        }
    }
    if (Master->Midpoint.Y < 0)
    {
        Master->Midpoint.Y += ScreenHeight;
        Master->Model.Polygon.C.Y += ScreenHeight;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].Polygon.C.Y += ScreenHeight;
        }
    }
    else if (Master->Midpoint.Y >= ScreenHeight)
    {
        Master->Midpoint.Y -= ScreenHeight;
        Master->Model.Polygon.C.Y -= ScreenHeight;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].Polygon.C.Y -= ScreenHeight;
        }
    }
}

void TickAllEntities(game_state *GameState, asteroids_player_input *Input)
{
    // Handle player first
    TickPlayerEntity(GameState, Input);

    // Now lasers
    laser_timing *Timers = &GameState->LaserTimers;
    for (uint32_t i = 0; i < GameState->LaserPool->PoolInfo.BlockCount; i++)
    {
        memory_block *Cur = &GameState->LaserPool->Blocks[i];
        if (!Cur->IsFree)
        {
            TickLaserEntity(&Cur->Entity);
            HandleEntityEdgeWarping(&Cur->Entity, GameState->WorldWidth, GameState->WorldHeight);
        }
    }

    // Now asteroids
    for (uint32_t i = 0; i < GameState->AsteroidPool->PoolInfo.BlockCount; i++)
    {
        memory_block *Cur = &GameState->AsteroidPool->Blocks[i];
        if (!Cur->IsFree)
        {
            TickAsteroidEntity(&Cur->Entity);
            HandleEntityEdgeWarping(&Cur->Entity, GameState->WorldWidth, GameState->WorldHeight);
        }
    }

    HandleEntityEdgeWarping(GameState->Player, GameState->WorldWidth, GameState->WorldHeight);
}

// Player can only collide with asteroids
// Asteroids can only collide with lasers
void CollideAllEntities(game_state *GameState)
{
    game_entity *Player = GameState->Player;
    memory_block *AsteroidBlocks = GameState->AsteroidPool->Blocks;
    memory_block *LaserBlocks = GameState->LaserPool->Blocks;

    for (uint32_t i = 0; i < GameState->AsteroidPool->PoolInfo.BlockCount; i++)
    {
        game_entity *CurAsteroid = &AsteroidBlocks[i].Entity;
        
        // As part of the collision process, asteroids that are hit with a laser will get demoted
        // to two new, smaller asteroids. These will take up spots in the pool but should 
        if (CurAsteroid->SpawnedThisFrame)
        {
            continue;
        }

        if (GameState->PlayerInfo.IFrames == 0 && CollideObjectWithEntity(&Player->Master, CurAsteroid))
        {
            HandlePlayerKilled(GameState);
            return;
        }


    }

    if (GameState->PlayerInfo.IFrames == 0)
    {
        for (uint32_t i = 0; i < GameState->AsteroidPool->PoolInfo.BlockCount; i++)
        {
            if (!AsteroidBlocks[i].IsFree && !AsteroidBlocks[i].Entity.SpawnedThisFrame)
            {
                if (CollideObjectWithEntity(&Player->Master, &AsteroidBlocks[i].Entity))
                {
                    HandlePlayerKilled(GameState);
                    return; // player only needs to die once, no need to check more lasers, etc.
                }

            }
        }
    }
    for (uint32_t i = 0; i < GameState->LaserPool->PoolInfo.BlockCount; i++)
    {
        if (!LaserBlocks[i].IsFree)
        {
            for (uint32_t j = 0; j < GameState->AsteroidPool->PoolInfo.BlockCount; j++)
            {
                if (!AsteroidBlocks[j].IsFree && !AsteroidBlocks[j].Entity.SpawnedThisFrame)
                {
                    if (CollideObjectWithEntity(&LaserBlocks[i].Entity.Master, &AsteroidBlocks[j].Entity))
                    {
                        asteroid_demote_results Results = DemoteAsteroid(&AsteroidBlocks[j].Entity, GameState);
                        LvlAsteroidWasKilled(GameState, &Results);

                        KillLaser(GameState, &LaserBlocks[i].Entity);
                        break;
                    }
                }
            }
        }
    }
}

void ProcessEntitiesForFrame(game_state *GameState, asteroids_player_input *Input, platform_bitmap_buffer *OffscreenBuffer)
{
    if (GameState->LevelInfo.CurrentLevel == 0)
    {
        level_info *Info = &GameState->LevelInfo;
        Info->KillScoreMultiplier = 10;
        Info->LevelsPerSpeedIncrementSegment = 5;
        Info->MeanTimeBetweenSpawns = 120;
        Info->MeanTimeBetweenSpawnDecrement = 20;
        Info->ScoreForOneUp = 1000;
        Info->ScorePerShot = 1;
        Info->NumLargeAsteroids = 0;
        Info->AsteroidIncrementPerLevel = 2;
        
        StartNextLevel(GameState);
    }
    
    TickAllEntities(GameState, Input);
    CollideAllEntities(GameState);
    RenderAllEntities(GameState, OffscreenBuffer);

    // We only tick the timers at the end of the entity update and render process. This ensures that lasers that are on
    // their final tick will survive long enough to be used in the collision process, etc.
    TickLaserTimers(GameState);
}