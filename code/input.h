#pragma once

#include <stdint.h>

#include "platform.h"
#include "geometry.h"

#define INPUT_HISTORY_SZ 3

struct thumbstick
{
    vec_2 StickVector_Normalized;
    float Magnitude;
};

struct thumbstick_history
{
    uint8_t ptr;
    thumbstick Buffer[INPUT_HISTORY_SZ];
};

struct asteroids_player_input
{
    bool32_t A_Down;
    bool32_t A_DownThisFrame;

    bool32_t B_Down;
    bool32_t B_DownThisFrame;

    bool32_t Start_Down;
    bool32_t Start_DownThisFrame;

    float LTrigger;
    float RTrigger;

    thumbstick LeftStick;

    thumbstick_history LeftStickHistory;
};

void InitializePlayerInput(asteroids_player_input *Input);
void TranslatePlatformInputToGame(asteroids_player_input *Dest, platform_player_input *ThisFrame, asteroids_player_input *LastFrame);