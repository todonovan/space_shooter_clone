#ifndef COLLISION_CPP
#define COLLISION_CPP

/*
 * Two step process:
 *   First, do a coarse test, by AABB
 *   If the AABB returns a 'coarse collision", then perform a raycast
 * IF AND ONLY IF this process is too slow for the # of entities we'll have on screen,
 *   add a spatial data structure to the code to allow for further trimming.
 *
 *  Current crazy idea:
 *      - Give each object 8 clones, making a 3x3 grid of "world spaces"
 *      - Still need to add the concept of an AABB -- for EACH object + clone... alternatively keep doing the circle test but make it the largest radius possible, not simply an average.
 *      - Only one of these world spaces, the middle, is rendered. The rest simply have their positions
 *          tracked for collision purposes.
 *      - On object move, move the object AND all clones. Check for collisions against all objects && their clones
 *      - Meta-object (e.g., object + clones) should be stored together for easy jettisoning
 */

#include "common.h"
#include "asteroids.h"
#include "geometry.h"
#include "game_entities.h"

inline bool CheckCoarseCollisionObjects(game_object *Obj1, game_object *Obj2)
{
    float radius_sum = Obj1->Radius + Obj2->Radius;
    float distance = CalculateVectorDistance(Obj1->Midpoint, Obj2->Midpoint);
    return distance <= radius_sum;
}

bool CheckIfCollisionObjects(game_object *Obj1, game_object *Obj2)
{
    if (!CheckCoarseCollisionObjects(Obj1, Obj2)) return false;

    return (SeparatingAxisTest(Obj1->Model->Polygon, Obj2->Model->Polygon));
    
}

bool CheckCollisionEntities(game_entity *Entity1, game_entity *Entity2)
{
    if (CheckIfCollisionObjects(Entity1->Master, Entity2->Master)) return true;

    for (int i = 0; i < 8; i++)
    {
        if (CheckIfCollisionObjects(Entity1->Master, Entity2->CloneSet->Clones[i].ClonedObject)) return true;
    }

    return false;
}

void HandleAsteroidShot(game_entity *Asteroid, game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources)
{
    // When spawning new asteroids out of a destroyed parent, the parent asteroid entity
    // is reused, and only one new entity is spawned.
    game_object *ParentObj = Asteroid->Master;
    switch (Asteroid->Type)
    {
        case ASTEROID_SMALL:
        {
            Asteroid->IsLive = false;
            Asteroid->Master->IsVisible = false;
        } break;
        case ASTEROID_MEDIUM:
        {
            game_object_info AsteroidInfo;

            AsteroidInfo.Type = ASTEROID_SMALL;
            AsteroidInfo.Midpoint = ParentObj->Midpoint;
            AsteroidInfo.Momentum = ParentObj->Momentum;
            AsteroidInfo.Momentum.X += ((float) (rand() % 10)) * .8f;
            AsteroidInfo.Momentum.Y -= ((float) (rand() % 10)) * .8f;
            AsteroidInfo.OffsetAngle = ParentObj->OffsetAngle;
            AsteroidInfo.InitVisible = true;
            AsteroidInfo.NewEntity = false;

            RepurposeAsteroidEntity(Asteroid, GameState, MemorySegment, Resources, &AsteroidInfo);

            AsteroidInfo.Momentum.X -= ((float) (rand() % 10)) * .8f;
            AsteroidInfo.Momentum.Y += ((float) (rand() % 10)) * .8f;

            InitializeAsteroidEntity(GameState, MemorySegment, Resources, &AsteroidInfo);
        } break;
        case ASTEROID_LARGE:
        {
            game_object_info AsteroidInfo;

            AsteroidInfo.Type = ASTEROID_MEDIUM;
            AsteroidInfo.Midpoint = ParentObj->Midpoint;
            AsteroidInfo.Momentum = ParentObj->Momentum;
            AsteroidInfo.Momentum.X += ((float) (rand() % 10)) * .6f;
            AsteroidInfo.Momentum.Y -= ((float) (rand() % 10)) * .6f;
            AsteroidInfo.OffsetAngle = ParentObj->OffsetAngle;
            AsteroidInfo.InitVisible = true;
            AsteroidInfo.NewEntity = false;

            RepurposeAsteroidEntity(Asteroid, GameState, MemorySegment, Resources, &AsteroidInfo);

            AsteroidInfo.Momentum.X -= ((float) (rand() % 10)) * .6f;
            AsteroidInfo.Momentum.Y += ((float) (rand() % 10)) * .6f;

            InitializeAsteroidEntity(GameState, MemorySegment, Resources, &AsteroidInfo);
        } break;
        default:
        {
            // should be unreachable
            HackyAssert(0);
        } break;
    }
}

void HandlePlayerHit(game_entity *Player)
{
    Player->Master->IsVisible = false;
}

void HandleCollision(game_entity *Entity1, game_entity *Entity2, game_state *GameState, loaded_resource_memory *Resources, game_permanent_memory *GamePermMem, uint32_t LaserIndex)
{
    game_object *Obj1 = Entity1->Master;
    game_object *Obj2 = Entity2->Master;
    if (!Obj2->IsVisible) return;

    switch(Obj1->Type)
    {
        case PLAYER:
        {
            if (Obj2->Type == LASER)
            {
                return;
            }
            else
            {
                HandlePlayerHit(Entity1);
            }
        } break;
        case ASTEROID_SMALL:
        case ASTEROID_MEDIUM:
        case ASTEROID_LARGE:
        {
            if (Obj2->Type == ASTEROID_SMALL || Obj2->Type == ASTEROID_MEDIUM || Obj2->Type == ASTEROID_LARGE) return; // OG asteroids don't bounce
            else if (Obj2->Type == LASER)
            {
                HandleAsteroidShot(Entity1, GameState, &GamePermMem->AsteroidMemorySegment, Resources);
                KillLaser(GameState, Entity2);
            }
            else if (Obj2->Type == PLAYER)
            {
                HandlePlayerHit(Entity2);
            }
        } break;
        case LASER:
        {
            if (Obj2->Type == LASER || Obj2->Type == PLAYER) return;
            else
            {
                HandleAsteroidShot(Entity2, GameState, &GamePermMem->AsteroidMemorySegment, Resources);
                KillLaser(GameState, Entity1);
            }
        } break;
    }
}

void HandleAllCollisions(game_state *GameState)
{
    // Player collisions
    
}

#endif