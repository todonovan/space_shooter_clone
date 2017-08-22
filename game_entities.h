#ifndef GAME_ENTITIES_H
#define GAME_ENTITIES_H

#include <windows.h>
#include <stdint.h>


void SetVertValue(vert_set *VertSet, uint32_t VertIndex, float XVal, float YVal);
game_entity * CreateGameEntity(game_state *GameState, memory_segment *MemorySegment,
                               loaded_resource_memory *Resources, game_object_info *GameObjectInfo);

#endif // game_entities.h