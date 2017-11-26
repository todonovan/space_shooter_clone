#ifndef COLLISION_H
#define COLLISION_H

#include <windows.h>
#include <stdint.h>

#include "asteroids.h"
#include "geometry.h"
#include "game_entities.h"

void HandleCollision(game_state *GameState, loaded_resource_memory *Resources, game_object *Obj1, game_object *Obj2);
bool CheckIfCollision(vec_2 Obj1Mid, float Obj1Rad, vec_2 Obj2Mid, float Obj2Rad);
bool CheckCoarseCollision(game_object Obj1, game_object Obj2);

#endif // collision.h