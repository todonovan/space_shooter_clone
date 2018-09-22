#ifndef ASTEROIDS_H
#define ASTEROIDS_H


#include "common.h"
#include "platform.h"
#include "geometry.h"
#include "input.cpp"

#define PLAYER_NUM_VERTICES 4
#define PLAYER_LINE_WIDTH 2.0f
#define PLAYER_RED 200
#define PLAYER_GREEN 200
#define PLAYER_BLUE 200
#define PLAYER_ANGULAR_MOMENTUM 0.05f
#define PLAYER_MAX_MOMENTUM 30.0f

#define LARGE_ASTEROID_NUM_VERTICES 8
#define MEDIUM_ASTEROID_NUM_VERTICES 6
#define SMALL_ASTEROID_NUM_VERTICES 4

#define ASTEROID_LINE_WIDTH 1.5f
#define ASTEROID_RED 125
#define ASTEROID_GREEN 125
#define ASTEROID_BLUE 125

#define MAX_NUM_SPAWNED_ASTEROIDS 98 // Each large asteroid results in 7 total asteroids spawning; 98 gives max of 14 large asteroids

#define LASER_LINE_WIDTH 1.0f
#define LASER_NUM_VERTICES 2
#define LASER_RED 163
#define LASER_GREEN 42
#define LASER_BLUE 21

#define MAX_NUM_SPAWNED_LASERS 5

#define LASER_SPEED_MAG 25.0f
#define LASER_SPAWN_TIMER 77


// Simple rect struct, useful for AABBs
struct game_rect
{
    vec_2 TopLeft;
    vec_2 BotRight;
};

struct color_triple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
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
    polygon *Polygon;
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
    // assign index to object for global id purposes; player is 0, lasers are 1 -- MAX_LASERS, asteroids are enumerated from there
    uint32_t Index;
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
    asteroids_player_input *Input;
    uint32_t EntityCount;   // this gives us the next usable index when spawning new entities
    game_entity *Player;
    uint32_t MaxNumAsteroids;
    uint32_t NumSpawnedAsteroids;
    asteroid_set *SpawnedAsteroids;
    uint32_t MaxNumLasers;
    uint32_t NumSpawnedLasers;
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

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *Cur, platform_player_input *Last);

#endif // asteroids.h