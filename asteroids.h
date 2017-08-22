#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include <windows.h>
#include <stdint.h>

#include "platform.h"

struct vec_2
{
    float X;
    float Y;
};

struct color_triple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

struct vert_set
{
    vec_2 *Verts;
};

typedef enum object_type
{
    PLAYER,
    ASTEROID_LARGE,
    ASTEROID_MEDIUM,
    ASTEROID_SMALL,
    LASER
} object_type;

struct object_model
{
    uint32_t NumVertices;
    vert_set *StartVerts;
    vert_set *DrawVerts;
    color_triple Color;
    float LineWidth;
};

// Clones are needed for collision checking at screen boundaries.
// A clone is simply a ref to an object (for vertex info) and an
// offset. The clone's position is found by adding the offset to
// the parent's position.


struct game_object
{
    object_type Type;
    object_model *Model;
    vec_2 Midpoint;
    float Radius;
    float X_Momentum;
    float Y_Momentum;
    float MaxMomentum;
    float OffsetAngle;
    float AngularMomentum;
    bool IsVisible;
};

struct game_object_info
{
    object_type Type;
    vec_2 *Midpoint;
    float X_Momentum;
    float Y_Momentum;
    float OffsetAngle;
    float AngularMomentum;
    bool InitVisible;
};

struct object_clone
{
    game_object *Parent;
    vec_2 *Midpoint;
};

struct clone_set
{
    uint32_t Count;
    object_clone *Clones;
};

struct game_entity
{
    bool IsLive;
    object_type Type;
    game_object *Master;
    clone_set *Clones;
};

struct asteroid_set
{
    game_entity *Asteroids;
};

struct laser_set
{
    game_entity *Lasers;
    uint32_t *LifeTimers;
};

struct game_state
{
    memory_segment SceneMemorySegment;
    int ScreenWidth;
    int ScreenHeight;
    game_object *Player;
    uint32_t NumSpawnedAsteroids;
    asteroid_set *SpawnedAsteroids;
    uint32_t MaxNumLasers;
    uint32_t NumSpawnedLasers;
    laser_set *LaserSet;
};

struct loaded_resource_memory
{
    memory_segment ResourceMemorySegment;
    vec_2 *PlayerVertices;
    vec_2 *SmallAsteroidVertices;
    vec_2 *MediumAsteroidVertices;
    vec_2 *LargeAsteroidVertices;
    vec_2 *LaserVertices;
};

struct game_permanent_memory
{
    memory_segment PermMemSegment;
    game_state *GameState;
    loaded_resource_memory *Resources;
};

#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size);
inline float CalculateVectorDistance(vec_2 P1, vec_2 P2);

#endif // asteroids.h