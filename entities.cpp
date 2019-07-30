#include "asteroids.h"
#include "entities.h"
#include "common.h"
#include "memory.h"
#include "geometry.h"
#include "model.h"
#include "input.h"

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
    }

    Clones[0].Offset.X = -WorldWidth;
    Clones[0].Offset.Y = -WorldHeight;

    Clones[1].Offset.X = -WorldWidth;
    Clones[1].Offset.Y = 0;

    Clones[2].Offset.X = -WorldWidth;
    Clones[2].Offset.Y = WorldHeight;

    Clones[3].Offset.X = 0;
    Clones[3].Offset.Y = WorldHeight;

    Clones[4].Offset.X = WorldWidth;
    Clones[4].Offset.Y = WorldHeight;

    Clones[5].Offset.X = WorldWidth;
    Clones[5].Offset.Y = 0;

    Clones[6].Offset.X = WorldWidth;
    Clones[6].Offset.Y = -WorldHeight;

    Clones[7].Offset.X = 0;
    Clones[7].Offset.Y = -WorldHeight;
}

game_entity * SpawnNonPlayerEntity(game_entity_pool *Pool, game_object_info *ObjInfoStruct, uint32_t WorldWidth, uint32_t WorldHeight)
{
    game_entity *New = AllocateEntity(Pool);
    New->EntityType = ObjInfoStruct->Type;
    
    game_object *Master = &New->Master;
    Master->Type = ObjInfoStruct->Type;
    Master->Midpoint = ObjInfoStruct->Midpoint;
    Master->Momentum = ObjInfoStruct->Momentum;
    Master->AngularMomentum = ObjInfoStruct->AngularMomentum;
    Master->OffsetAngle = ObjInfoStruct->OffsetAngle;
    InitObjectModel(New);

    InitObjectClones(New, WorldWidth, WorldHeight);
    return New;
}

game_entity * SpawnAsteroid(game_state *GameState, game_object_info *ObjInfoStruct)
{
    return SpawnNonPlayerEntity(GameState->AsteroidPool, ObjInfoStruct, GameState->WorldWidth, GameState->WorldHeight);
}

asteroid_split_results DemoteAsteroid(game_entity *Asteroid)
{
    asteroid_split_results results = {};

    if (Asteroid->EntityType == SMALL_ASTEROID)
    {
        KillAsteroid(Asteroid);
        results.WasSplit = 0;
        return results;
    }
    else
    {
        return SplitAsteroid(Asteroid);
    }
}

asteroid_split_results SplitAsteroid(game_entity *Asteroid)
{
    HackyAssert(!(Asteroid->EntityType == SMALL_ASTEROID));
    HackyAssert(!(Asteroid->Pool->Blocks[Asteroid->BlockIndex].IsFree));

    asteroid_split_results results = {};
    // This asteroid was split into two instead of simply killed.
    results.WasSplit = 1;

    // Copy fields over from current asteroid so we can free its space up before
    // trying to allocate new asteroids; otherwise we would artificially be restricting
    // the # of possible asteroids to MAX_ASTEROID_COUNT - 1 upon asteroid splits.
    game_entity OldAsteroid = *Asteroid;

    KillAsteroid(Asteroid);

    // Prepare two new asteroid objects.
    game_object_info Asteroid_A_Info, Asteroid_B_Info = {};
    object_type NewAsteroidType = (OldAsteroid.EntityType == LARGE_ASTEROID) ? MEDIUM_ASTEROID : SMALL_ASTEROID;
    object_type Type;
    vec_2 Midpoint;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;

    Asteroid_A_Info.Type = NewAsteroidType;
    Asteroid_A_Info.Midpoint = OldAsteroid.Master.Midpoint;
    Asteroid_A_Info.OffsetAngle = OldAsteroid.Master.OffsetAngle;
    Asteroid_A_Info.AngularMomentum = OldAsteroid.Master.AngularMomentum + GenerateRandomFloat(-10.0f, 10.0f);
    
    Asteroid_A_Info.Momentum = Perpendicularize(OldAsteroid.Master.Momentum);
    Asteroid_A_Info.Momentum.X += GenerateRandomFloat(-20.0f, 20.0f);
    Asteroid_A_Info.Momentum.Y += GenerateRandomFloat(-20.0f, 20.0f);

    results.A = SpawnNonPlayerEntity(OldAsteroid.Pool, &Asteroid_A_Info);

    Asteroid_B_Info.Type = NewAsteroidType;
    Asteroid_B_Info.Midpoint = OldAsteroid.Master.Midpoint;
    Asteroid_B_Info.OffsetAngle = OldAsteroid.Master.OffsetAngle;
    Asteroid_B_Info.AngularMomentum = OldAsteroid.Master.AngularMomentum + GenerateRandomFloat(-10.0f, 10.0f);

    Asteroid_B_Info.Momentum = Perpendicularize(Perpendicularize(Perpendicularize(OldAsteroid.Master.Momentum)));
    Asteroid_B_Info.Momentum.X += GenerateRandomFloat(-20.0f, 20.0f);
    Asteroid_B_Info.Momentum.Y += GenerateRandomFloat(-20.0f, 20.0f);

    results.B = SpawnNonPlayerEntity(OldAsteroid.Pool, &Asteroid_B_Info);

    return results;
}

void KillAsteroid(game_entity *Asteroid)
{
    FreeEntity(Asteroid);  
}

void InitPlayer(game_entity *PlayerEntity, game_object_info *ObjInfo)
{
    game_object *Player = &PlayerEntity->Master;
    Player->Type = PLAYER;
    Player->Midpoint.X = ObjInfo->Midpoint.X;
    Player->Midpoint.Y = ObjInfo->Midpoint.Y;

    Player->Model = PushToMemorySegment(MemorySegment, object_model);
    object_model *P_Model = Player->Model;
    P_Model->Polygon = PushToMemorySegment(MemorySegment, polygon);
    polygon *P_Poly = P_Model->Polygon;

    P_Poly->N = PLAYER_NUM_VERTICES;
    P_Poly->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
    P_Poly->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, PLAYER_NUM_VERTICES, vec_2);

    P_Poly->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
    P_Poly->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, PLAYER_NUM_VERTICES, vec_2);

    // NOTE! These values will need to be stored in a 'resource' file -- a config text file, whatever.
    for (uint32_t i = 0; i < P_Poly->N; ++i)
    {
        SetVertValue(P_Poly->StartVerts, i, Resources->PlayerVertices[i].X, Resources->PlayerVertices[i].Y);
    }

    P_Model->LineWidth = PLAYER_LINE_WIDTH;

    P_Model->Color.Red = PLAYER_RED;
    P_Model->Color.Green = PLAYER_GREEN;
    P_Model->Color.Blue = PLAYER_BLUE;
    Player->Momentum.X = ObjInfo->Momentum.X;
    Player->Momentum.Y = ObjInfo->Momentum.Y;
    Player->OffsetAngle = ObjInfo->OffsetAngle;
    Player->AngularMomentum = ObjInfo->AngularMomentum;
    Player->IsVisible = ObjInfo->InitVisible;
    Player->Radius = CalculateMaxObjectRadius(Player);

    return Player;
}

void InitializeLaserTimers(laser_timing *Timers)
{
    for (uint32_t i = 0; i < MAX_LASER_COUNT; i++)
    {
        Timers->Timers[i] = 0;    
    }
}

void SpawnLaser(game_entity_pool *Pool, game_object_info *ObjInfoStruct, laser_timing *Timers)
{
    game_entity *New = SpawnNonPlayerEntity(Pool, ObjInfoStruct);
    Timers->Timers[New->BlockIndex] = Timers->InitialValue;
}

void KillLaser(game_entity *Laser, laser_timing *Timers)
{
    Timers->Timers[Laser->BlockIndex] = 0;
    FreeEntity(Laser);
}

void TickLaserTimers(game_entity_pool *Lasers, laser_timing *Timers)
{
    for (uint32_t i = 0; i < MAX_LASER_COUNT; i++)
    {
        Timers->Timers[i] -= 1;
        if (Timers->Timers[i] == 0)
        {
            KillLaser(&Lasers->Blocks[i].Memory, Timers);
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
    game_object *Player = *GameState->Player->Master;
    game_object_info LaserInfo = {};

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

    UpdateClones(LaserInfo.tity, GameState->WorldWidth, GameState->WorldHeight);
    SpawnLaser(GameState->LaserPool, &LaserInfo, &GameState->LaserTimers);    
}

void TickPlayerEntity(game_state *GameState, asteroids_player_input *Input)
{
    // State changes (not incl. collisions), translation/rotation
    if (Input->A_DownThisFrame)
    {
        FireLaser(GameState);
    }
    TickPlayerObject(&GameState->Player->Master, Input);

    RecalcClonePos(Entity);
}

void TickLaserEntity(game_entity *Entity, laser_timing *Timers)
{
    TickLaserObject(&Entity->Master, Timers);
    RecalcClonePos(Entity);
}

void TickAsteroidEntity(game_entity *Entity)
{
    TickAsteroidObject(&Entity->Master);
    RecalcClonePos(Entity);
}

void TickAllEntities(game_state *GameState, asteroids_player_input *Input)
{
    // Handle player first
    TickPlayerEntity(GameState, Input);
    HandleEntityEdgeWarping(GameState->Player, GameState->WorldWidth, GameState->WorldHeight);

    // Now lasers
    laser_timing *Timers = &GameState->LaserTimers;
    for (uint32_t i = 0; i < GameState->LaserPool->PoolInfo.BlockCount; i++)
    {
        memory_block *Cur = &GameState->LaserPool->Blocks[i];
        if (!Cur->IsFree)
        {
            TickLaserEntity(&Cur->Memory, Timers);
            HandleEntityEdgeWarping(&Cur->Memory, GameState->WorldWidth, GameState->WorldHeight);
        }
    }

    // Now asteroids
    for (uint32_t i = 0; i < GameState->AsteroidPool->PoolInfo.BlockCount; i++)
    {
        memory_block *Cur = &GameState->AsteroidPool->Blocks[i];
        if (!Cur->IsFree)
        {
            TickAsteroidEntity(&Cur->Memory);
            HandleEntityEdgeWarping(&Cur->Memory, GameState->WorldWidth, GameState->WorldHeight);
        }
    }
}

void CollideAllEntities(game_state *GameState)
{

}

void DrawAllEntities(game_state *GameState)
{

}

void ProcessEntitiesForFrame(game_state *GameState, asteroids_player_input *Input)
{
    TickAllEntities(GameState, Input);
    CollideAllEntities(GameState);
    DrawAllEntities(GameState);

    // We only tick the timers at the end of the entity update and render process. This ensures that lasers that are on
    // their final tick will survive long enough to be used in the collision process, etc.
    TickLaserTimers(GameState->LaserPool, &GameState->LaserTimers);

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