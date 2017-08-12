#ifndef GAME_ENTITIES_H
#define GAME_ENTITIES_H

#include <windows.h>
#include <stdint.h>


void SetVertValue(vert_set *VertSet, uint32_t VertIndex, float XVal, float YVal);
void SpawnAsteroid(game_state *GameState, memory_segment *MemorySegment, loaded_resource_memory *Resources, object_type AsteroidType, float X_Spawn, float Y_Spawn,
                   float X_Mo, float Y_Mo, float AngularMomentum);
void SpawnLaser(game_state *GameState, memory_segment *MemorySegment,
                loaded_resource_memory *Resources, game_object *Player);
void DespawnLaser(game_state *GameState, uint32_t LaserIndex);
void InitializePlayer(memory_segment *MemSegment, game_state *GameState,
                      loaded_resource_memory *Resources, float X_Coord, float Y_Coord);
float CalculateObjectRadius(game_object *Object);

#endif // game_entities.h