#pragma once

#include "model.h"
#include "common.h"
#include "geometry.h"
#include "asteroids.h"
#include "memory.h"

static reference_model_polygons ReferencePolygons;

void LoadReferencePolygons(game_state *GameState)
{
    reference_model_polygons *RefPolys = &GameState->ReferenceModelPolygons;

    // Player
    RequestResourceLoad((LPCSTR)"C:/Asteroids/build/Debug/player_vertices.dat", (void *)(&RefPolys->Player.BaseVertices), MODEL_VERTICES_SIZE);
    RefPolys->Player.N = PLAYER_NUM_VERTICES;
    // Small
    RequestResourceLoad((LPCSTR)"C:/Asteroids/build/Debug/sm_ast_vertices.dat", (void *)(&RefPolys->SmallAsteroid.BaseVertices), MODEL_VERTICES_SIZE);
    RefPolys->SmallAsteroid.N = SMALL_ASTEROID_NUM_VERTICES;
    // Medium
    RequestResourceLoad((LPCSTR)"C:/Asteroids/build/Debug/med_ast_vertices.dat", (void *)(&RefPolys->MediumAsteroid.BaseVertices), MODEL_VERTICES_SIZE);
    RefPolys->MediumAsteroid.N = MEDIUM_ASTEROID_NUM_VERTICES;
    // Large
    RequestResourceLoad((LPCSTR)"C:/Asteroids/build/Debug/lg_ast_vertices.dat", (void *)(&RefPolys->LargeAsteroid.BaseVertices), MODEL_VERTICES_SIZE);
    RefPolys->LargeAsteroid.N = LARGE_ASTEROID_NUM_VERTICES;
    // Laser
    RequestResourceLoad((LPCSTR)"C:/Asteroids/build/Debug/laser_vertices.dat", (void *)(&RefPolys->Laser.BaseVertices), MODEL_VERTICES_SIZE);
    RefPolys->Laser.N = LASER_NUM_VERTICES;

    ReferencePolygons = GameState->ReferenceModelPolygons;
}

void InitObjectModel(game_entity *Entity)
{
    object_model *Model = &Entity->Master.Model;
    switch (Entity->EntityType)
    {
        case PLAYER:
        {
            Model->Color.Blue = PLAYER_BLUE;
            Model->Color.Green = PLAYER_GREEN;
            Model->Color.Red = PLAYER_RED;

            Model->LineWidth = PLAYER_LINE_WIDTH;

            Model->Polygon.N = PLAYER_NUM_VERTICES;
            for (uint32_t i = 0; i < PLAYER_NUM_VERTICES; i++)
            {
                Model->Polygon.BaseVertices[i] = Model->Polygon.Vertices[i] = ReferencePolygons.Player.BaseVertices[i];
            }
        }; break;
        case SMALL_ASTEROID:
        {
            Model->Color.Blue = ASTEROID_BLUE;
            Model->Color.Green = ASTEROID_GREEN;
            Model->Color.Red = ASTEROID_RED;

            Model->LineWidth = ASTEROID_LINE_WIDTH;

            Model->Polygon.N = SMALL_ASTEROID_NUM_VERTICES;
            for (uint32_t i = 0; i < SMALL_ASTEROID_NUM_VERTICES; i++)
            {
                Model->Polygon.BaseVertices[i] = Model->Polygon.Vertices[i] = ReferencePolygons.SmallAsteroid.BaseVertices[i];
            }
        }; break;
        case MEDIUM_ASTEROID:
        {
            Model->Color.Blue = ASTEROID_BLUE;
            Model->Color.Green = ASTEROID_GREEN;
            Model->Color.Red = ASTEROID_RED;

            Model->LineWidth = ASTEROID_LINE_WIDTH;

            Model->Polygon.N = MEDIUM_ASTEROID_NUM_VERTICES;
            for (uint32_t i = 0; i < MEDIUM_ASTEROID_NUM_VERTICES; i++)
            {
                Model->Polygon.BaseVertices[i] = Model->Polygon.Vertices[i] = ReferencePolygons.MediumAsteroid.BaseVertices[i];
            }
        }; break;
        case LARGE_ASTEROID:
        {
            Model->Color.Blue = ASTEROID_BLUE;
            Model->Color.Green = ASTEROID_GREEN;
            Model->Color.Red = ASTEROID_RED;

            Model->LineWidth = ASTEROID_LINE_WIDTH;

            Model->Polygon.N = LARGE_ASTEROID_NUM_VERTICES;
            for (uint32_t i = 0; i < LARGE_ASTEROID_NUM_VERTICES; i++)
            {
                Model->Polygon.BaseVertices[i] = Model->Polygon.Vertices[i] = ReferencePolygons.LargeAsteroid.BaseVertices[i];
            }
        }; break;
        case LASER:
        {
            Model->Color.Blue = LASER_BLUE;
            Model->Color.Green = LASER_GREEN;
            Model->Color.Red = LASER_RED;

            Model->LineWidth = LASER_LINE_WIDTH;

            Model->Polygon.N = LASER_NUM_VERTICES;
            for (uint32_t i = 0; i < LASER_NUM_VERTICES; i++)
            {
                Model->Polygon.BaseVertices[i] = Model->Polygon.Vertices[i] = ReferencePolygons.Laser.BaseVertices[i];
            }
        }; break;
    }
    Model->Polygon.C = Entity->Master.Midpoint;
    ConstructAABB(&Model->Polygon);
}