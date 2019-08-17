#pragma once

#include "platform.h"
#include "geometry.h"
#include "input.h"

void UpdateStickHistory(thumbstick_history *History, thumbstick *Next)
{
    History->Buffer[History->ptr].Magnitude = Next->Magnitude;
    History->Buffer[History->ptr].StickVector_Normalized = Next->StickVector_Normalized;

    History->ptr++;
    if (History->ptr == INPUT_HISTORY_SZ) History->ptr = 0;
}

void UpdateStick(asteroids_player_input *Dest, thumbstick *RawNext)
{
    thumbstick_history *hist = &Dest->LeftStickHistory;
    UpdateStickHistory(hist, RawNext);

    float x_sum = 0.0f;
    float y_sum = 0.0f;
    float mag_sum = 0.0f;
    for (uint32_t i = 0; i < INPUT_HISTORY_SZ; i++)
    {
        x_sum += hist->Buffer[i].StickVector_Normalized.X;
        y_sum += hist->Buffer[i].StickVector_Normalized.Y;
        mag_sum += hist->Buffer[i].Magnitude;
    }
    Dest->LeftStick.StickVector_Normalized.X = x_sum / (float)INPUT_HISTORY_SZ;
    Dest->LeftStick.StickVector_Normalized.Y = y_sum / (float)INPUT_HISTORY_SZ;
    Dest->LeftStick.Magnitude = mag_sum / (float)INPUT_HISTORY_SZ;
}

void InitializePlayerInput(asteroids_player_input *Input)
{
    Input->LeftStickHistory = {};
    Input->LeftStickHistory.ptr = 0;
    for (uint32_t i = 0; i < INPUT_HISTORY_SZ; i++)
    {
        Input->LeftStickHistory.Buffer[i].StickVector_Normalized = {};
        Input->LeftStickHistory.Buffer[i].Magnitude = 0.0f;
    }
}

void TranslatePlatformInputToGame(asteroids_player_input *Dest, platform_player_input *ThisFrame, asteroids_player_input *LastFrame)
{
    Dest->A_Down = ThisFrame->A_Pressed;
    Dest->A_DownThisFrame = (ThisFrame->A_Pressed && !LastFrame->A_Down);

    Dest->B_Down = ThisFrame->B_Pressed;
    Dest->B_DownThisFrame = (ThisFrame->B_Pressed && !LastFrame->B_Down);

    Dest->Start_Down = ThisFrame->Start_Pressed;
    Dest->Start_DownThisFrame = (ThisFrame->Start_Pressed && !LastFrame->Start_Down);

    // Upscale triggers
    Dest->LTrigger = ThisFrame->NormalizedLTrigger * 1.25f;
    Dest->RTrigger = ThisFrame->NormalizedRTrigger * 1.25f;

    thumbstick NewStick = {};
    NewStick.Magnitude = ThisFrame->NormalizedMagnitude;
    NewStick.StickVector_Normalized.X = ThisFrame->NormalizedLX;
    NewStick.StickVector_Normalized.Y = ThisFrame->NormalizedLY;

    Dest->LeftStickHistory = LastFrame->LeftStickHistory;
    UpdateStick(Dest, &NewStick);
}