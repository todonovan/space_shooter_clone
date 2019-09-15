#pragma once

#include "fonts.fwd.h"
#include "platform.fwd.h"

#include "display_text.h"
#include "common.h"
#include "geometry.h"


struct display_text
{
    vec_2 WorldLocation;
    vec_2 BoxDimensions;
    bool IsVisible;
    bool IsPermanent;
    uint32_t NumFramesToDisplay;

    bool HasBorder;
    color_triple BorderColor;
    uint32_t BorderWidth;
    vec_2 BorderPadding;

    vec_2 ContentPadding;
    char *Content;

    font_face *Font;
};

void PrintDisplayText(display_text *ToPrint, platform_bitmap_buffer *Bitmap);