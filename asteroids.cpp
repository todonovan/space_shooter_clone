#include <windows.h>
#include <stdint.h>
#include <math.h>
#include <xinput.h>

#include "platform.h"

struct Vec2
{
    float X;
    float Y;
};

struct ColorTriple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};

struct PlayerModel
{
    Vec2 Vertices[4];
    ColorTriple Color;
    float LineWidth;
};

struct PlayerObject
{
    PlayerModel *Model;
    Vec2 Midpoint;
    float X_Momentum;
    float Y_Momentum;
    float AngularMomentum;
};

void ClearBuffer()
{
    if (!BitmapMemory) return;
	ZeroMemory(BitmapMemory, BitmapHeight * BitmapWidth * 4);
}

void SetPixelInBuffer(Vec2 *Coords, ColorTriple *Colors, int WindowWidth, int WindowHeight)
{
    if (!BitmapMemory) return;
    int X = (Coords->X < 0) ? ((int)Coords->X + WindowWidth) : ((int)Coords->X % WindowWidth);
    int Y = (Coords->Y < 0) ? ((int)Coords->Y + WindowHeight) : ((int)Coords->Y % WindowHeight);

    uint8_t *Pixel = (uint8_t *)((char *)BitmapMemory + (Y * BitmapWidth * 4) + (X * 4));

    *Pixel = Colors->Blue;
    Pixel++;
    *Pixel = Colors->Green;
    Pixel++;
    *Pixel = Colors->Red;
}

/// TODO!! Restore the actual width capabilities (e.g., reducing brightness incrementally further away pixel is from center of line)
void DrawLineWidth(Vec2 *Point1, Vec2 *Point2, ColorTriple *Color, int WindowWidth, int WindowHeight)
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
        Vec2 Coords;
        Coords.X = x0;
        Coords.Y = y0;
        // To restore the anti-aliasing, this proc would have to take a param
        // that represented the amount to scale down the 'brightness' of the pixel.
        SetPixelInBuffer(&Coords, Color, WindowWidth, WindowHeight);
        e2 = err; x2 = x0;
        if (2*e2 >= -dx)
        {
            for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                Vec2 Coords2;
                Coords2.X = x0;
                Coords2.Y = y2 += sy;
                
                SetPixelInBuffer(&Coords2, Color, WindowWidth, WindowHeight);
            }            
            if (x0 == x1) break;
            e2 = err; err -= dy; x0 += sx; 
        } 
        if (2*e2 <= dy)
        {
            for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
            {
                Vec2 Coords2;
                Coords2.X = x2 += sx;
                Coords2.Y = y0;
                SetPixelInBuffer(&Coords2, Color, WindowWidth, WindowHeight);
            }
            if (y0 == y1) break;
            err += dx; y0 += sy; 
        }
    }
}

void RotatePlayerModel(bool CounterClockwise)
{
    char RotateSign = 1;
    if (!CounterClockwise) RotateSign *= -1;

    for (int i = 0; i < 4; ++i)
    {
        float Theta = Player.AngularMomentum * RotateSign;
        float X_Orig = Player.Model->Vertices[i].X;
        float Y_Orig = Player.Model->Vertices[i].Y;
        Player.Model->Vertices[i].X = (X_Orig * cos(Theta)) - (Y_Orig * sin(Theta));
        Player.Model->Vertices[i].Y = (X_Orig * sin(Theta)) + (Y_Orig * cos(Theta));
    }
}

void HandlePlayerEdgeWarping(int WindowWidth, int WindowHeight)
{
    if (Player.Midpoint.X < 0)
    {
        Player.Midpoint.X += WindowWidth;
    }
    else if (Player.Midpoint.X >= WindowWidth)
    {
        Player.Midpoint.X -= WindowWidth;
    }
    if (Player.Midpoint.Y < 0)
    {
        Player.Midpoint.Y += WindowHeight;
    }
    else if (Player.Midpoint.Y >= WindowHeight)
    {
        Player.Midpoint.Y -= WindowHeight;
    }
}

void DrawPlayer(int WindowWidth, int WindowHeight)
{
    if (!BitmapMemory) return;

    int cur = 0, next = 1;
    Vec2 Cur, Next;

    for (int i = 0; i < 3; ++i)
    {
        Cur.X = Player.Model->Vertices[cur].X + Player.Midpoint.X;
        Cur.Y = Player.Model->Vertices[cur].Y + Player.Midpoint.Y;
        Next.X = Player.Model->Vertices[next].X + Player.Midpoint.X;
        Next.Y = Player.Model->Vertices[next].Y + Player.Midpoint.Y;
        DrawLineWidth(&Cur, &Next, &(Player.Model->Color), WindowWidth, WindowHeight);
        ++cur;
        if (i < 2) ++next;
    }
    Cur.X = Player.Model->Vertices[cur].X + Player.Midpoint.X;
    Cur.Y = Player.Model->Vertices[cur].Y + Player.Midpoint.Y;
    Next.X = Player.Model->Vertices[0].X + Player.Midpoint.X;
    Next.Y = Player.Model->Vertices[0].Y + Player.Midpoint.Y;
    DrawLineWidth(&Cur, &Next, &(Player.Model->Color), WindowWidth, WindowHeight);
}

void RenderGame(HWND WindHandle, HDC WindowDC)
{
    ClearBuffer();
    RECT ClientRect;
    GetClientRect(WindHandle, &ClientRect);
    int Width = ClientRect.right - ClientRect.left, Height = ClientRect.bottom - ClientRect.top;
    DrawToWindow(WindowDC, &ClientRect, Width, Height);
}

void UpdateGameState(XINPUT_GAMEPAD *Controller, long WindowWidth, long WindowHeight)
{
    PlayerControlInput PlayerInput;
    float LX = Controller->sThumbLX;
    float LY = Controller->sThumbLY;
    float magnitude = sqrt(LX*LX + LY*LY);

    PlayerInput.NormalizedLX = LX / magnitude;
    PlayerInput.NormalizedLY = LY / magnitude;

    float normalizedMagnitude = 0;

    if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        if (magnitude > 32767) magnitude = 32767;
        magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        normalizedMagnitude = magnitude / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }
    else
    {
        magnitude = 0.0;
        normalizedMagnitude = 0.0;
    }
    PlayerInput.Magnitude = normalizedMagnitude;

    PlayerInput.A_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_A);
    PlayerInput.B_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_B);
    PlayerInput.LTrigger_Pressed = Controller->bLeftTrigger > 50;
    PlayerInput.RTrigger_Pressed = Controller->bRightTrigger > 50;
    PlayerInput.Start_Pressed = (Controller->wButtons & XINPUT_GAMEPAD_START);

    if (PlayerInput.Start_Pressed)
    {
        PostQuitMessage(0);
    }
    if (PlayerInput.B_Pressed)
    {
        Player.Model->Color.Red = 0, Player.Model->Color.Blue = 200;
    }
    if (PlayerInput.LTrigger_Pressed)
    {
        RotatePlayerModel(true);
    }
    if (PlayerInput.RTrigger_Pressed)
    {
        RotatePlayerModel(false);
    }
    
    Player.X_Momentum += (PlayerInput.NormalizedLX * PlayerInput.Magnitude) * .5f;
    Player.Y_Momentum += (PlayerInput.NormalizedLY * PlayerInput.Magnitude) * .5f;

    Player.Midpoint.X += Player.X_Momentum;
    Player.Midpoint.Y += Player.Y_Momentum;
    HandlePlayerEdgeWarping(WindowWidth, WindowHeight);
}