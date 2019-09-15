#pragma once

// Forward-declarations
#include "render.fwd.h"
#include "common.fwd.h"
#include "memory.fwd.h"
#include "platform.fwd.h"

#include "common.h"
#include "memory.h"
#include "platform.h"

void SetPixelInBuffer(platform_bitmap_buffer *Buffer, vec_2 *Coords, color_triple *Color);
void DrawLineSegmentWithWidth(platform_bitmap_buffer *Buffer, vec_2 *StartPoint, vec_2 *EndPoint, color_triple *Color, float LineWidth);
void RenderAllEntities(game_state *GameState, platform_bitmap_buffer *OffscreenBuffer);