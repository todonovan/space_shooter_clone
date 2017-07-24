#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>

#include "platform.h"

// Thoughts for allocating into this game memory...
// Need to allocate a chunk of space for the asteroids somehow (even just VirtualAlloc), and assign the resulting pointer to Memory.Asteroids
// Then, to create an asteroid, build a new asteroid within a function, then do a memcpy(), where the location is equal
// to GameMemory.Asteroids + NumAsteroids, the length is sizeof(game_object), etc.
// To iterate through the asteroids, use the NumAsteroids/pointer arithmetic again.
// When despawning an asteroid, have to consolidate the memory. Simply do a memcpy() from the last asteroid to where the
// asteroid was despawned, then decrement the NumAsteroids so the next asteroid to be added will get added to where the last one
// used to be.

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
void DrawLineWidth(platform_bitmap_buffer *Buffer, vec_2 *Point1, vec_2 *Point2, color_triple *Color, float LineWidth)
{
    int x0 = Point1->X;
    int x1 = Point2->X;
    int y0 = Point1->Y;
    int y1 = Point2->Y;
    float wd = LineWidth;
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = dx-dy, e2, x2, y2;
    float ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);


    for (wd = (wd+1)/2; ; )
    {
        vec_2 Coords;
        Coords.X = x0;
        Coords.Y = y0;
        SetPixelInBuffer(Buffer, &Coords, Color);
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                vec_2 Coords2;
                Coords2.X = x0;
                Coords2.Y = y2 += sy;

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
                Coords2.X = x2 += sx;
                Coords2.Y = y0;
                SetPixelInBuffer(Buffer, &Coords2, Color);
            }
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }
}

void SetObjectModelForDraw(game_object *Object)
{
    object_model *Model = Object->Model;
    float Theta = Object->OffsetAngle;
    vec_2 *StartVerts = Model->StartVerts->Verts;
    vec_2 *DrawVerts = Model->DrawVerts->Verts;

    for (int i = 0; i < Model->NumVertices; ++i)
    {
        float X_Orig = StartVerts[i].X;
        float Y_Orig = StartVerts[i].Y;
        DrawVerts[i].X = (X_Orig * cos(Theta)) - (Y_Orig * sin(Theta));
        DrawVerts[i].Y = (X_Orig * sin(Theta)) + (Y_Orig * cos(Theta));
    }
}

inline void HandleObjectEdgeWarping(game_object *Object, int Width, int Height)
{
    vec_2 Mid = Object->Midpoint;
    if (Object->Midpoint.X < 0)
    {
        Object->Midpoint.X += Width;
    }
    else if (Object->Midpoint.X >= Width)
    {
        Object->Midpoint.X -= Width;
    }
    if (Object->Midpoint.Y < 0)
    {
        Object->Midpoint.Y += Height;
    }
    else if (Object->Midpoint.Y >= Height)
    {
        Object->Midpoint.Y -= Height;
    }
}

void DrawObjectModelInBuffer(platform_bitmap_buffer *Buffer, game_object *Object)
{
    if (!(Buffer->Memory)) return;

    int cur = 0, next = 1;
    vec_2 Cur, Next;
    object_model *Model = Object->Model;
    vert_set *DrawVerts = Model->DrawVerts;
    vert_set *StartVerts = Model->StartVerts;

    for (int i = 0; i < Model->NumVertices - 1; ++i)
    {
        Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
        Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
        Next.X = DrawVerts->Verts[next].X + Object->Midpoint.X;
        Next.Y = DrawVerts->Verts[next].Y + Object->Midpoint.Y;
        DrawLineWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
        ++cur;
        if (i < Model->NumVertices - 2) ++next;
    }
    Cur.X = DrawVerts->Verts[cur].X + Object->Midpoint.X;
    Cur.Y = DrawVerts->Verts[cur].Y + Object->Midpoint.Y;
    Next.X = DrawVerts->Verts[0].X + Object->Midpoint.X;
    Next.Y = DrawVerts->Verts[0].Y + Object->Midpoint.Y;
    DrawLineWidth(Buffer, &Cur, &Next, &Model->Color, Model->LineWidth);
}

void BeginMemorySegment(memory_segment *Segment, uint32_t Size, uint8_t *Storage)
{
    Segment->Size = Size;
    Segment->BaseStorageLocation = Storage;
    Segment->Used = 0;
}

#define PushArrayToMemorySegment(Segment, Count, type) (type *)AssignToMemorySegment_(Segment, (Count)*sizeof(type))
#define PushToMemorySegment(Segment, type) (type *)AssignToMemorySegment_(Segment, sizeof(type))
void * AssignToMemorySegment_(memory_segment *Segment, uint32_t Size)
{
    void *Result = Segment->BaseStorageLocation + Segment->Used;
    Segment->Used += Size;
    return Result;
}

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *PlayerInput)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        // Set up game memory here!

        BeginMemorySegment(&GameState->SceneMemorySegment, (Memory->PermanentStorageSize - sizeof(game_state)),
                        (uint8_t *)&Memory->PermanentStorage + sizeof(game_state));

        
        GameState->Player = PushToMemorySegment(&GameState->SceneMemorySegment, game_object);
        GameState->Player->Type = PLAYER;
        game_object *Player = GameState->Player;
        Player->Midpoint.X = OffscreenBuffer->Width / 2;
        Player->Midpoint.Y = OffscreenBuffer->Height / 10;

        Player->Model = PushToMemorySegment(&GameState->SceneMemorySegment, object_model);
        object_model *PlayerModel = Player->Model;
        PlayerModel->NumVertices = PLAYER_NUM_VERTICES;
        PlayerModel->StartVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);
        PlayerModel->DrawVerts = PushToMemorySegment(&GameState->SceneMemorySegment, vert_set);

        for (int i = 0; i < PLAYER_NUM_VERTICES; ++i)
        {
            PlayerModel->StartVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);
        }

        for (int i = 0; i < PLAYER_NUM_VERTICES; ++i)
        {
            PlayerModel->DrawVerts->Verts = PushArrayToMemorySegment(&GameState->SceneMemorySegment, PLAYER_NUM_VERTICES, vec_2);
        }

        PlayerModel->LineWidth = 1.5f;


        /*S_Verts[0].X = -20.0f, S_Verts[0].Y = -20.0f;
        D_Verts[0].X = -20.0f, D_Verts[0].Y = -20.0f;
        S_Verts[1].X = 0.0f, S_Verts[1].Y = 40.0f;
        D_Verts[1].X = 0.0f, D_Verts[1].Y = 40.0f;
        S_Verts[2].X = 20.0f, S_Verts[2].Y = -20.0f;
        D_Verts[2].X = 20.0f, D_Verts[2].Y = -20.0f;
        S_Verts[3].X = 0.0f, S_Verts[3].Y = 0.0f;
        D_Verts[3].X = 0.0f, D_Verts[3].Y = 0.0f;
        */

		PlayerModel->Color.Red = 100, PlayerModel->Color.Blue = 100, PlayerModel->Color.Green = 100;
        Player->X_Momentum = 0.0f, Player->Y_Momentum = 0.0f;
        Player->OffsetAngle = 0.0f;
        Player->AngularMomentum = 0.1f;

        /*Asteroid = {};
        A_Model = {};
        Asteroid.Type = ASTEROID_LARGE;
        Asteroid.Model = A_Model;
        Asteroid.Model.LineWidth = 1.0f;

        Asteroid.Midpoint.X = OffscreenBuffer->Width / 10;
        Asteroid.Midpoint.Y = (OffscreenBuffer->Height / 10) * 7;
        vec_2 AstLeft, AstTopLeft, AstTopRight, AstRight, AstBotRight, AstBotLeft;
        AstLeft.X = -80.0f, AstLeft.Y = 0.0f;
        AstTopLeft.X = -25.0f, AstTopLeft.Y = 100.0f;
        AstTopRight.X = 30.0f, AstTopRight.Y = 85.0f;
        AstRight.X = 45.0f, AstRight.Y = 2.0f;
        AstBotRight.X = 20.0f, AstBotRight.Y = -35.0f;
        AstBotLeft.X = -60.0f, AstBotLeft.Y = -70.0f;

        Asteroid.Model.StartVertices[0] = Asteroid.Model.DrawVertices[0] = AstLeft;
        Asteroid.Model.StartVertices[1] = Asteroid.Model.DrawVertices[1] = AstTopLeft;
        Asteroid.Model.StartVertices[2] = Asteroid.Model.DrawVertices[2] = AstTopRight;
        Asteroid.Model.StartVertices[3] = Asteroid.Model.DrawVertices[3] = AstRight;
        Asteroid.Model.StartVertices[4] = Asteroid.Model.DrawVertices[4] = AstBotRight;
        Asteroid.Model.StartVertices[5] = Asteroid.Model.DrawVertices[5] = AstBotLeft;

        Asteroid.Model.Color.Red = 220, Asteroid.Model.Color.Blue = 220, Asteroid.Model.Color.Green = 200;
        Asteroid.X_Momentum = -0.25f, Asteroid.Y_Momentum = -1.0f;
        Asteroid.AngularMomentum = 0.005f;*/

        Memory->IsInitialized = true;
    }

    game_object *Player = GameState->Player;
    object_model *PlayerModel = Player->Model;

    if (PlayerInput->Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput->B_Pressed)
    {
        PlayerModel->Color.Red = 0, PlayerModel->Color.Blue = 200;
    }

    Player->OffsetAngle += Player->AngularMomentum * (PlayerInput->LTrigger + PlayerInput->RTrigger);
    //Asteroid.OffsetAngle += Asteroid.AngularMomentum;

    Player->X_Momentum += (PlayerInput->NormalizedLX * PlayerInput->Magnitude) * .5f;
    Player->Y_Momentum += (PlayerInput->NormalizedLY * PlayerInput->Magnitude) * .5f;

    Player->Midpoint.X += Player->X_Momentum;
    Player->Midpoint.Y += Player->Y_Momentum;

    //Asteroid.Midpoint.X += Asteroid.X_Momentum;
    //Asteroid.Midpoint.Y += Asteroid.Y_Momentum;

    //HandleObjectEdgeWarping(&Asteroid, OffscreenBuffer->Width, OffscreenBuffer->Height);
    HandleObjectEdgeWarping(Player, OffscreenBuffer->Width, OffscreenBuffer->Height);
    //SetObjectModelForDraw(&Asteroid);
    SetObjectModelForDraw(Player);
    //DrawObjectModelInBuffer(OffscreenBuffer, &Asteroid);
    DrawObjectModelInBuffer(OffscreenBuffer, Player);
}