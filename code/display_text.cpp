#pragma once

#include "display_text.h"
#include "common.h"
#include "platform.h"
#include "render.h"

void PrintDisplayText(display_text *Text, platform_bitmap_buffer *Bitmap)
{
    if (!Text->IsVisible || (Text->IsPermanent || Text->NumFramesToDisplay > 0))
    {
        return;
    }

    if (Text->HasBorder)
    {
        vec_2 BorderCoords[4];

        BorderCoords[0].X = Text->WorldLocation.X + Text->BorderPadding.X;
        BorderCoords[0].Y = Text->WorldLocation.Y + Text->BorderPadding.Y;

        BorderCoords[1].X = Text->WorldLocation.X + Text->BoxDimensions.X - (2 * Text->BorderPadding.X);
        BorderCoords[1].Y = BorderCoords[0].Y;

        BorderCoords[2].X = BorderCoords[1].X;
        BorderCoords[2].Y = Text->WorldLocation.Y + Text->BoxDimensions.Y - (2 * Text->BorderPadding.Y);

        BorderCoords[3].X = BorderCoords[0].X;
        BorderCoords[3].Y = BorderCoords[2].Y;

        for (uint32_t i = 0; i < 3; i++)
        {
            DrawLineSegmentWithWidth(Bitmap, &(BorderCoords[i]), &(BorderCoords[i + 1]), &(Text->BorderColor), Text->BorderWidth);
        }

        DrawLineSegmentWithWidth(Bitmap, &(BorderCoords[3]), &(BorderCoords[0]), &(Text->BorderColor), Text->BorderWidth);
    }

    
}