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
 */

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>


bool CheckIfCollision(vec_2 Obj1Mid, float Obj1Rad, vec_2 Obj2Mid, float Obj2Rad)
{
    return (CalculateVectorDistance(Obj1Mid, Obj2Mid)) < (Obj1Rad + Obj2Rad);
}

bool CheckCoarseCollision(game_object Obj1, game_object Obj2)
{
    return false;
}

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