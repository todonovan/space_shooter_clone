#pragma once

// Forward-declarations
#include "collision.fwd.h"
#include "asteroids.fwd.h"
#include "geometry.fwd.h"
#include "game_object.fwd.h"
#include "entities.fwd.h"

#include "asteroids.h"
#include "geometry.h"
#include "game_object.h"
#include "entities.h"

bool AABBAABB(AABB A, AABB B);
bool CheckIfCollisionObjects(game_object *Obj1, game_object *Obj2);

// This function is used to test an object, e.g. the player object, against an object *and* its clones
bool CollideObjectWithEntity(game_object *Obj, game_entity *Entity);


bool CheckCollisionEntities(game_entity *Entity1, game_entity *Entity2);
void HandleAllCollisions(game_state *GameState);
