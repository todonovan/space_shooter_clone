#pragma once

#include "asteroids.h"

bool32_t AABBAABB(AABB A, AABB B);
bool32_t CheckIfCollisionObjects(game_object *Obj1, game_object *Obj2);

// This function is used to test an object, e.g. the player object, against an object *and* its clones
bool32_t CollideObjectWithEntity(game_object *Obj, game_entity *Entity);


bool32_t CheckCollisionEntities(game_entity *Entity1, game_entity *Entity2);
void HandleAllCollisions(game_state *GameState);
