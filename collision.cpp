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

#include <stdint.h>
#include <stdlib.h>
#include "asteroids.h"
#include "geometry.h"

inline bool CheckCoarseCollision(game_object *Obj1, game_object *Obj2)
{
    float radius_sum = Obj1->Radius + Obj2->Radius;
    float distance = CalculateVectorDistance(Obj1->Midpoint, Obj2->Midpoint);
    return distance <= radius_sum;
}

bool CheckIfCollision(game_state *GameState, game_object *Obj1, game_object *Obj2, loaded_resource_memory *Resources)
{
    if (!CheckCoarseCollision(Obj1, Obj2)) return false;

    return (SeparatingAxisTest(Obj1->Model->Polygon, Obj2->Model->Polygon));
    
}

void HandleCollision(game_state *GameState, loaded_resource_memory *Resources, game_object *Obj1, game_object *Obj2)
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
                if (Obj2->IsVisible) Obj1->IsVisible = false;
            }
        } break;
    }
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