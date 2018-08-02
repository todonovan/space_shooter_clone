#include "platform.h"
#include "geometry.h"
#include "input.h"

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

void TranslatePlatformInputToGame(asteroids_player_input *Dest, platform_player_input *ThisFrame, platform_player_input *LastFrame)
{
    Dest->A_Down = ThisFrame->A_Pressed;
    Dest->A_DownThisFrame = (ThisFrame->A_Pressed && !LastFrame->A_Pressed);

    Dest->B_Down = ThisFrame->B_Pressed;
    Dest->B_DownThisFrame = (ThisFrame->B_Pressed && !LastFrame->B_Pressed);

    Dest->Start_Down = ThisFrame->Start_Pressed;
    Dest->Start_DownThisFrame = (ThisFrame->Start_Pressed && !LastFrame->Start_Pressed);

    float raw_ltrig = ThisFrame->LTrigger > ThisFrame->TriggerDeadzone ? (float)ThisFrame->LTrigger : 0.0f;
    Dest->LTrigger = (raw_ltrig - ThisFrame->TriggerDeadzone) / (ThisFrame->TriggerMax - ThisFrame->TriggerDeadzone);
    float raw_rtrig = ThisFrame->RTrigger > ThisFrame->TriggerDeadzone ? (float)ThisFrame->RTrigger : 0.0f;
    Dest->RTrigger = (raw_rtrig - ThisFrame->TriggerDeadzone) / (ThisFrame->TriggerMax - ThisFrame->TriggerDeadzone);

    thumbstick raw_stick = {};

    vec_2 left_stick_vec;
    left_stick_vec.X = ThisFrame->LX;
    left_stick_vec.Y = ThisFrame->LY;

    raw_stick.StickVector_Normalized = Normalize(left_stick_vec);
    raw_stick.Magnitude = Magnitude(left_stick_vec);

    if (raw_stick.Magnitude > ThisFrame->StickDeadzone)
    {
        if (raw_stick.Magnitude > ThisFrame->MaxMagnitude)
        {
            raw_stick.Magnitude = (float)ThisFrame->MaxMagnitude;
        }
        raw_stick.Magnitude = (raw_stick.Magnitude - ThisFrame->StickDeadzone) / (ThisFrame->MaxMagnitude - ThisFrame->StickDeadzone);
    }
    else
    {
        raw_stick.Magnitude = 0.0f;
    }

    Dest->LeftStickHistory = LastFrame->LeftStickHistory;
    UpdateStick(Dest, &raw_stick);
}