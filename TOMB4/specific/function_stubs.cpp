#include "../tomb4/pch.h"
#include "function_stubs.h"

PHD_VECTOR CamPos;
PHD_VECTOR CamRot;

long nPolyType;

char* malloc_buffer;
char* malloc_ptr;
long malloc_size;
long malloc_free;

static long malloc_used;

static long rand_1 = 0xD371F947;
static long rand_2 = 0xD371F947;

void other_log(char* format, ...)
{
	va_list arglist;
	char buffer[4096];

	va_start(arglist, format);
	vsprintf(buffer, format, arglist);
	Log(0, buffer);
}

void S_ExitSystem(const char* string)
{
	Log(0, "**** %s ****", string);
}

long GetRandomControl()
{
	rand_1 = 0x41C64E6D * rand_1 + 12345;
	return (rand_1 >> 10) & 0x7FFF;
}

void SeedRandomControl(long seed)
{
	rand_1 = seed;
}

long GetRandomDraw()
{
	rand_2 = 0x41C64E6D * rand_2 + 12345;
	return (rand_2 >> 10) & 0x7FFF;
}

void SeedRandomDraw(long seed)
{
	rand_2 = seed;
}

void init_game_malloc()
{
	malloc_buffer = (char*)malloc(MALLOC_SIZE);
	malloc_size = MALLOC_SIZE;
	malloc_ptr = malloc_buffer;
	malloc_free = MALLOC_SIZE;
	malloc_used = 0;
}

void* game_malloc(long size, long type)
{
	char* ptr;

	size = (size + 3) & -4;

	if (size > malloc_free)
	{
		Log(0, "OUT OF MEMORY");
		return 0;
	}
	else
	{
		ptr = malloc_ptr;
		malloc_free -= size;
		malloc_used += size;
		malloc_ptr += size;
		memset(ptr, 0, size);
		return ptr;
	}
}

void Log(ulong type, const char* s, ...)
{
	//not empty originally, obv
}
