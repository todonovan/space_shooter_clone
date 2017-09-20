/*
 * Two step process:
 *   First, do a coarse test, by AABB
 *   If the AABB returns a 'coarse collision", then perform a raycast
 * IF AND ONLY IF this process is too slow for the # of entities we'll have on screen,
 *   add a spatial data structure to the code to allow for further trimming.
 *
 * Initial question: How to handle the 'screen wrapping'?
 * Two approaches I've considered:
 *   1. If the object is overlapping a boundary, split the object into two objects. Include
 *      some sort of tag or identifier to ensure the game still considers the two to be the
 *      same object. Potential issue: If the object is on a corner, may need to be split into FOUR.
 *      In other words, the splitting will have to work for any N, likely.
 *   2. Create clones of the object that are stored in non-screen space. In other words,
 *      extend the 'world space' so that you have copies of the world to the left, right, top, bottom.
 *      This mirrored clones obviously aren't drawn and have to mimic the movements of the "real" entity,
 *      but allow for collision detection without 'splitting' the objects on the boundaries.
 *      Issue -- how does this work for corner cases? (Like, the literal corners of the world.)
 *
 *
 *  Current crazy idea:
 *      - Give each object 8 clones, making a 3x3 grid of "world spaces"
 *      - Still need to add the concept of an AABB -- for EACH object + clone... alternatively keep doing the circle test but make it the largest radius possible, not simply an average.
 *      - Only one of these world spaces, the middle, is rendered. The rest simply have their positions
 *          tracked for collision purposes.
 *      - On object move, move the object AND all clones. Check for collisions against all objects && their clones
 *      - Meta-object (e.g., object + clones) should be stored together for easy jettisoning
 *      - Memory is getting a little eesh. I honest-to-god might want to implement a sort of basic GC. Nothing that
 *          does reference counting or anything similarly crazy. Instead, just mark memory chunks as zeroed out when
 *          deleted, and pause occasionally to clear out unused memory.
 *      - Alternatively, just go hog wild with memory and simply clear out the memory at the end of each level...
 */

#include <stdint.h>
#include <stdlib.h>

inline bool CheckCoarseCollision(vec_2 *Obj1_Position, game_object *Obj1, vec_2 *Obj2_Position, game_object *Obj2)
{
    float radius_sum = Obj1->Radius + Obj2->Radius;
    float distance = CalculateVectorDistance(Obj1->Midpoint, Obj2->Midpoint);
    return distance <= radius_sum;
}

// Accepting the position separate from the game_object itself allows the procedure to check collisions
// for desired/targeted endpoints of movement, rather than simply limiting to the current, un-updated
// position of an entity.
bool CheckCollision(vec_2 *Obj1_Position, game_object *Obj1, vec_2 *Obj2_Position, game_object *Obj2)
{
    return CheckCoarseCollision(Obj1_Position, Obj1, Obj2_Position, Obj2);
}

void HandleCollision(game_state *GameState, loaded_resource_memory *Resources, game_object *Obj1, game_object *Obj2, uint32_t LaserIndex)
{
    return;
}

/*
void HandleCollision(game_state *GameState, loaded_resource_memory *Resources, game_object *Obj1, game_object *Obj2, uint32_t LaserIndex)
{
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
                Obj1->IsVisible = false;
            }
        } break;
        case ASTEROID_SMALL:
        case ASTEROID_MEDIUM:
        case ASTEROID_LARGE:
        {

        } break;
        case LASER:
        {
            if (Obj2->Type == ASTEROID_SMALL)
            {
				Obj2->IsVisible = false;
            }
            else if (Obj2->Type == ASTEROID_MEDIUM)
            {
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, Resources, ASTEROID_SMALL, Obj2->Midpoint.X + 40.0f, Obj2->Midpoint.Y + 40.0f, 2.0f * Obj2->X_Momentum, 2.0f * Obj2->Y_Momentum, 2.0f * Obj2->AngularMomentum);
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, Resources, ASTEROID_SMALL, Obj2->Midpoint.X - 40.0f, Obj2->Midpoint.Y - 40.0f, -2.0f * Obj2->X_Momentum, -2.0f * Obj2->Y_Momentum, -2.0f * Obj2->AngularMomentum);
				Obj2->IsVisible = false;;
            }
            else if (Obj2->Type == ASTEROID_LARGE)
            {
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, Resources, ASTEROID_MEDIUM, Obj2->Midpoint.X + 70.0f, Obj2->Midpoint.Y + 70.0f, 2.0f * Obj2->X_Momentum, 2.0f * Obj2->Y_Momentum, 2.0f * Obj2->AngularMomentum);
                SpawnAsteroid(GameState, &GameState->SceneMemorySegment, Resources, ASTEROID_MEDIUM, Obj2->Midpoint.X - 70.0f, Obj2->Midpoint.Y - 70.0f, -2.0f * Obj2->X_Momentum, -2.0f * Obj2->Y_Momentum, -2.0f * Obj2->AngularMomentum);
				Obj2->IsVisible = false;
            }

            DespawnLaser(GameState, LaserIndex);
        } break;
    }
}
*/