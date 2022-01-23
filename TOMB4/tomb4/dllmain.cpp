#include "../tomb4/pch.h"
#include "../game/control.h"
#include "../game/lara2gun.h"
#include "../game/laramisc.h"
#include "../game/laraswim.h"
#include "../game/collide.h"
#include "../game/lara.h"
#include "../game/lara1gun.h"
#include "../game/bike.h"
#include "../specific/LoadSave.h"
#include "../specific/output.h"
#include "../game/init.h"
#include "../game/objects.h"
#include "../game/tomb4fx.h"
#include "../specific/specificfx.h"
#include "../specific/lighting.h"
#include "../game/train.h"
#include "../game/laraflar.h"
#include "../game/camera.h"
#include "../specific/3dmath.h"
#include "../game/gameflow.h"
#include "../game/delstuff.h"
#include "../game/deltapak.h"
#include "../game/hair.h"
#include "../game/draw.h"
#include "../specific/drawroom.h"
#include "../game/box.h"
#include "../game/newinv.h"
#include "../game/footprnt.h"
#include "../game/larasurf.h"
#include "../game/laraclmb.h"

HINSTANCE hinstance = NULL;

#ifndef BUILD_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

DLL_EXPORT int dummyz();

int dummyz()
{
	return 0;
}

void inject_all(bool replace)
{
	inject_control(replace);
	inject_lara2gun(replace);
	inject_laramisc(replace);
	inject_laraswim(replace);
	inject_collide(replace);
	inject_lara(replace);
	inject_lara1gun(replace);
	inject_bike(replace);
	inject_loadsave(replace);
	inject_output(replace);
	inject_init(replace);
	inject_objects(replace);
	inject_tomb4fx(replace);
	inject_specificfx(replace);
	inject_lighting(replace);
	inject_train(replace);
	inject_laraflar(replace);
	inject_camera(replace);
	inject_3dmath(replace);
	inject_gameflow(replace);
	inject_delstuff(replace);
	inject_deltapack(replace);
	inject_hair(replace);
	inject_draw(replace);
	inject_drawroom(replace);
	inject_box(replace);
	inject_newinv(replace);
	inject_footprint(replace);
	inject_larasurf(replace);
	inject_laraclmb(replace);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hinstance = hModule;
		inject_all(true);

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return 1;
}
