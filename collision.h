#ifndef COLLISION_H
#define COLLISION_H

#include "asteroids.h"

void HandleCollision(game_entity *Entity1, game_entity *Entity2, game_state *GameState, loaded_resource_memory *Resources, game_permanent_memory *GamePermMem, uint32_t LaserIndex = 0);
bool CheckIfCollisionObjects(game_object *Obj1, game_object *Obj2);
bool CheckCollisionEntities(game_entity *Entity1, game_entity *Entity2);
void HandleAllCollisions(game_state *GameState);

#endif // collision.h