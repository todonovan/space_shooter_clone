#pragma once

#include "common.h"
#include "platform.h"
#include "memory.h"
#include "render.h"

// Note that this may not be portable, as it relies upon the way Windows structures
// bitmap data in memory.
void SetPixelInBuffer(platform_bitmap_buffer *Buffer, vec_2 *Coords, color_triple *Colors)
{
    if (!(Buffer->Memory)) return;
    int X = (Coords->X < 0) ? ((int)Coords->X + Buffer->Width) : ((int)Coords->X % Buffer->Width);
    int Y = (Coords->Y < 0) ? ((int)Coords->Y + Buffer->Height) : ((int)Coords->Y % Buffer->Height);

    uint8_t *Pixel = (uint8_t *)((char *)(Buffer->Memory) + (Y * Buffer->Width * 4) + (X * 4));

    *Pixel = (Colors->Blue);
    Pixel++;
    *Pixel = (Colors->Green);
    Pixel++;
    *Pixel = (Colors->Red);
}

/// TODO!! This implementation of Bresenham, courtesy of the internet, is busted. Must find better line-drawing
/// alg that handles width properly. All Bresenham implementations seem to be poorly suited to width handling.
void DrawLineSegmentWithWidth(platform_bitmap_buffer *Buffer, vec_2 *StartPoint, vec_2 *EndPoint, color_triple *Color, float LineWidth)
{
    int x0 = (int)(StartPoint->X);
    int x1 = (int)(EndPoint->X);
    int y0 = (int)(StartPoint->Y);
    int y1 = (int)(EndPoint->Y);
    float wd = LineWidth;
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = dx-dy, e2, x2, y2;
    float ed = (dx+dy == 0) ? 1.0f : sqrtf(((float)dx*(float)dx)+((float)dy*(float)dy));


    for (wd = (wd+1)/2; ; )
    {
        vec_2 Coords;
        Coords.X = (float)(x0);
        Coords.Y = (float)(y0);
        SetPixelInBuffer(Buffer, &Coords, Color);
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                vec_2 Coords2;
                Coords2.X = (float)(x0);
                Coords2.Y = (float)(y2 += sy);

                SetPixelInBuffer(Buffer, &Coords2, Color);
            }
            if (x0 == x1) break;
            e2 = err; err -= dy; x0 += sx;
        }
        if (2*e2 <= dy)
        {
            for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
            {
                vec_2 Coords2;
                Coords2.X = (float)(x2 += sx);
                Coords2.Y = (float)(y0);
                SetPixelInBuffer(Buffer, &Coords2, Color);
            }
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }
}

void DrawObjectModelIntoBuffer(game_object *Object, platform_bitmap_buffer *Buffer)
{
    if (!(Buffer->Memory)) return;

    int cur = 0, next = 1;
    vec_2 Cur, Next;
    object_model *Model = &Object->Model;
    polygon *Poly = &Model->Polygon;

    for (uint32_t i = 0; i < Poly->N - 1; ++i)
    {
        Cur.X = Poly->Vertices[cur].X;
        Cur.Y = Poly->Vertices[cur].Y;
        Next.X = Poly->Vertices[next].X;
        Next.Y = Poly->Vertices[next].Y;
        DrawLineSegmentWithWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
        ++cur;
        if (i < Poly->N - 2) ++next;
    }
    if (Object->Type != LASER)
    {
        Cur.X = Poly->Vertices[cur].X;
        Cur.Y = Poly->Vertices[cur].Y;
        Next.X = Poly->Vertices[0].X;
        Next.Y = Poly->Vertices[0].Y;
        DrawLineSegmentWithWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
    }
}		

// This is a good example of why it might be a better idea to have a data structure that keeps
// track of pointers to all live entities, so we can iterate through only the living ones, rather
// than having to iterate through the entire memory pools.
void RenderAllEntities(game_state *GameState, platform_bitmap_buffer *OffscreenBuffer)
{
    // Player
    if (GameState->PlayerInfo.IsLive)
    {
        DrawObjectModelIntoBuffer(&GameState->Player->Master, OffscreenBuffer);
    }

    // Lasers
    for (uint32_t i = 0; i < GameState->LaserPool->PoolInfo.BlockCount; i++)
    {
        if (!GameState->LaserPool->Blocks[i].IsFree)
        {
            DrawObjectModelIntoBuffer(&GameState->LaserPool->Blocks[i].Entity.Master, OffscreenBuffer);
        }
    }

    // Asteroids
    for (uint32_t i = 0; i < GameState->AsteroidPool->PoolInfo.BlockCount; i++)
    {
        if (!GameState->AsteroidPool->Blocks[i].IsFree)
        {
            DrawObjectModelIntoBuffer(&GameState->AsteroidPool->Blocks[i].Entity.Master, OffscreenBuffer);
        }
    }
}


// https://github.com/ArminJo/STMF3-Discovery-Demos/blob/master/lib/graphics/src/thickLine.cpp