#pragma once

#include "common.h"

#define NUM_FONT_CHARS 95
#define OFFSET_FROM_ASCII 32 // We're skipping the first 32 chars in ASCII, i.e., the control chars

struct font_face;

struct font_char
{
    // Which ASCII char is this?
    char Value;

    // Deref this to get info such as, is fixed width?
    font_face *Font;

    // Height and width of character. Although my stuff
    // will likely only be fixed width, it might be ok
    // to support variable widths.
    uint32_t CharWidth;
    uint32_t CharHeight;

    // "Foxels" = "Fo"nt Pi"xels". Foxels[i] == 1 if it's set.
    uint8_t *Foxels;
};

struct font_face
{
    vec_2 UnscaledCharDimensions;
    uint32_t Scale;
    font_char Characters[NUM_FONT_CHARS];
};

struct font_file_parse_metadata
{
    vec_2 CharDimensions;
};

void LoadFontFace(font_face *FontFace, char *FileName, uint32_t Scale);
void InitFontChar(font_char *Char, font_face *FontFace);
font_char * GetChar(char AsciiCode);