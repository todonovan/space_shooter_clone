#pragma once

// Forward-declarations
#include "game_object.fwd.h"
#include "common.fwd.h"
#include "geometry.fwd.h"
#include "model.fwd.h"
#include "input.fwd.h"

#include "common.h"
#include "geometry.h"
#include "model.h"

typedef enum object_type
{
    PLAYER,
    SMALL_ASTEROID,
    MEDIUM_ASTEROID,
    LARGE_ASTEROID,
    LASER
} object_type;

struct game_object
{
    object_type Type;
    vec_2 Midpoint;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;
    object_model Model;
};

// Used to init game objects
struct game_object_info
{
    object_type Type;
    vec_2 Midpoint;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;
    uint32_t WorldWidth;
    uint32_t WorldHeight;
};

// Clone is simply a reference to the clone's master, an offset to add to the master's center
// to get the clone's center, and a model to allow for space to, e.g., store computed vertices
// instead of having to recompute them on each access.
struct object_clone
{
    game_object *Master;
    vec_2 Offset;
    polygon Polygon;
};

void TickPlayerObject(game_state *GameState, asteroids_player_input *Input);
void TickLaserObject(game_object *Obj);
void TickAsteroidObject(game_object *Obj);
