#pragma once
#include "../global/types.h"

void S_DrawHealthBar(long pos);
void S_DrawAirBar(long pos);
void S_DrawDashBar(long pos);
void S_InitLoadBar(long maxpos);
void S_LoadBar();
void DoBar(long x, long y, long width, long height, long pos, long clr1, long clr2);
void DoOptions();
void DoStatScreen();
long S_DisplayPauseMenu(long reset);
long DoLoadSave(long LoadSave);
long S_LoadSave(long load_or_save, long mono);
void S_DrawTile(long x, long y, long w, long h, LPDIRECT3DTEXTUREX t, long tU, long tV, long tW, long tH, long c0, long c1, long c2, long c3);
void S_DisplayMonoScreen();
void CreateMonoScreen();
void FreeMonoScreen();
void RGBM_Mono(uchar* r, uchar* g, uchar* b);
void MemBltSurf(void* dest, long x, long y, long w, long h, long dadd, void* source, long x2, long y2, DDSURFACEDESCX surface, float xsize, float ysize);
void ConvertSurfaceToTextures(LPDIRECTDRAWSURFACEX surface);
void DoSlider(long x, long y, long width, long height, long pos, long clr1, long clr2, long clr3);
void CheckKeyConflicts();
long S_PauseMenu();
long GetSaveLoadFiles();

extern long sfx_frequencies[3];
extern long SoundQuality;
extern long MusicVolume;
extern long SFXVolume;
extern long ControlMethod;
extern char MonoScreenOn;
