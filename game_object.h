#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "common.h"
#include "geometry.h"
#include "model.h"
#include "input.h"
#include "entities.h"

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
    AABB BoundingBox;
};

// Used to init game objects
struct game_object_info
{
    object_type Type;
    vec_2 Midpoint;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;
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

void TickPlayerObject(game_object *Obj, asteroids_player_input *Input);
void TickLaserObject(game_object *Obj, laser_timing *Timers);
void TickAsteroidObject(game_object *Obj);

#endif