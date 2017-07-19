#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>

#include "platform.h"

struct vec_2
{
    float X;
    float Y;
};

struct color_triple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

struct player_model
{
    vec_2 StartVertices[4];
    vec_2 DrawVertices[4];
    color_triple Color;
    float LineWidth;
};

struct player_object
{
    player_model *Model;
    vec_2 Midpoint;
    float X_Momentum;
    float Y_Momentum;
    float OffsetAngle;
    float AngularMomentum;
};

struct game_memory
{
    uint32_t PersistStorageSize;
    void *PersistStorage;
    uint32_t TransientStorageSize;
    void *TransientStorage;
    bool Initialized;
};

static game_memory GameMemory;
static player_object Player;
static bool GameMemoryInitialized = false;

// Note that this may not be portable, as it relies upon the way Windows structures
// bitmap data in memory.
void SetPixelInBuffer(platform_bitmap_buffer *Buffer, vec_2 *Coords, color_triple *Colors, float Brightness)
{
    if (!(Buffer->Memory)) return;
    int X = (Coords->X < 0) ? ((int)Coords->X + Buffer->Width) : ((int)Coords->X % Buffer->Width);
    int Y = (Coords->Y < 0) ? ((int)Coords->Y + Buffer->Height) : ((int)Coords->Y % Buffer->Height);

    uint8_t *Pixel = (uint8_t *)((char *)(Buffer->Memory) + (Y * Buffer->Width * 4) + (X * 4));

    *Pixel = (Colors->Blue * Brightness);
    Pixel++;
    *Pixel = (Colors->Green * Brightness);
    Pixel++;
    *Pixel = (Colors->Red * Brightness);
}

/// TODO!! Restore the actual width capabilities (e.g., reducing brightness incrementally further away pixel is from center of line)
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
        // To restore the anti-aliasing, this proc would have to take a param
        // that represented the amount to scale down the 'brightness' of the pixel.
        SetPixelInBuffer(Buffer, &Coords, Color, max(0,100*(abs(err-dx+dy)/ed-wd+1)));
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                vec_2 Coords2;
                Coords2.X = x0;
                Coords2.Y = y2 += sy;

                SetPixelInBuffer(Buffer, &Coords2, Color, max(0,100*(abs(err-dx+dy)/ed-wd+1)));
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
                SetPixelInBuffer(Buffer, &Coords2, Color, max(0,100*(abs(err-dx+dy)/ed-wd+1)));
            }
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }
}

void SetPlayerModelForDraw()
{
    float Theta = Player.OffsetAngle;
    for (int i = 0; i < 4; ++i)
    {
        float X_Orig = Player.Model->StartVertices[i].X;
        float Y_Orig = Player.Model->StartVertices[i].Y;
        Player.Model->DrawVertices[i].X = (X_Orig * cos(Theta)) - (Y_Orig * sin(Theta));
        Player.Model->DrawVertices[i].Y = (X_Orig * sin(Theta)) + (Y_Orig * cos(Theta));
    }
}

inline void HandlePlayerEdgeWarping(int Width, int Height)
{
    if (Player.Midpoint.X < 0)
    {
        Player.Midpoint.X += Width;
    }
    else if (Player.Midpoint.X >= Width)
    {
        Player.Midpoint.X -= Width;
    }
    if (Player.Midpoint.Y < 0)
    {
        Player.Midpoint.Y += Height;
    }
    else if (Player.Midpoint.Y >= Height)
    {
        Player.Midpoint.Y -= Height;
    }
}

// To consider -- remove the hacky DrawVertices[] from model, simply handle
// the rotation here prior to draw.
// Consider -- will we want the true location of the player's vertices stored
// for things like collision detection? May want to keep even though DrawVertices[]
// seems like a strange solution.
// Rebuttal -- if we need the actual vertices, post-rota, stored, is storing the angle
// of offset and recomputing rotation each frame still the appropriate method?
// Maybe just do both. Store the angle of offset (for things like laser spawning) and
// simply recompute the draw vertices each time from the start vertices.
void DrawPlayerModelInBuffer(platform_bitmap_buffer *Buffer)
{
    if (!(Buffer->Memory)) return;

    int cur = 0, next = 1;
    vec_2 Cur, Next;

    for (int i = 0; i < 3; ++i)
    {
        Cur.X = Player.Model->DrawVertices[cur].X + Player.Midpoint.X;
        Cur.Y = Player.Model->DrawVertices[cur].Y + Player.Midpoint.Y;
        Next.X = Player.Model->DrawVertices[next].X + Player.Midpoint.X;
        Next.Y = Player.Model->DrawVertices[next].Y + Player.Midpoint.Y;
        DrawLineWidth(Buffer, &Cur, &Next, &(Player.Model->Color), Player.Model->LineWidth);
        ++cur;
        if (i < 2) ++next;
    }
    Cur.X = Player.Model->DrawVertices[cur].X + Player.Midpoint.X;
    Cur.Y = Player.Model->DrawVertices[cur].Y + Player.Midpoint.Y;
    Next.X = Player.Model->DrawVertices[0].X + Player.Midpoint.X;
    Next.Y = Player.Model->DrawVertices[0].Y + Player.Midpoint.Y;
    DrawLineWidth(Buffer, &Cur, &Next, &(Player.Model->Color), Player.Model->LineWidth);
}

void UpdateGameAndRender(platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *PlayerInput)
{
    if (!GameMemoryInitialized)
    {
        // Initialize game memory here!
        Player = {};
        Player->Model.LineWidth = 3.0f;

        Player.Midpoint.X = OffscreenBuffer->Width / 2;
        Player.Midpoint.Y = OffscreenBuffer->Height / 10;
        vec_2 PlayerLeft, PlayerTop, PlayerRight, PlayerBottom;
        PlayerLeft.X = -20.0f, PlayerLeft.Y = -20.0f;
        PlayerTop.X = 0.0f, PlayerTop.Y = 40.0f;
        PlayerRight.X = 20.0f, PlayerRight.Y = -20.0f;
        PlayerBottom.X = 0.0f, PlayerBottom.Y = 0.0f;
        Player->Model.StartVertices[0] = PlayerLeft, Player->Model.StartVertices[1] = Player->PlayerTop, Player->Model.StartVertices[2] = PlayerRight, Player->Model.StartVertices[3] = PlayerBottom;
		Player->Model.DrawVertices[0] = PlayerLeft, Player->Model.DrawVertices[1] = Player->PlayerTop, Player->Model.DrawVertices[2] = PlayerRight, Player->Model.DrawVertices[3] = PlayerBottom;

		Player->Model.Color.Red = 100, Player->Model.Color.Blue = 100, Player->Model.Color.Green = 100;

        // This value currently is fixed and does not change
        Player.AngularMomentum = 0.1f;
		GameMemoryInitialized = true;
    }

    if (PlayerInput->Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput->B_Pressed)
    {
        Player.Model->Color.Red = 0, Player.Model->Color.Blue = 200;
    }
    if (PlayerInput->LTrigger_Pressed)
    {
        Player.OffsetAngle += Player.AngularMomentum;
    }
    if (PlayerInput->RTrigger_Pressed)
    {
        Player.OffsetAngle -= Player.AngularMomentum;
    }

    Player.X_Momentum += (PlayerInput->NormalizedLX * PlayerInput->Magnitude) * .5f;
    Player.Y_Momentum += (PlayerInput->NormalizedLY * PlayerInput->Magnitude) * .5f;

    Player.Midpoint.X += Player.X_Momentum;
    Player.Midpoint.Y += Player.Y_Momentum;

    HandlePlayerEdgeWarping(OffscreenBuffer->Width, OffscreenBuffer->Height);
    SetPlayerModelForDraw();
    DrawPlayerModelInBuffer(OffscreenBuffer);
}