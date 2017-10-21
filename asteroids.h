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

struct game_object
{
    object_type Type;
    object_model *Model;
    vec_2 Midpoint;
    float Radius;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;
    bool IsVisible;
};

struct game_object_info
{
    object_type Type;
    vec_2 Midpoint;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;
    bool InitVisible;
    bool NewEntity; // false if the associated entity is being 'repurposed'
};

// Clones are needed for collision checking at screen boundaries.
// Clones have been reworked to contain a reference to a totally
// new game object, rather than simply referring to the parent object.
// This is so clones can be placed in data structures just like their
// parents. This may be necessary if, for instance, a 'first pass' collision
// detection sweep is done. Such a pass would certainly result in parent objects
// being rejected while clones should be included; such an algorithm appears
// difficult if clones do not contain their own semi-independent game objects.

struct clone_set; // forward declaration

struct game_entity
{
    bool IsLive;
    object_type Type;
    game_object *Master;
    clone_set *CloneSet;
};

struct object_clone
{
    game_entity *ParentEntity;
    game_object *ClonedObject;
};

struct clone_set
{
    uint32_t Count;
    object_clone *Clones;
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
    int WorldWidth;
    int WorldHeight;
    game_entity *Player;
    uint32_t NumSpawnedAsteroids;
    asteroid_set *SpawnedAsteroids;
    uint32_t MaxNumLasers;
    uint32_t NumSpawnedLasers;
    memory_segment LaserMemorySegment;
    laser_set *LaserSet;
};

struct loaded_resource_memory
{
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
    memory_segment ResourceMemorySegment;
    memory_segment SceneMemorySegment;        
    memory_segment LaserMemorySegment;
    memory_segment AsteroidMemorySegment;
};

#define PERM_STORAGE_STRUCT_SIZE ((5 * (sizeof(memory_segment))) + (sizeof(game_state)) + (sizeof(loaded_resource_memory)))
#define LASER_MEMORY_SIZE Megabytes(5)
#define RESOURCE_MEMORY_SIZE Megabytes(1)
#define ASTEROID_MEMORY_SIZE ((GAME_PERM_MEMORY_SIZE - LASER_MEMORY_SIZE - RESOURCE_MEMORY_SIZE) / 2)
#define SCENE_MEMORY_SIZE ((GAME_PERM_MEMORY_SIZE - LASER_MEMORY_SIZE - RESOURCE_MEMORY_SIZE - ASTEROID_MEMORY_SIZE))


#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size);
inline float CalculateVectorDistance(vec_2 P1, vec_2 P2);
vec_2 AddVectors(vec_2 A, vec_2 B);

#endif // asteroids.h