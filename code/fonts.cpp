#pragma once

#include "fonts.h"
#include "common.h"
#include "asteroids.h"
#include "memory.h"

#include "stdlib.h"

void ClearParseBuffer(uint8_t *Buffer, uint32_t Size)
{
    for (uint32_t i = 0; i < Size; i++)
    {
        Buffer[i] = 0;
    }
}

// In hindsight, creating a bizarre custom file structure for this stuff was maybe
// a bad choice, although I'm not sure what other choice I really had. 

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                          FONT FILE STRUCTURE                                *
 *      Dimensions.X in numeric chars, null terminated (two or three bytes)    *
 *      Dimensions.Y in numeric chars, null terminated (two or three bytes)    *
 *          ** For each font_char in NUM_FONT_CHARS **                         *
 *      Dimensions.X * Dimensions.Y numeric chars, null terminated             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

font_file_parse_metadata ParseFontData(font_face *FontFace, char *Buffer, uint64_t BufferSize)
{
    uint8_t *Cursor = (uint8_t *)Buffer;
    uint8_t ParseBuffer[1024];
    uint32_t ParseBufferIndex = 0;
    font_file_parse_metadata Metadata;

    // Parse X coord for char dimensions
    while (*Cursor != '\0')
    {
        ParseBuffer[ParseBufferIndex++] = *Cursor;
        Cursor++;
    }

    ParseBuffer[ParseBufferIndex] = '\0';
    Metadata.CharDimensions.X = (float)atoi((char *)&ParseBuffer);
    Cursor++;

    ClearParseBuffer(ParseBuffer, 1024);

    // Parse Y coord for char dimensions
    while (*Cursor != '\0')
    {
        ParseBuffer[ParseBufferIndex++] = *Cursor;
        Cursor++;
    }

    ParseBuffer[ParseBufferIndex] = '\0';
    Metadata.CharDimensions.Y = (float)atoi((char *)&ParseBuffer);
    Cursor++;

    ClearParseBuffer(ParseBuffer, 1024);

    // Loop through the font chars in file. We know there will be Dims.X * Dims.Y chars to check
    for (uint32_t i = 0; i < NUM_FONT_CHARS; i++)
    {
        font_char *CurrentChar = &FontFace->Characters[i];
        CurrentChar->CharHeight = Metadata.CharDimensions.X;
        CurrentChar->CharWidth = Metadata.CharDimensions.Y;

        CurrentChar->Font = FontFace;
        CurrentChar->Value = (char)(i + OFFSET_FROM_ASCII);
            
        uint8_t *Foxels = CurrentChar->Foxels;
        // For each 'character,' iterate through the X * Y numeric chars, setting the value in font_char appropriately
        
        for (uint32_t j = 0; j < Metadata.CharDimensions.X * Metadata.CharDimensions.Y; j++)
        {
            if (*Cursor == '1')
            {
                Foxels[j] = 1;
            }
            else
            {
                Foxels[j] = 0;
            }

            Cursor++;
        }

        HackyAssert(*Cursor == '\0');
        Cursor++;
    }

    return Metadata;
}

void LoadFontFile(font_face *FontFace, char *FileName, uint32_t Scale)
{
    uint64_t FileSize = PlatformGetFileSize((LPCSTR)FileName);
    char *Buffer = (char *)AllocateIntoTempMemory(FileSize);
    RequestResourceLoad((LPCSTR)FileName, Buffer, FileSize);
    font_file_parse_metadata Metadata = ParseFontData(FontFace, Buffer, FileSize);
    FontFace->UnscaledCharDimensions = Metadata.CharDimensions;
    FontFace->Scale = Scale;
}

font_char * GetChar(char AsciiCode, font_face *Font)
{
    char FontCode = AsciiCode - OFFSET_FROM_ASCII;
    return &(Font->Characters[FontCode]);
}