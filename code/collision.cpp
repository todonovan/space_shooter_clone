#pragma once

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
#include "memory.h"
#include "entities.h"
#include "game_object.h"

inline bool32_t AABBAABB(AABB A, AABB B)
{
    return (
        (A.Min.X < B.Min.X + B.Lengths.X) &&
        (A.Min.X + A.Lengths.X > B.Min.X) &&
        (A.Min.Y < B.Min.Y + B.Lengths.Y) &&
        (A.Min.Y + A.Lengths.Y > B.Min.Y)
    );
}

inline bool32_t CheckCoarseCollision(polygon *Poly1, polygon *Poly2)
{
    return AABBAABB(Poly1->BoundingBox, Poly2->BoundingBox);
}

bool32_t CheckIfCollisionPolygons(polygon *Poly1, polygon *Poly2)
{
    if (!CheckCoarseCollision(Poly1, Poly2)) return false;

    return (SeparatingAxisTest(Poly1, Poly2));
    
}

bool32_t CollideObjectWithEntity(game_object *Obj, game_entity *Entity)
{
    polygon *ObjPoly = &Obj->Model.Polygon;

    if (CheckIfCollisionPolygons(ObjPoly, &Entity->Master.Model.Polygon))
    {
        return true;
    }

    for (uint32_t i = 0; i < NUM_OBJECT_CLONES; i++)
    {
        if (CheckIfCollisionPolygons(ObjPoly, &Entity->CloneSet[i].Polygon))
        {
            return true;
        }
    }

    return false;
}

bool32_t CheckCollisionEntities(game_entity *Entity1, game_entity *Entity2)
{
    if (CheckIfCollisionPolygons(&Entity1->Master.Model.Polygon, &Entity2->Master.Model.Polygon)) return true;

    for (int i = 0; i < 8; i++)
    {
        if (CheckIfCollisionPolygons(&Entity1->Master.Model.Polygon, &Entity2->CloneSet[i].Polygon)) return true;
    }

    return false;
}

void HandleAsteroidShot(game_entity *Asteroid, game_state *GameState)
{
    asteroid_demote_results DemoteResults = DemoteAsteroid(Asteroid);
    if (DemoteResults.WasKilled)
    {
        GameState->PlayerInfo.Kills++;
    }
    else
    {
        // Do we need to do anything with the new asteroids? Was originally planning on keeping some data structure
        // containing pointers to all entities, but I no longer think that will be necessary (unless I decide to implement
        // a space-partitioning scheme...). Even if I do decide to implement that, adding the new asteroids to the
        // structure would almost certainly be handled elsewhere, in the entity spawning code. So this whole thing
        // can likely be changed so DemoteAsteroid() merely returns a bool indicating whether it was killed or not, although
        // having a struct with a named field does make the intention a little clearer.
    }
}

void HandlePlayerHit(game_state *GameState)
{
    GameState->PlayerInfo.IsLive = false;
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
