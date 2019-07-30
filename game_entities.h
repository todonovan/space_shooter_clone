#ifndef GAME_ENTITIES_H
#define GAME_ENTITIES_H

#include "geometry.h"

#define NUM_OBJ_CLONES 8
#define PLAYER_INDEX 0

void SetVertValue(vert_set *VertSet, uint32_t VertIndex, float XVal, float YVal);

void InitializePlayer(game_entity *Entity, game_state *GameState, memory_segment *MemorySegment,
    loaded_resource_memory *Resources, game_object_info *GameObjectInfo);

void InitializeAsteroidEntity(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *NewAsteroidInfo);

void InitializeLaserEntities(game_entity *Entity, game_state *GameState, memory_segment *MemorySegment,
    loaded_resource_memory *Resources, game_object_info *GameObjectInfo);

void RepurposeAsteroidEntity(game_entity *Old, game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, game_object_info *NewAsteroidInfo);

void KillLaser(game_state *GameState, game_entity *Laser);

void FireLaser(game_state *GameState, memory_segment *LaserMemSegment, loaded_resource_memory *Resources, game_entity *Player);



void UpdateGameEntityMomentumAndAngle(game_state *GameState, vec_2 MomentumD, float AngularMomentumD);

void IterateThroughAllGameEntities(game_state *GameState);
void IterateThroughAllGameObjects(game_state *GameState);
#endif // game_entities.h