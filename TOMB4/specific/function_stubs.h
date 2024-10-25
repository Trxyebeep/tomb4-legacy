#pragma once
#include "../global/types.h"

void other_log(char* format, ...);
void S_ExitSystem(const char* string);
long GetRandomControl();
void SeedRandomControl(long seed);
long GetRandomDraw();
void SeedRandomDraw(long seed);
void init_game_malloc();
void* game_malloc(long size, long type = 0);
void Log(ulong type, const char* s, ...);

extern PHD_VECTOR CamPos;
extern PHD_VECTOR CamRot;
extern long nPolyType;
extern char* malloc_buffer;
extern char* malloc_ptr;
extern long malloc_size;
extern long malloc_free;
