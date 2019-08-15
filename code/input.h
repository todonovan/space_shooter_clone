#pragma once

// Forward-declarations
#include "input.fwd.h"
#include "common.fwd.h"
#include "platform.fwd.h"
#include "geometry.fwd.h"

#include "common.h"
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
    bool A_Down;
    bool A_DownThisFrame;

    bool B_Down;
    bool B_DownThisFrame;

    bool Start_Down;
    bool Start_DownThisFrame;

    float LTrigger;
    float RTrigger;

    thumbstick LeftStick;

    thumbstick_history LeftStickHistory;
};

void InitializePlayerInput(asteroids_player_input *Input);
void TranslatePlatformInputToGame(asteroids_player_input *Dest, platform_player_input *ThisFrame, asteroids_player_input *LastFrame);