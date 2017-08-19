#include <windows.h>
#include <stdint.h>

#include "platform.h"
#include "asteroids.h"
#include "game_entities.h"

// TODO!!!!!!!!!!!!!! Set drawing points as LINE SEGMENTS, not sets of vertices!
// Change the drawing code to operate on line segments instead of points.
// Will more easily facilitate arbitrary drawing

void SetVertValue(vert_set *VertSet, uint32_t VertIndex, float XVal, float YVal)
{
    VertSet->Verts[VertIndex].X = XVal;
    VertSet->Verts[VertIndex].Y = YVal;
}

inline float Slope(vec_2 *P1, vec_2 *P2)
{
    return ((P2->Y - P1->Y) / (P2->X - P1->X));
}

void SpawnAsteroid(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, object_type AsteroidType, float X_Spawn, float Y_Spawn, float X_Mo, float Y_Mo, float AngularMomentum)
{
    if (GameState->NumSpawnedAsteroids < MAX_NUM_SPAWNED_ASTEROIDS)
    {
        game_object *NewAsteroid = &GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids];
        NewAsteroid->Type = AsteroidType;
        NewAsteroid->Model = PushToMemorySegment(MemorySegment, object_model);
        object_model *Model = NewAsteroid->Model;
        vec_2 *ResourceVertices = 0;
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
            GameState->NumSpawnedAsteroids += 1;
            Model->Color.Red = ASTEROID_RED;
            Model->Color.Green = ASTEROID_GREEN;
            Model->Color.Blue = ASTEROID_BLUE;
            Model->LineWidth = ASTEROID_LINE_WIDTH;
            NewAsteroid->Midpoint.X = X_Spawn;
            NewAsteroid->Midpoint.Y = Y_Spawn;
            NewAsteroid->X_Momentum = X_Mo;
            NewAsteroid->Y_Momentum = Y_Mo;
            NewAsteroid->OffsetAngle = 0.0f;
            NewAsteroid->AngularMomentum = AngularMomentum;
            NewAsteroid->IsVisible = true;
            vert_set *Verts = GameState->SpawnedAsteroids->Asteroids[GameState->NumSpawnedAsteroids - 1].Model->StartVerts;
            for (uint32_t i = 0; i < NewAsteroid->Model->NumVertices; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }
            NewAsteroid->Radius = CalculateObjectRadius(NewAsteroid);
        }
    }
    else
    {
        // Out of space to store asteroids; handle here?
    }
}

void SpawnLaser(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object *Player)
{
    if (GameState->NumSpawnedLasers < GameState->MaxNumLasers)
    {
        uint32_t NewLaserIndex = 0;
        while (GameState->LaserSet->Lasers[NewLaserIndex].IsVisible) NewLaserIndex++;

        game_object *NewLaser = &GameState->LaserSet->Lasers[NewLaserIndex];
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
            GameState->NumSpawnedLasers += 1;
            Model->Color.Red = LASER_RED;
            Model->Color.Green = LASER_GREEN;
            Model->Color.Blue = LASER_BLUE;
            Model->LineWidth = LASER_LINE_WIDTH;

            float mag = LASER_SPEED_MAG;
            float theta = Player->OffsetAngle;
            NewLaser->OffsetAngle = theta;
            NewLaser->X_Momentum = mag * cosf(theta + (3.14159f / 2.0f));
            NewLaser->Y_Momentum = mag * sinf(theta + (3.14159f / 2.0f));
            NewLaser->AngularMomentum = 0.0f;
            NewLaser->IsVisible = true;

            vert_set *Verts = Model->StartVerts;
            for (uint32_t i = 0; i < Model->NumVertices; ++i)
            {
                SetVertValue(Verts, i, ResourceVertices[i].X, ResourceVertices[i].Y);
            }

            // Laser midpoint calculated by halving the laser model vector, then rotating that vector
            // by the offset angle, then adding those values to the Player object's midpoint.
            float X = 0.0f;
            float Y = (Resources->LaserVertices[1].Y / 2.0f);
            float X_Rot = (X * cosf(theta)) - (Y * sinf(theta));
            float Y_Rot = (X * sinf(theta)) + (Y * cosf(theta));
            NewLaser->Midpoint.X = Player->Midpoint.X + X_Rot;
            NewLaser->Midpoint.Y = Player->Midpoint.Y + Y_Rot;
            NewLaser->Radius = CalculateObjectRadius(NewLaser);

            GameState->LaserSet->LifeTimers[NewLaserIndex] = 77; // enough to return to spawn position when going horizontally across the screen
        }
        else
        {
            // Error creating model; handle here
        }
    }
    else
    {
        // Can't spawn more lasers; do any necessary handling here (sound effects, etc.)
        // But likely none needed.
    }
}

void DespawnLaser(game_state *GameState, uint32_t LaserIndex)
{
    GameState->LaserSet->Lasers[LaserIndex] = {};
    GameState->LaserSet->LifeTimers[LaserIndex] = 0;
    GameState->NumSpawnedLasers -= 1;
}

void InitializePlayer(memory_segment *MemSegment, game_state *GameState, loaded_resource_memory *Resources, float X_Coord, float Y_Coord)
{
    GameState->Player = PushToMemorySegment(&GameState->SceneMemorySegment, game_object);
    game_object *Player = GameState->Player;
    Player->Type = PLAYER;
    Player->Midpoint.X = X_Coord;
    Player->Midpoint.Y = Y_Coord;

    Player->Model = PushToMemorySegment(&GameState->SceneMemorySegment, object_model);
    object_model *P_Model = Player->Model;
    P_Model->NumVertices = PLAYER_NUM_VERTICES;
    P_Model->StartVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
    P_Model->StartVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);

    P_Model->DrawVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
    P_Model->DrawVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);

    // NOTE! These values will need to be stored in a 'resource' file -- a config text file, whatever.
    for (uint32_t i = 0; i < PLAYER_NUM_VERTICES; ++i)
    {
        SetVertValue(GameState->Player->Model->StartVerts, i, Resources->PlayerVertices[i].X, Resources->PlayerVertices[i].Y);
    }

    P_Model->LineWidth = PLAYER_LINE_WIDTH;

    P_Model->Color.Red = PLAYER_RED;
    P_Model->Color.Green = PLAYER_GREEN;
    P_Model->Color.Blue = PLAYER_BLUE;
    Player->X_Momentum = 0.0f, Player->Y_Momentum = 0.0f;
    Player->OffsetAngle = 0.0f;
    Player->AngularMomentum = PLAYER_ANGULAR_MOMENTUM;
    Player->MaxMomentum = PLAYER_MAX_MOMENTUM;
    Player->IsVisible = true;
    Player->Radius = CalculateObjectRadius(Player);
}

void PopulateScreenCrossingVec(vec_2 *P1, vec_2 *P2, vec_2 *Crossing, int ScreenWidth, int ScreenHeight)
{
    if ((P1->X < 0 && P2->X > 0) || (P2->X < 0 && P1->X > 0))
    {
        Crossing->X = 0.0f;
        float slope = Slope(P1, P2);
        Crossing->Y = P1->Y + (fabs(P1->X) * slope);
        return;
    }
    if ((P1->X < ScreenWidth && P2->X > ScreenWidth) || (P2->X < ScreenWidth && P1->X > ScreenWidth))
    {
        Crossing->X = (float)ScreenWidth;
        float slope = Slope(P1, P2);
        Crossing->Y = P1->Y + (fabs(P1->X - (float)ScreenWidth) * slope);
        return;
    }
    if ((P1->Y < 0 && P2->Y > 0) || (P2->Y < 0 && P1->Y > 0))
    {
        Crossing->Y = 0.0f;
        float slope = Slope(P1, P2);
        Crossing->X = P1->X + (P1->Y * fabs(slope));
    }
    if ((P1->Y < ScreenHeight && P2->Y < ScreenHeight) || (P2->Y < ScreenHeight && P1->Y > ScreenHeight))
    {
        Crossing->Y = (float)ScreenHeight;
        float slope = Slope(P1, P2);
        Crossing->X = P1->X + ((P1->Y - (float)ScreenHeight) * slope);
    }
}

bool CheckScreenCrossing(vec_2 P1, vec_2 P2, int ScreenWidth, int ScreenHeight)
{
    if (P1.X < 0 && P2.X > 0) || (P1.X > 0 && P2.X < 0) return true;
    if (P1.X < ScreenWidth && P2.X > ScreenWidth) || (P1.X > ScreenWidth && P2.X < ScreenWidth) return true;

    if (P1.Y < 0 && P2.Y > 0) || (P1.Y > 0 && P2.Y < 0) return true;
    if (P1.Y < ScreenHeight && P2.Y > ScreenHeight) || (P1.Y > ScreenHeight && P2.Y < ScreenHeight) return true;

    return false;
}

float CalculateObjectRadius(game_object *Object)
{
    object_model *Model = Object->Model;
    vec_2 *Verts = Object->Model->StartVerts->Verts;
    float cur_mag = 0.0f;
    vec_2 Zero = {};
    for (uint32_t i = 0; i < Model->NumVertices; ++i)
    {
        cur_mag += CalculateVectorDistance(Zero, Verts[i]);
    }
    return (cur_mag / (float)Model->NumVertices);
}