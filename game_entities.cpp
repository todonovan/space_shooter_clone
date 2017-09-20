#include <stdint.h>

#include "platform.h"
#include "asteroids.h"
#include "game_entities.h"

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
    object_model *Model = Object->Model;
    vec_2 *Verts = Object->Model->StartVerts->Verts;
    float max_radius = 0.0f, current_radius = 0.0f;
    vec_2 Zero = {};
    for (uint32_t i = 0; i < Model->NumVertices; ++i)
    {
        current_radius = CalculateVectorDistance(Zero, Verts[i]);
        if (current_radius > max_radius) max_radius = current_radius;
    }
    return max_radius;
}

clone_set * CreateClones(memory_segment *MemorySegment, game_object *Object, int ScreenWidth, int ScreenHeight)
{
    clone_set *CloneSet = PushToMemorySegment(MemorySegment, clone_set);
    CloneSet->Count = 8;
    CloneSet->Clones = PushArrayToMemorySegment(MemorySegment, 8, object_clone);
    vec_2 ParentMidpoint = Object->Midpoint;

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
                Current->Parent = Object;
                Current->Midpoint = PushToMemorySegment(MemorySegment, vec_2);
                SetVertValue(Current->Midpoint, X, Y);
            }
            Y += ScreenHeight;
            Current++;
        }
        X += ScreenWidth;
    }
    return CloneSet;
}

game_object * SpawnAsteroid(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjectInfo)
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
            switch (NewAsteroid->Type)
            {
                case ASTEROID_LARGE:
                {
                    Model->NumVertices = LARGE_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->LargeAsteroidVertices;
                } break;
                case ASTEROID_MEDIUM:
                {
                    Model->NumVertices = MEDIUM_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->MediumAsteroidVertices;
                } break;
                case ASTEROID_SMALL:
                {
                    Model->NumVertices = SMALL_ASTEROID_NUM_VERTICES;
                    ResourceVertices = Resources->SmallAsteroidVertices;
                } break;
            }

            Model->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            Model->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
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
            vert_set *Verts = NewAsteroid->Model->StartVerts;
            for (uint32_t i = 0; i < NewAsteroid->Model->NumVertices; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }
            NewAsteroid->Radius = CalculateMaxObjectRadius(NewAsteroid);
            GameState->NumSpawnedAsteroids += 1;
            return NewAsteroid;
        }
    }
    else
    {
        HackyAssert(0);
    }

    // Unreachable
    return (game_object *)0;
}

game_object * SpawnLaser(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object *Player)
{
    if (GameState->NumSpawnedLasers < GameState->MaxNumLasers)
    {
        uint32_t NewLaserIndex = 0;
        while (GameState->LaserSet->Lasers[NewLaserIndex].IsLive) NewLaserIndex++;

        game_object *NewLaser = PushToMemorySegment(MemorySegment, game_object);
        NewLaser->Model = PushToMemorySegment(MemorySegment, object_model);
        object_model *Model = NewLaser->Model;
        vec_2 *ResourceVertices = Resources->LaserVertices;
        if (Model)
        {
            NewLaser->Type = LASER;
            Model->NumVertices = LASER_NUM_VERTICES;
            Model->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            Model->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
            Model->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, Model->NumVertices, vec_2);
            Model->Color.Red = LASER_RED;
            Model->Color.Green = LASER_GREEN;
            Model->Color.Blue = LASER_BLUE;
            Model->LineWidth = LASER_LINE_WIDTH;

            vert_set *Verts = Model->StartVerts;
            for (uint32_t i = 0; i < Model->NumVertices; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }

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
        // Can't spawn more lasers; do any necessary handling here (sound effects, etc.)
        // But likely none needed.
        HackyAssert(0);
    }

    // Unreachable
    return (game_object *)0;
}

game_object * SpawnPlayer(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjectInfo)
{
    game_object *Player = PushToMemorySegment(MemorySegment, game_object);
    Player->Type = PLAYER;
    Player->Midpoint.X = GameObjectInfo->Midpoint.X;
    Player->Midpoint.Y = GameObjectInfo->Midpoint.Y;

    Player->Model = PushToMemorySegment(MemorySegment, object_model);
    object_model *P_Model = Player->Model;
    P_Model->NumVertices = PLAYER_NUM_VERTICES;
    P_Model->StartVerts = PushToMemorySegment(MemorySegment, vert_set);
    P_Model->StartVerts->Verts = PushArrayToMemorySegment(MemorySegment, PLAYER_NUM_VERTICES, vec_2);

    P_Model->DrawVerts = PushToMemorySegment(MemorySegment, vert_set);
    P_Model->DrawVerts->Verts = PushArrayToMemorySegment(MemorySegment, PLAYER_NUM_VERTICES, vec_2);

    // NOTE! These values will need to be stored in a 'resource' file -- a config text file, whatever.
    for (uint32_t i = 0; i < PLAYER_NUM_VERTICES; ++i)
    {
        SetVertValue(P_Model->StartVerts, i, Resources->PlayerVertices[i].X, Resources->PlayerVertices[i].Y);
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

void InitializeGameEntity(game_entity *Entity, game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *GameObjectInfo)
{
    game_object *Master = 0;
    switch (GameObjectInfo->Type)
    {
        case PLAYER:
        {
            Master = SpawnPlayer(GameState, MemorySegment, Resources, GameObjectInfo);
            GameState->Player = Entity;
        } break;
        case LASER:
        {
            Master = SpawnLaser(GameState, MemorySegment, Resources, GameState->Player->Master);
        } break;
        case ASTEROID_SMALL:
        case ASTEROID_MEDIUM:
        case ASTEROID_LARGE:
        {
            Master = SpawnAsteroid(GameState, MemorySegment, Resources, GameObjectInfo);
        } break;
        default:
        {
            HackyAssert(0);
        }
    }
    Entity->IsLive = true;
    Entity->Type = GameObjectInfo->Type;
    Entity->Master = Master;
    Entity->Clones = CreateClones(MemorySegment, Master, GameState->WorldWidth, GameState->WorldHeight);
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

    GameState->LaserSet->LifeTimers[LaserIndex] = 77; // enough to return to spawn position when going horizontally across the screen
    GameState->NumSpawnedLasers += 1;
}

void KillLaser(game_state *GameState, uint32_t LaserIndex)
{
    GameState->LaserSet->Lasers[LaserIndex].Master->IsVisible = false;
    GameState->LaserSet->LifeTimers[LaserIndex] = 0;
    GameState->NumSpawnedLasers -= 1;
}