#ifndef ENTITIES_H
#define ENTITIES_H

#include "asteroids.h"

#define NUM_OBJECT_CLONES 8
#define MAX_ASTEROID_COUNT 300
#define MAX_LASER_COUNT 10

struct color_triple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

typedef enum object_type
{
    PLAYER,
    SMALL_ASTEROID,
    MEDIUM_ASTEROID,
    LARGE_ASTEROID,
    LASER
} object_type;

struct object_model
{
    polygon Polygon;
    color_triple Color;
    float LineWidth;
};

struct game_object
{
    object_type Type;
    vec_2 Midpoint;
    float MaxRadius; // for coarse collision detection
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

struct game_entity
{
    uint32_t BlockIndex;
    game_entity_pool *Pool;
    object_type EntityType;
    game_object Master;
    object_clone CloneSet[NUM_OBJECT_CLONES];
};

struct object_clone
{
    game_entity *ParentEntity;
    game_object ClonedObject;
};

void SpawnAsteroid(game_entity_pool *Pool, game_object_info NewAsteroidInfo);
void DemoteAsteroid(game_entity *Asteroid);

#endif