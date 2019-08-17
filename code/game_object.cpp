#pragma once

#include "common.h"
#include "game_object.h"
#include "input.h"
#include "entities.h"
#include "geometry.h"
#include "memory.h"

// the entire pipeline for moving, rotating, etc, should live here? entity? split?

// This resets the 'scratch-paper' vertices to the object's starting vertices,
// allowing for simpler math for rotations.
void ResetObjectVerticesForFrame(game_object *Obj)
{
    for (uint32_t i = 0; i < Obj->Model.Polygon.N; i++)
    {
        Obj->Model.Polygon.Vertices[i] = Obj->Model.Polygon.BaseVertices[i];
    }
}

void TranslateObjectToWorldSpace(game_object *Obj)
{
    object_model *M = &Obj->Model;
    RotatePolygon(&Obj->Model.Polygon, Obj->OffsetAngle);
    
    M->Polygon.C = Obj->Midpoint;
    for (uint32_t i = 0; i < M->Polygon.N; i++)
    {
        TranslateVector(&M->Polygon.Vertices[i], M->Polygon.C);
    }

}

void TickPlayerObject(game_state *GameState, asteroids_player_input *Input)
{
    game_object *Player = &GameState->Player->Master;
    vec_2 deltaV = ScaleVector(Input->LeftStick.StickVector_Normalized, Input->LeftStick.Magnitude * 0.8f);
    TranslateVector(&Player->Momentum, deltaV);
    TranslateVector(&Player->Midpoint, Player->Momentum);
    
    float deltaTheta = Player->AngularMomentum * (Input->LTrigger + Input->RTrigger);
    Player->OffsetAngle += deltaTheta;

    ResetObjectVerticesForFrame(Player);
    TranslateObjectToWorldSpace(Player);

    Player->Model.Polygon.BoundingBox = ConstructAABB(&Player->Model.Polygon);
}

void TickLaserObject(game_object *Obj)
{
    TranslateVector(&Obj->Midpoint, Obj->Momentum);
    Obj->Model.Polygon.C = Obj->Midpoint;

    ResetObjectVerticesForFrame(Obj);
    TranslateObjectToWorldSpace(Obj);
    
    Obj->Model.Polygon.BoundingBox = ConstructAABB(&Obj->Model.Polygon);
}

void TickAsteroidObject(game_object *Obj)
{
    TranslateVector(&Obj->Midpoint, Obj->Momentum);
    Obj->OffsetAngle += Obj->AngularMomentum;
    ResetObjectVerticesForFrame(Obj);
    
    TranslateObjectToWorldSpace(Obj);

    Obj->Model.Polygon.BoundingBox = ConstructAABB(&Obj->Model.Polygon);
}