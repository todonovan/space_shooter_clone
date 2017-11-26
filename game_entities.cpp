#include <stdint.h>
#include <math.h>

#include "platform.h"
#include "asteroids.h"
#include "geometry.h"
#include "game_entities.h"
#include "collision.h"

inline void SetVertValue(vec_2 *Vert, float XVal, float YVal)
{
    Vert->X = XVal;
    Vert->Y = YVal;
}

inline void SetVertValue(vert_set *VertSet, uint32_t VertIndex, float XVal, float YVal)
{
    VertSet->Verts[VertIndex].X = XVal;
    VertSet->Verts[VertIndex].Y = YVal;
}

float CalculateMaxObjectRadius(game_object *Object)
{
    polygon *Poly = Object->Model->Polygon;
    vec_2 *Verts = Poly->StartVerts->Verts;
    float max_radius = 0.0f, current_radius = 0.0f;
    vec_2 Zero = {};
    for (uint32_t i = 0; i < Poly->N; ++i)
    {
        current_radius = CalculateVectorDistance(Zero, Verts[i]);
        if (current_radius > max_radius) max_radius = current_radius;
    }
    return max_radius;
}

clone_set * CreateClones(memory_segment *MemorySegment, game_entity *Entity, int ScreenWidth, int ScreenHeight)
{
    clone_set *CloneSet = PushToMemorySegment(MemorySegment, clone_set);
    CloneSet->Count = 8;
    CloneSet->Clones = PushArrayToMemorySegment(MemorySegment, 8, object_clone);
    game_object *Master = Entity->Master;
    vec_2 ParentMidpoint = Master->Midpoint;

    float X = ParentMidpoint.X - ScreenWidth;
    float Y;
    object_clone *Current = CloneSet->Clones;
    for (int i = 0; i < 3; ++i)
    {
        Y = ParentMidpoint.Y - ScreenHeight;
        for (int j = 0; j < 3; ++j)
        {
            if (i == 1 && j == 1) continue;
            else
            {
                Current->ParentEntity = Entity;
                Current->ClonedObject = PushToMemorySegment(MemorySegment, game_object);
                game_object *Clone = Current->ClonedObject;
                Clone->Type = Master->Type;
                Clone->Model = Master->Model;
                Clone->Radius = Master->Radius;
                Clone->Momentum = Master->Momentum;
                Clone->OffsetAngle = Master->OffsetAngle;
                Clone->AngularMomentum = Master->AngularMomentum;
                Clone->IsVisible = false;
                SetVertValue(&Clone->Midpoint, X, Y); 
            }
            Y += ScreenHeight;
            Current++;
        }
        X += ScreenWidth;
    }
    return CloneSet;
}

void UpdateClones(game_entity *Entity, int ScreenWidth, int ScreenHeight)
{
    clone_set *CloneSet = Entity->CloneSet;
    vec_2 ParentMidpoint = Entity->Master->Midpoint;

    float X = ParentMidpoint.X - ScreenWidth;
    float Y;
    object_clone *Current = CloneSet->Clones;
    for (int i = 0; i < 3; ++i)
    {
        Y = ParentMidpoint.Y - ScreenHeight;
        for (int j = 0; j < 3; ++j)
        {
            if (i == 1 && j == 1) continue;
            else
            {
                SetVertValue(&Current->ClonedObject->Midpoint, X, Y);
            }
            Y += ScreenHeight;
            Current++;
        }
        X += ScreenWidth;
    }
}

game_object * SpawnAsteroidObject(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjectInfo)
{
    if (GameState->NumSpawnedAsteroids < MAX_NUM_SPAWNED_ASTEROIDS)
    {
        game_object *NewAsteroid = PushToMemorySegment(MemorySegment, game_object);
        NewAsteroid->Type = GameObjectInfo->Type;
        NewAsteroid->Model = PushToMemorySegment(MemorySegment, object_model);
        object_model *Model = NewAsteroid->Model;
        vec_2 *ResourceVertices = {0};
        if (Model)
        {
            Model->Polygon = PushToMemorySegment(MemorySegment, polygon);
            polygon *A_Poly = Model->Polygon;
            switch (NewAsteroid->Type)
            {
                case ASTEROID_LARGE:
                {
                    A_Poly->N = LARGE_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->LargeAsteroidVertices;
                } break;
                case ASTEROID_MEDIUM:
                {
                    A_Poly->N = MEDIUM_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->MediumAsteroidVertices;
                } break;
                case ASTEROID_SMALL:
                {
                    A_Poly->N = SMALL_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->SmallAsteroidVertices;
                } break;
            }

            A_Poly->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
            A_Poly->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, A_Poly->N, vec_2);
            A_Poly->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
            A_Poly->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, A_Poly->N, vec_2);
            Model->Color.Red = ASTEROID_RED;
            Model->Color.Green = ASTEROID_GREEN;
            Model->Color.Blue = ASTEROID_BLUE;
            Model->LineWidth = ASTEROID_LINE_WIDTH;
            NewAsteroid->Midpoint.X = GameObjectInfo->Midpoint.X;
            NewAsteroid->Midpoint.Y = GameObjectInfo->Midpoint.Y;
            NewAsteroid->Momentum.X = GameObjectInfo->Momentum.X;
            NewAsteroid->Momentum.Y = GameObjectInfo->Momentum.Y;
            NewAsteroid->OffsetAngle = 0.0f;
            NewAsteroid->AngularMomentum = GameObjectInfo->AngularMomentum;
            NewAsteroid->IsVisible = GameObjectInfo->InitVisible;
            vert_set *Verts = A_Poly->StartVerts;
            for (uint32_t i = 0; i < A_Poly->N; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }
            NewAsteroid->Radius = CalculateMaxObjectRadius(NewAsteroid);
            return NewAsteroid;
        }
    }
    HackyAssert(0);
    return (game_object *)0; // unreachable
}

game_object * SpawnLaserObject(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjInfo)
{
    if (GameState->NumSpawnedLasers < GameState->MaxNumLasers)
    {
        game_object *NewLaser = PushToMemorySegment(MemorySegment, game_object);
        NewLaser->Model = PushToMemorySegment(MemorySegment, object_model);
        object_model *Model = NewLaser->Model;
        vec_2 *ResourceVertices = Resources->LaserVertices;
        if (Model)
        {
            NewLaser->Type = LASER;
            Model->Polygon = PushToMemorySegment(MemorySegment, polygon);
            polygon *L_Poly = Model->Polygon;

            L_Poly->N = LASER_NUM_VERTICES;
            L_Poly->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
            L_Poly->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, L_Poly->N, vec_2);
            L_Poly->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
            L_Poly->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, L_Poly->N, vec_2);
            Model->Color.Red = LASER_RED;
            Model->Color.Green = LASER_GREEN;
            Model->Color.Blue = LASER_BLUE;
            Model->LineWidth = LASER_LINE_WIDTH;

            vert_set *Verts = L_Poly->StartVerts;
            for (uint32_t i = 0; i < L_Poly->N; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }
            NewLaser->IsVisible = GameObjInfo->InitVisible;
            return NewLaser;
        }
        else
        {
            // Error creating model; handle here
            HackyAssert(0);
        }
    }
    else
    {
        HackyAssert(0);
    }

    // Unreachable
    return (game_object *)0;
}

game_object * SpawnPlayerObject(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjectInfo)
{
    game_object *Player = PushToMemorySegment(MemorySegment, game_object);
    Player->Type = PLAYER;
    Player->Midpoint.X = GameObjectInfo->Midpoint.X;
    Player->Midpoint.Y = GameObjectInfo->Midpoint.Y;

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
    Player->Momentum.X = GameObjectInfo->Momentum.X;
    Player->Momentum.Y = GameObjectInfo->Momentum.Y;
    Player->OffsetAngle = GameObjectInfo->OffsetAngle;
    Player->AngularMomentum = GameObjectInfo->AngularMomentum;
    Player->IsVisible = GameObjectInfo->InitVisible;
    Player->Radius = CalculateMaxObjectRadius(Player);

    return Player;
}

void InitializePlayer(game_entity *Player, game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjInfo)
{
    game_object *PlayerObject = SpawnPlayerObject(GameState, MemorySegment, Resources, GameObjInfo);
    Player->IsLive = true;
    Player->Type = GameObjInfo->Type;
    Player->Master = PlayerObject;
    Player->CloneSet = CreateClones(MemorySegment, Player, GameState->WorldWidth, GameState->WorldHeight);
}

void InitializeAsteroidEntity(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *NewAsteroidInfo)
{
    game_object *AsteroidObject = SpawnAsteroidObject(GameState, MemorySegment, Resources, NewAsteroidInfo);
    game_entity *AsteroidEntity = &GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids];
    AsteroidEntity->IsLive = true;
    AsteroidEntity->Master = AsteroidObject;
    AsteroidEntity->Type = NewAsteroidInfo->Type;
    AsteroidEntity->CloneSet = CreateClones(MemorySegment, AsteroidEntity, GameState->WorldWidth, GameState->WorldHeight);
    GameState->NumSpawnedAsteroids += 1;
}

void RepurposeAsteroidEntity(game_entity *Old, game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *NewAsteroidInfo)
{
    game_object *NewAsteroidObject = SpawnAsteroidObject(GameState, MemorySegment, Resources, NewAsteroidInfo);
    Old->Master = NewAsteroidObject;
    Old->IsLive = true;
    Old->Type = NewAsteroidInfo->Type;
    Old->CloneSet = CreateClones(MemorySegment, Old, GameState->WorldWidth, GameState->WorldHeight);
}

void InitializeLaserEntities(game_entity *Lasers, game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjInfo)
{
    for (uint32_t i = 0; i < GameState->MaxNumLasers; ++i)
    {
        game_object *LaserObject = SpawnLaserObject(GameState, MemorySegment, Resources, GameObjInfo);
        Lasers[i].IsLive = true;
        Lasers[i].Master = LaserObject;
        Lasers[i].Type = GameObjInfo->Type;
        Lasers[i].CloneSet = CreateClones(MemorySegment, &Lasers[i], GameState->WorldWidth, GameState->WorldHeight);
    }
}

void FireLaser(game_state *GameState, memory_segment *LaserMemSegment, loaded_resource_memory *Resources, game_entity *Player)
{
    laser_set *LaserSet = GameState->LaserSet;
    uint32_t LaserIndex = 0;
    for (; LaserIndex < GameState->MaxNumLasers; ++LaserIndex)
    {
        if (LaserSet->LifeTimers[LaserIndex] == 0)
        {
            break;
        }
    }

    game_entity *NewLaserEntity = &LaserSet->Lasers[LaserIndex];
    game_object *NewLaser = NewLaserEntity->Master;
    

    float mag = LASER_SPEED_MAG;
    float theta = Player->Master->OffsetAngle;
    NewLaser->OffsetAngle = theta;
    NewLaser->Momentum.X = mag * cosf(theta + (3.14159f / 2.0f));
    NewLaser->Momentum.Y = mag * sinf(theta + (3.14159f / 2.0f));
    NewLaser->AngularMomentum = 0.0f;
    NewLaser->IsVisible = true;

    // Laser midpoint calculated by halving the laser model vector, then rotating that vector
    // by the offset angle, then adding those values to the Player object's midpoint.
    float X = 0.0f;
    float Y = (Resources->LaserVertices[1].Y / 2.0f);
    float X_Rot = (X * cosf(theta)) - (Y * sinf(theta));
    float Y_Rot = (X * sinf(theta)) + (Y * cosf(theta));
    NewLaser->Midpoint.X = Player->Master->Midpoint.X + X_Rot;
    NewLaser->Midpoint.Y = Player->Master->Midpoint.Y + Y_Rot;
    NewLaser->Radius = CalculateMaxObjectRadius(NewLaser);
    UpdateClones(NewLaserEntity, GameState->WorldWidth, GameState->WorldHeight);
    GameState->LaserSet->LifeTimers[LaserIndex] = 77; // enough to return to spawn position when going horizontally across the screen
    GameState->NumSpawnedLasers += 1;
}

void KillLaser(game_state *GameState, uint32_t LaserIndex)
{
    GameState->LaserSet->Lasers[LaserIndex].Master->IsVisible = false;
    GameState->LaserSet->LifeTimers[LaserIndex] = 0;
    GameState->NumSpawnedLasers -= 1;
}

void HandleEntityEdgeWarping(game_entity *Entity, int ScreenWidth, int ScreenHeight)
{
    game_object *Master = Entity->Master;
    if (Master->Midpoint.X < 0)
    {
        Master->Midpoint.X += ScreenWidth;
        object_clone *Clones = Entity->CloneSet->Clones;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].ClonedObject->Midpoint.X += ScreenWidth;
        }
    }
    else if (Master->Midpoint.X >= ScreenWidth)
    {
        Master->Midpoint.X -= ScreenWidth;
        object_clone *Clones = Entity->CloneSet->Clones;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].ClonedObject->Midpoint.X -= ScreenWidth;
        }
    }
    if (Master->Midpoint.Y < 0)
    {
        Master->Midpoint.Y += ScreenHeight;
        object_clone *Clones = Entity->CloneSet->Clones;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].ClonedObject->Midpoint.Y += ScreenHeight;
        }
    }
    else if (Master->Midpoint.Y >= ScreenHeight)
    {
        Master->Midpoint.Y -= ScreenHeight;
        object_clone *Clones = Entity->CloneSet->Clones;
        for (int i = 0; i < 8; ++i)
        {
            Clones[i].ClonedObject->Midpoint.Y -= ScreenHeight;
        }
    }
}

void UpdateGameEntityMomentumAndAngle(game_state *GameState, vec_2 MomentumDelta, float AngularMomentumDelta)
{
    game_object *PlayerMaster = GameState->Player->Master;
    object_clone *PlayerClones = GameState->Player->CloneSet->Clones;

    PlayerMaster->Momentum = AddVectors(PlayerMaster->Momentum, MomentumDelta);
    PlayerMaster->OffsetAngle += PlayerMaster->AngularMomentum * AngularMomentumDelta;

    for (uint32_t i = 0; i < NUM_OBJ_CLONES; ++i)
    {
        PlayerClones[i].ClonedObject->Momentum = PlayerMaster->Momentum;
        PlayerClones[i].ClonedObject->OffsetAngle = PlayerMaster->OffsetAngle;
    }

    game_entity *Asteroids = GameState->SpawnedAsteroids->Asteroids;
    for (uint32_t i = 0; i < GameState->NumSpawnedAsteroids; ++i)
    {
        Asteroids[i].Master->OffsetAngle += Asteroids[i].Master->AngularMomentum;
        object_clone *AsteroidClones = Asteroids[i].CloneSet->Clones;
        for (uint32_t j = 0; j < NUM_OBJ_CLONES; ++j)
        {
            AsteroidClones[j].ClonedObject->OffsetAngle = Asteroids[i].Master->OffsetAngle;
        }
    }
}