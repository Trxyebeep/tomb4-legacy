#pragma once
#include "../global/types.h"

void InitFont();
void UpdatePulseColour();
long GetStringLength(const char* string, short* top, short* bottom);
void DrawChar(short x, short y, ushort col, CHARDEF* def);
void PrintString(ushort x, ushort y, uchar col, const char* string, ushort flags);

extern long small_font;
extern long font_height;
extern long GnFrameCounter;
