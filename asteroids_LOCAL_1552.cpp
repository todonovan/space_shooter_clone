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

typedef enum object_type {
    PLAYER,
    ASTEROID_LARGE,
    ASTEROID_MEDIUM,
    ASTEROID_SMALL
} object_type;

struct object_model
{
    int NumVertices;
    vec_2 *StartVertices;
    vec_2 *DrawVertices;
    color_triple Color;
    float LineWidth;
};

struct game_object
{
    object_type Type;
    object_model Model;
    vec_2 Midpoint;
    float X_Momentum;
    float Y_Momentum;
    float OffsetAngle;
    float AngularMomentum;
};

static game_state
{
    game_object *Player;
};

// Thoughts for allocating into this game memory...
// Need to allocate a chunk of space for the asteroids somehow (even just VirtualAlloc), and assign the resulting pointer to Memory.Asteroids
// Then, to create an asteroid, build a new asteroid within a function, then do a memcpy(), where the location is equal
// to GameMemory.Asteroids + NumAsteroids, the length is sizeof(game_object), etc.
// To iterate through the asteroids, use the NumAsteroids/pointer arithmetic again.
// When despawning an asteroid, have to consolidate the memory. Simply do a memcpy() from the last asteroid to where the
// asteroid was despawned, then decrement the NumAsteroids so the next asteroid to be added will get added to where the last one
// used to be.

static game_state GameState;
static game_object Player;
static object_model P_Model;
static game_object Asteroid;
static object_model A_Model;


// Note that this may not be portable, as it relies upon the way Windows structures
// bitmap data in memory.
void SetPixelInBuffer(platform_bitmap_buffer *Buffer, vec_2 *Coords, color_triple *Colors, float Brightness)
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

void SetObjectModelForDraw(game_object *Object)
{
    float Theta = Object->OffsetAngle;
    for (int i = 0; i < Object->Model.NumVertices; ++i)
    {
        float X_Orig = Object->Model.StartVertices[i].X;
        float Y_Orig = Object->Model.StartVertices[i].Y;
        Object->Model.DrawVertices[i].X = (X_Orig * cos(Theta)) - (Y_Orig * sin(Theta));
        Object->Model.DrawVertices[i].Y = (X_Orig * sin(Theta)) + (Y_Orig * cos(Theta));
    }
}

inline void HandleObjectEdgeWarping(game_object *Object, int Width, int Height)
{
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

    for (int i = 0; i < Object->Model.NumVertices - 1; ++i)
    {
        Cur.X = Object->Model.DrawVertices[cur].X + Object->Midpoint.X;
        Cur.Y = Object->Model.DrawVertices[cur].Y + Object->Midpoint.Y;
        Next.X = Object->Model.DrawVertices[next].X + Object->Midpoint.X;
        Next.Y = Object->Model.DrawVertices[next].Y + Object->Midpoint.Y;
        DrawLineWidth(Buffer, &Cur, &Next, &(Object->Model.Color), Object->Model.LineWidth);
        ++cur;
        if (i < 2) ++next;
    }
    Cur.X = Object->Model.DrawVertices[cur].X + Object->Midpoint.X;
    Cur.Y = Object->Model.DrawVertices[cur].Y + Object->Midpoint.Y;
    Next.X = Object->Model.DrawVertices[0].X + Object->Midpoint.X;
    Next.Y = Object->Model.DrawVertices[0].Y + Object->Midpoint.Y;
    DrawLineWidth(Buffer, &Cur, &Next, &(Object->Model.Color), Object->Model.LineWidth);
}

void UpdateGameAndRender(game_memory *Memory, platform_bitmap_buffer *OffscreenBuffer, platform_sound_buffer *SoundBuffer, platform_player_input *PlayerInput)
{
    if (!Memory->IsInitialized)
    {
        // Initialize game memory here!
        Player = {};
        P_Model = {};
        Player.Type = PLAYER;
        Player.Model = P_Model;
        Player.Model.LineWidth = 1.5f;

        Player.Midpoint.X = OffscreenBuffer->Width / 2;
        Player.Midpoint.Y = OffscreenBuffer->Height / 10;
        vec_2 PlayerLeft, PlayerTop, PlayerRight, PlayerBottom;
        PlayerLeft.X = -20.0f, PlayerLeft.Y = -20.0f;
        PlayerTop.X = 0.0f, PlayerTop.Y = 40.0f;
        PlayerRight.X = 20.0f, PlayerRight.Y = -20.0f;
        PlayerBottom.X = 0.0f, PlayerBottom.Y = 0.0f;
        Player.Model.StartVertices[0] = PlayerLeft, Player.Model.StartVertices[1] = PlayerTop, Player.Model.StartVertices[2] = PlayerRight, Player.Model.StartVertices[3] = PlayerBottom;
		Player.Model.DrawVertices[0] = PlayerLeft, Player.Model.DrawVertices[1] = PlayerTop, Player.Model.DrawVertices[2] = PlayerRight, Player.Model.DrawVertices[3] = PlayerBottom;

		Player.Model.Color.Red = 100, Player.Model.Color.Blue = 100, Player.Model.Color.Green = 100;
        Player.AngularMomentum = 0.1f;

        Asteroid = {};
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
        Asteroid.AngularMomentum = 0.005f;

        Memory->IsInitialized = true;
    }

    if (PlayerInput->Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput->B_Pressed)
    {
        Player.Model.Color.Red = 0, Player.Model.Color.Blue = 200;
    }

    Player.OffsetAngle += Player.AngularMomentum * (PlayerInput->LTrigger + PlayerInput->RTrigger);
    Asteroid.OffsetAngle += Asteroid.AngularMomentum;

    Player.X_Momentum += (PlayerInput->NormalizedLX * PlayerInput->Magnitude) * .5f;
    Player.Y_Momentum += (PlayerInput->NormalizedLY * PlayerInput->Magnitude) * .5f;

    Player.Midpoint.X += Player.X_Momentum;
    Player.Midpoint.Y += Player.Y_Momentum;

    Asteroid.Midpoint.X += Asteroid.X_Momentum;
    Asteroid.Midpoint.Y += Asteroid.Y_Momentum;

    HandleObjectEdgeWarping(&Asteroid, OffscreenBuffer->Width, OffscreenBuffer->Height);
    HandleObjectEdgeWarping(&Player, OffscreenBuffer->Width, OffscreenBuffer->Height);
    SetObjectModelForDraw(&Asteroid);
    SetObjectModelForDraw(&Player);
    DrawObjectModelInBuffer(OffscreenBuffer, &Asteroid);
    DrawObjectModelInBuffer(OffscreenBuffer, &Player);
}