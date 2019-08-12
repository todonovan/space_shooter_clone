#pragma once

#include "common.h"
#include "geometry.h"
#include "entities.h"
#include "memory.h"

#define PLAYER_NUM_VERTICES 4
#define PLAYER_LINE_WIDTH 2.0f
#define PLAYER_RED 200
#define PLAYER_GREEN 200
#define PLAYER_BLUE 200
#define PLAYER_ANGULAR_MOMENTUM 0.05f
#define PLAYER_MAX_MOMENTUM 30.0f

#define LARGE_ASTEROID_NUM_VERTICES 8
#define MEDIUM_ASTEROID_NUM_VERTICES 6
#define SMALL_ASTEROID_NUM_VERTICES 5

#define ASTEROID_LINE_WIDTH 1.5f
#define ASTEROID_RED 125
#define ASTEROID_GREEN 125
#define ASTEROID_BLUE 125

#define LASER_LINE_WIDTH 1.0f
#define LASER_NUM_VERTICES 2
#define LASER_RED 163
#define LASER_GREEN 42
#define LASER_BLUE 21

#define MODEL_VERTICES_SIZE (sizeof(vec_2) * MAX_NUM_VERTS)

struct object_model
{
    polygon Polygon;
    color_triple Color;
    float LineWidth;
};

struct reference_model_polygons
{
    polygon SmallAsteroid;
    polygon MediumAsteroid;
    polygon LargeAsteroid;
    polygon Laser;
    polygon Player;
};

void InitObjectModel(game_entity *Entity);

void LoadReferencePolygons(game_state *GameState);