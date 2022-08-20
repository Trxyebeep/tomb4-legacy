#include "../tomb4/pch.h"
#include "draw.h"
#include "../specific/3dmath.h"
#include "../specific/output.h"
#include "../specific/lighting.h"
#include "deltapak.h"
#include "lara_states.h"
#include "control.h"
#include "delstuff.h"
#include "../specific/specificfx.h"
#include "objects.h"
#include "../specific/function_stubs.h"
#include "train.h"
#include "tomb4fx.h"
#include "rope.h"
#include "sound.h"
#include "../specific/polyinsert.h"
#include "mirror.h"
#include "scarab.h"
#include "croc.h"
#include "health.h"
#include "items.h"
#include "effect2.h"
#include "camera.h"
#include "effects.h"
#include "lara.h"
#include "gameflow.h"

static BITE_INFO EnemyBites[2] =
{
	{0, -40, 272, 7},
	{0, -20, 180, 11}
};

STATIC_INFO static_objects[NUMBER_STATIC_OBJECTS];

long* IMptr;
long IM_rate;
long IM_frac;
long IMstack[indices_count * 64];

long current_room;
short no_rotation[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

long outside;

short SkyPos;
short SkyPos2;

ushort LightningRGB[3];
ushort LightningRGBs[3];
short LightningCount;
short dLightningRand;

static short LightningRand;
static short LightningSFXDelay = 0;

static long outside_top;
static long outside_left;
static long outside_right;
static long outside_bottom;

static long draw_room_list[128];
static long room_list_start = 0;
static long room_list_end = 0;
static long number_draw_rooms;
static short draw_rooms[200];
static short ClipRoomNum;

static long camera_underwater;

void InitInterpolate(long frac, long rate)
{
	IM_rate = rate;
	IM_frac = frac;
	IMptr = IMstack;
	memcpy(IMstack, phd_mxptr, 48);
}

void phd_PopMatrix_I()
{
	phd_PopMatrix();
	IMptr -= 12;
}

void phd_PushMatrix_I()
{
	phd_PushMatrix();
	memcpy(IMptr + 12, IMptr, 48);
	IMptr += 12;
}

void phd_RotY_I(short ang)
{
	long* ptr;

	phd_RotY(ang);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	phd_RotY(ang);
	phd_mxptr = ptr;
}

void phd_RotX_I(short ang)
{
	long* ptr;

	phd_RotX(ang);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	phd_RotX(ang);
	phd_mxptr = ptr;
}

void phd_RotZ_I(short ang)
{
	long* ptr;

	phd_RotZ(ang);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	phd_RotZ(ang);
	phd_mxptr = ptr;
}

void phd_TranslateRel_I(long x, long y, long z)
{
	long* ptr;

	phd_TranslateRel(x, y, z);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	phd_TranslateRel(x, y, z);
	phd_mxptr = ptr;
}

void phd_TranslateRel_ID(long x, long y, long z, long x2, long y2, long z2)
{
	long* ptr;

	phd_TranslateRel(x, y, z);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	phd_TranslateRel(x2, y2, z2);
	phd_mxptr = ptr;
}

void phd_RotYXZ_I(short y, short x, short z)
{
	long* ptr;

	phd_RotYXZ(y, x, z);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	phd_RotYXZ(y, x, z);
	phd_mxptr = ptr;
}

void gar_RotYXZsuperpack_I(short** pprot1, short** pprot2, long skip)
{
	long* ptr;

	gar_RotYXZsuperpack(pprot1, skip);
	ptr = phd_mxptr;
	phd_mxptr = IMptr;
	gar_RotYXZsuperpack(pprot2, skip);
	phd_mxptr = ptr;
}

void gar_RotYXZsuperpack(short** pprot, long skip)
{
	ushort* prot;

	while (skip)
	{
		prot = (ushort*)*pprot;

		if (prot[0] & 0xC000)
			*pprot += 1;
		else
			*pprot += 2;

		skip--;
	}

	prot = (ushort*)*pprot;

	switch (prot[0] >> 14)
	{
	case 0:
		phd_RotYXZpack((prot[0] << 16) + prot[1]);
		++*pprot;
		break;
		
	case 1:
		phd_RotX(short((prot[0] & 0xFFF) << 4));
		break;

	case 2:
		phd_RotY(short((prot[0] & 0xFFF) << 4));
		break;

	default:
		phd_RotZ(short((prot[0] & 0xFFF) << 4));
	}

	++*pprot;
}

void phd_PutPolygons_I(short* ptr, long clip)
{
	phd_PushMatrix();
	InterpolateMatrix();
	phd_PutPolygons(ptr, clip);
	phd_PopMatrix();
}

void InterpolateMatrix()
{
	if (IM_rate == 2 || (IM_frac == 2 && IM_rate == 4))
	{
		phd_mxptr[M00] = (phd_mxptr[M00] + IMptr[M00]) >> 1;
		phd_mxptr[M01] = (phd_mxptr[M01] + IMptr[M01]) >> 1;
		phd_mxptr[M02] = (phd_mxptr[M02] + IMptr[M02]) >> 1;
		phd_mxptr[M03] = (phd_mxptr[M03] + IMptr[M03]) >> 1;
		phd_mxptr[M10] = (phd_mxptr[M10] + IMptr[M10]) >> 1;
		phd_mxptr[M11] = (phd_mxptr[M11] + IMptr[M11]) >> 1;
		phd_mxptr[M12] = (phd_mxptr[M12] + IMptr[M12]) >> 1;
		phd_mxptr[M13] = (phd_mxptr[M13] + IMptr[M13]) >> 1;
		phd_mxptr[M20] = (phd_mxptr[M20] + IMptr[M20]) >> 1;
		phd_mxptr[M21] = (phd_mxptr[M21] + IMptr[M21]) >> 1;
		phd_mxptr[M22] = (phd_mxptr[M22] + IMptr[M22]) >> 1;
		phd_mxptr[M23] = (phd_mxptr[M23] + IMptr[M23]) >> 1;
	}
	else if (IM_frac == 1)
	{
		phd_mxptr[M00] += (IMptr[M00] - phd_mxptr[M00]) >> 2;
		phd_mxptr[M01] += (IMptr[M01] - phd_mxptr[M01]) >> 2;
		phd_mxptr[M02] += (IMptr[M02] - phd_mxptr[M02]) >> 2;
		phd_mxptr[M03] += (IMptr[M03] - phd_mxptr[M03]) >> 2;
		phd_mxptr[M10] += (IMptr[M10] - phd_mxptr[M10]) >> 2;
		phd_mxptr[M11] += (IMptr[M11] - phd_mxptr[M11]) >> 2;
		phd_mxptr[M12] += (IMptr[M12] - phd_mxptr[M12]) >> 2;
		phd_mxptr[M13] += (IMptr[M13] - phd_mxptr[M13]) >> 2;
		phd_mxptr[M20] += (IMptr[M20] - phd_mxptr[M20]) >> 2;
		phd_mxptr[M21] += (IMptr[M21] - phd_mxptr[M21]) >> 2;
		phd_mxptr[M22] += (IMptr[M22] - phd_mxptr[M22]) >> 2;
		phd_mxptr[M23] += (IMptr[M23] - phd_mxptr[M23]) >> 2;
	}
	else
	{
		phd_mxptr[M00] = IMptr[M00] - ((IMptr[M00] - phd_mxptr[M00]) >> 2);
		phd_mxptr[M01] = IMptr[M01] - ((IMptr[M01] - phd_mxptr[M01]) >> 2);
		phd_mxptr[M02] = IMptr[M02] - ((IMptr[M02] - phd_mxptr[M02]) >> 2);
		phd_mxptr[M03] = IMptr[M03] - ((IMptr[M03] - phd_mxptr[M03]) >> 2);
		phd_mxptr[M10] = IMptr[M10] - ((IMptr[M10] - phd_mxptr[M10]) >> 2);
		phd_mxptr[M11] = IMptr[M11] - ((IMptr[M11] - phd_mxptr[M11]) >> 2);
		phd_mxptr[M12] = IMptr[M12] - ((IMptr[M12] - phd_mxptr[M12]) >> 2);
		phd_mxptr[M13] = IMptr[M13] - ((IMptr[M13] - phd_mxptr[M13]) >> 2);
		phd_mxptr[M20] = IMptr[M20] - ((IMptr[M20] - phd_mxptr[M20]) >> 2);
		phd_mxptr[M21] = IMptr[M21] - ((IMptr[M21] - phd_mxptr[M21]) >> 2);
		phd_mxptr[M22] = IMptr[M22] - ((IMptr[M22] - phd_mxptr[M22]) >> 2);
		phd_mxptr[M23] = IMptr[M23] - ((IMptr[M23] - phd_mxptr[M23]) >> 2);
	}
}

void InterpolateArmMatrix(long* mx)
{
	phd_mxptr[M00] = mx[M00];
	phd_mxptr[M01] = mx[M01];
	phd_mxptr[M02] = mx[M02];
	phd_mxptr[M10] = mx[M10];
	phd_mxptr[M11] = mx[M11];
	phd_mxptr[M12] = mx[M12];
	phd_mxptr[M20] = mx[M20];
	phd_mxptr[M21] = mx[M21];
	phd_mxptr[M22] = mx[M22];

	if (IM_rate == 2 || (IM_frac == 2 && IM_rate == 4))
	{
		phd_mxptr[M03] = (phd_mxptr[M03] + IMptr[M03]) >> 1;
		phd_mxptr[M13] = (phd_mxptr[M13] + IMptr[M13]) >> 1;
		phd_mxptr[M23] = (phd_mxptr[M23] + IMptr[M23]) >> 1;
	}
	else if (IM_frac == 1)
	{
		phd_mxptr[M03] += (IMptr[M03] - phd_mxptr[M03]) >> 2;
		phd_mxptr[M13] += (IMptr[M13] - phd_mxptr[M13]) >> 2;
		phd_mxptr[M23] += (IMptr[M23] - phd_mxptr[M23]) >> 2;
	}
	else
	{
		phd_mxptr[M03] = IMptr[M03] - ((IMptr[M03] - phd_mxptr[M03]) >> 2);
		phd_mxptr[M13] = IMptr[M13] - ((IMptr[M13] - phd_mxptr[M13]) >> 2);
		phd_mxptr[M23] = IMptr[M23] - ((IMptr[M23] - phd_mxptr[M23]) >> 2);
	}
}

void S_InsertRoom(short room_number)
{
	ROOM_INFO* r;

	current_room = room_number;
	r = &room[room_number];
	phd_TranslateAbs(0, 0, 0);
	phd_left = r->left;
	phd_right = r->right;
	phd_top = r->top;
	phd_bottom = r->bottom;
	_InsertRoom(r);
}

void CalculateObjectLighting(ITEM_INFO* item, short* frame)
{
	long x, y, z;

	if (item->shade >= 0)
		S_CalculateStaticMeshLight(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->shade & 0x7FFF, &room[item->room_number]);
	else
	{
		phd_PushUnitMatrix();
		phd_mxptr[M03] = 0;
		phd_mxptr[M13] = 0;
		phd_mxptr[M23] = 0;
		phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
		phd_TranslateRel((frame[0] + frame[1]) >> 1, (frame[2] + frame[3]) >> 1, (frame[4] + frame[5]) >> 1);
		x = item->pos.x_pos + (phd_mxptr[M03] >> 14);
		y = item->pos.y_pos + (phd_mxptr[M13] >> 14);
		z = item->pos.z_pos + (phd_mxptr[M23] >> 14);
		phd_PopMatrix();
		current_item = item;
		item->il.item_pos.x = x;
		item->il.item_pos.y = y;
		item->il.item_pos.z = z;
		CalcAmbientLight(item);
		CreateLightList(item);
	}
}

void CalculateObjectLightingLara()
{
	PHD_VECTOR pos;
	short room_no;

	if (GLOBAL_playing_cutseq)
		CalculateObjectLightingLaraCutSeq();
	else
	{
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;

		if (lara_item->anim_number == ANIM_DUCKBREATHE || lara_item->anim_number == ANIM_ALL4S || lara_item->anim_number == ANIM_BREATH)
		{
			pos.x = lara_item->pos.x_pos;

			if (lara_item->anim_number == ANIM_BREATH)
				pos.y = lara_item->pos.y_pos - 512;
			else
				pos.y = lara_item->pos.y_pos - 192;

			pos.z = lara_item->pos.z_pos;
			room_no = lara_item->room_number;
			GetFloor(pos.x, pos.y, pos.z, &room_no);
		}
		else
		{
			GetLaraJointPos(&pos, 7);
			room_no = lara_item->room_number;
			GetFloor(pos.x, pos.y, pos.z, &room_no);
		}

		current_item = lara_item;
		lara_item->il.item_pos.x = pos.x;
		lara_item->il.item_pos.y = pos.y;
		lara_item->il.item_pos.z = pos.z;
		CalcAmbientLight(lara_item);
		CreateLightList(lara_item);
	}
}

void DrawAnimatingItem(ITEM_INFO* item)
{
	OBJECT_INFO* obj;
	BITE_INFO* bite;
	short** meshpp;
	long* bone;
	short* frm[2];
	short* data;
	short* rot;
	short* rot2;
	long frac, rate, clip, bit, rnd;

	frac = GetFrames(item, frm, &rate);
	obj = &objects[item->object_number];
	bite = &EnemyBites[obj->bite_offset];

	if (obj->shadow_size)
		S_PrintShadow(obj->shadow_size, frm[0], item);

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

	if (obj->object_mip && (obj + 1)->loaded && phd_mxptr[M23] >> 16 > obj->object_mip)
		obj++;

	if (item->object_number < ENEMY_JEEP || item->object_number > SETHA_MIP)
		calc_animating_item_clip_window(item, frm[0]);
	else
	{
		phd_left = 0;
		phd_right = phd_winwidth;
		phd_top = 0;
		phd_bottom = phd_winheight;
	}

	clip = S_GetObjectBounds(frm[0]);

	if (clip)
	{
		CalculateObjectLighting(item, frm[0]);

		if (!item->data)
			data = no_rotation;
		else
			data = (short*)item->data;

		bit = 1;
		meshpp = &meshes[obj->mesh_index];
		bone = &bones[obj->bone_index];

		if (frac)
		{
			InitInterpolate(frac, rate);
			phd_TranslateRel_ID(frm[0][6], frm[0][7], frm[0][8], frm[1][6], frm[1][7], frm[1][8]);
			rot = frm[0] + 9;
			rot2 = frm[1] + 9;
			gar_RotYXZsuperpack_I(&rot, &rot2, 0);

			if (item->mesh_bits & 1)
			{
				if (item->meshswap_meshbits & 1)
					phd_PutPolygons_I(meshpp[1], clip);
				else
					phd_PutPolygons_I(meshpp[0], clip);
			}

			meshpp += 2;

			for (int i = 0; i < obj->nmeshes - 1; i++, bone += 4, meshpp += 2)
			{
				if (bone[0] & 1)
					phd_PopMatrix_I();

				if (bone[0] & 2)
					phd_PushMatrix_I();

				phd_TranslateRel_I(bone[1], bone[2], bone[3]);
				gar_RotYXZsuperpack_I(&rot, &rot2, 0);

				if (bone[0] & 0x1C)
				{
					if (bone[0] & 8)
						phd_RotY_I(*data++);

					if (bone[0] & 4)
						phd_RotX_I(*data++);

					if (bone[0] & 16)
						phd_RotZ_I(*data++);
				}

				bit <<= 1;

				if (bit & item->mesh_bits)
				{
					if (bit & item->meshswap_meshbits)
						phd_PutPolygons_I(meshpp[1], clip);
					else
						phd_PutPolygons_I(meshpp[0], clip);
				}

				if (item->fired_weapon && i == bite->mesh_num - 1)
				{
					rnd = GetRandomDraw();
					phd_PushMatrix_I();
					phd_TranslateRel_I(bite->x, bite->y, bite->z);
					phd_RotYXZ_I(0, -0x3FFC, short((rnd << 14) + (rnd >> 2) - 4096));
					InterpolateMatrix();
					//empty func call here
					phd_PutPolygons(meshes[objects[GUN_FLASH].mesh_index], clip);
					phd_PopMatrix_I();
					item->fired_weapon--;
				}
			}
		}
		else
		{
			phd_TranslateRel(frm[0][6], frm[0][7], frm[0][8]);
			rot = frm[0] + 9;
			gar_RotYXZsuperpack(&rot, 0);

			if (item->mesh_bits & 1)
			{
				if (item->meshswap_meshbits & 1)
					phd_PutPolygons(meshpp[1], clip);
				else
					phd_PutPolygons(meshpp[0], clip);
			}

			meshpp += 2;

			for (int i = 0; i < obj->nmeshes - 1; i++, bone += 4, meshpp += 2)
			{
				if (bone[0] & 1)
					phd_PopMatrix();

				if (bone[0] & 2)
					phd_PushMatrix();

				phd_TranslateRel(bone[1], bone[2], bone[3]);
				gar_RotYXZsuperpack(&rot, 0);

				if (bone[0] & 0x1C)
				{
					if (bone[0] & 8)
						phd_RotY(*data++);

					if (bone[0] & 4)
						phd_RotX(*data++);

					if (bone[0] & 16)
						phd_RotZ(*data++);
				}

				bit <<= 1;

				if (bit & item->mesh_bits)
				{
					if (bit & item->meshswap_meshbits)
						phd_PutPolygons(meshpp[1], clip);
					else
						phd_PutPolygons(meshpp[0], clip);
				}

				if (item->fired_weapon && i == bite->mesh_num - 1)
				{
					phd_PushMatrix();
					phd_RotX(-16380);
					phd_TranslateRel(bite->x, bite->y, bite->z);
					//empty func call here
					phd_PutPolygons(meshes[objects[GUN_FLASH].mesh_index], clip);
					phd_PopMatrix();
					item->fired_weapon--;
				}
			}
		}
	}

	phd_left = 0;
	phd_top = 0;
	phd_right = phd_winwidth;
	phd_bottom = phd_winheight;
	phd_PopMatrix();
}

void DrawRooms(short CurrentRoom)
{
	ROOM_INFO* r;
	long lx, ly, lz;
	short lr;

	current_room = CurrentRoom;
	r = &room[CurrentRoom];
	r->test_left = 0;
	r->test_top = 0;
	phd_left = 0;
	phd_top = 0;
	phd_right = phd_winxmax;
	phd_bottom = phd_winymax;
	r->test_right = phd_winxmax;
	r->test_bottom = phd_winymax;
	outside = r->flags & ROOM_OUTSIDE;
	camera_underwater = r->flags & ROOM_UNDERWATER;
	r->bound_active = 2;
	draw_room_list[0] = CurrentRoom;
	room_list_start = 0;
	room_list_end = 1;
	number_draw_rooms = 0;

	if (outside)
	{
		outside_top = 0;
		outside_left = 0;
		outside_right = phd_winxmax;
		outside_bottom = phd_winymax;
	}
	else
	{
		outside_left = phd_winxmax;
		outside_top = phd_winymax;
		outside_bottom = 0;
		outside_right = 0;
	}

	GetRoomBounds();
	InitialiseFogBulbs();
	CreateFXBulbs();

	if (outside)	//inlined SkyDrawPhase? did it exist?
	{
		if (!objects[HORIZON].loaded)
			outside = -1;
		else
		{
			if (BinocularRange)
				AlterFOV(14560 - (short)BinocularRange);

			phd_PushMatrix();
			phd_TranslateAbs(camera.pos.x, camera.pos.y, camera.pos.z);

			if (gfLevelFlags & GF_LIGHTNING)
			{
				if (!LightningCount && !LightningRand)
				{
					if (!(GetRandomDraw() & 127))
					{
						LightningCount = (GetRandomDraw() & 0x1F) + 16;
						dLightningRand = (GetRandomDraw() & 0xFF) + 256;
						LightningSFXDelay = (GetRandomDraw() & 3) + 12;
					}
				}
				else
				{
					UpdateSkyLightning();

					if (LightningSFXDelay > -1)
						LightningSFXDelay--;

					if (!LightningSFXDelay)
						SoundEffect(SFX_THUNDER_RUMBLE, 0, SFX_DEFAULT);
				}
			}

			nPolyType = 6;
			phd_PushMatrix();

			if (gfLevelFlags & GF_LAYER1)
			{
				phd_RotY(32760);

				if (gfLevelFlags & GF_LIGHTNING)
					DrawFlatSky(RGBA(LightningRGB[0], LightningRGB[1], LightningRGB[2], 44), SkyPos, -1536, 4);
				else
					DrawFlatSky(*(ulong*)&gfLayer1Col, SkyPos, -1536, 4);
			}

			if (gfLevelFlags & GF_LAYER2)
				DrawFlatSky(0xFF000000 | *(ulong*)&gfLayer2Col, SkyPos2, -1536, 2);

			if (gfLevelFlags & GF_LAYER1 || gfLevelFlags & GF_LAYER2)
				OutputSky();

			phd_PopMatrix();

			if (gfLevelFlags & GF_HORIZON)
			{
				phd_PutPolygonSkyMesh(meshes[objects[HORIZON].mesh_index], -1);
				OutputSky();
			}

			phd_PopMatrix();

			if (BinocularRange)
				AlterFOV(7 * (2080 - (short)BinocularRange));
		}
	}

	if (objects[LARA].loaded)
	{
		if (!(lara_item->flags & IFL_INVISIBLE))
		{
			nPolyType = 4;

			if (lara_item->mesh_bits)
			{
				DrawLara(lara_item, 0);
				phd_PushMatrix();

				if (lara.right_arm.flash_gun)
				{
					phd_mxptr[M00] = lara_matrices[132 + M00];
					phd_mxptr[M01] = lara_matrices[132 + M01];
					phd_mxptr[M02] = lara_matrices[132 + M02];
					phd_mxptr[M03] = lara_matrices[132 + M03];
					phd_mxptr[M10] = lara_matrices[132 + M10];
					phd_mxptr[M11] = lara_matrices[132 + M11];
					phd_mxptr[M12] = lara_matrices[132 + M12];
					phd_mxptr[M13] = lara_matrices[132 + M13];
					phd_mxptr[M20] = lara_matrices[132 + M20];
					phd_mxptr[M21] = lara_matrices[132 + M21];
					phd_mxptr[M22] = lara_matrices[132 + M22];
					phd_mxptr[M23] = lara_matrices[132 + M23];
					SetGunFlash(lara.gun_type);
				}

				if (lara.left_arm.flash_gun)
				{
					phd_mxptr[M00] = lara_matrices[168 + M00];
					phd_mxptr[M01] = lara_matrices[168 + M01];
					phd_mxptr[M02] = lara_matrices[168 + M02];
					phd_mxptr[M03] = lara_matrices[168 + M03];
					phd_mxptr[M10] = lara_matrices[168 + M10];
					phd_mxptr[M11] = lara_matrices[168 + M11];
					phd_mxptr[M12] = lara_matrices[168 + M12];
					phd_mxptr[M13] = lara_matrices[168 + M13];
					phd_mxptr[M20] = lara_matrices[168 + M20];
					phd_mxptr[M21] = lara_matrices[168 + M21];
					phd_mxptr[M22] = lara_matrices[168 + M22];
					phd_mxptr[M23] = lara_matrices[168 + M23];
					SetGunFlash(lara.gun_type);
				}

				phd_PopMatrix();
				DrawGunflashes();
			}

			if (gfLevelFlags & GF_MIRROR && lara_item->room_number == gfMirrorRoom)
				Draw_Mirror_Lara();
		}
	}

	InitDynamicLighting();
	nPolyType = 0;

	for (int i = 0; i < MAX_DYNAMICS; i++)
	{
		if (dynamics[i].on)
		{
			if (dynamics[i].x < 0)
				dynamics[i].x = 0;

			if (dynamics[i].z < 0)
				dynamics[i].z = 0;
		}
	}

	for (int i = 0; i < number_draw_rooms; i++)
		S_InsertRoom(draw_rooms[i]);

	if (gfLevelFlags & GF_TRAIN)
		DrawTrainFloor();

	DrawGunshells();
	nPolyType = 3;

	if (GLOBAL_playing_cutseq)
		DrawCutSeqActors();

	DrawRopeList();
	S_DrawSparks();
	lx = lara_item->pos.x_pos;
	ly = lara_item->pos.y_pos;
	lz = lara_item->pos.z_pos;
	lr = lara_item->room_number;
	lara_item->pos.x_pos = camera.pos.x;
	lara_item->pos.y_pos = camera.pos.y;
	lara_item->pos.z_pos = camera.pos.z;
	lara_item->room_number = camera.pos.room_number;
	DoUwEffect();
	S_DrawFires();
	S_DrawSmokeSparks();
	S_DrawSplashes();
	DrawBubbles();
	DrawDebris();
	DrawBlood();
	DrawDrips();
	DrawShockwaves();
	DrawScarabs();
	DrawLocusts();
	DrawLightning();
	lara_item->pos.x_pos = lx;
	lara_item->pos.y_pos = ly;
	lara_item->pos.z_pos = lz;
	lara_item->room_number = lr;

	if (gfLevelFlags & GF_LENSFLARE)
		SetUpLensFlare(gfLensFlare.x, gfLensFlare.y - 4096, gfLensFlare.z, 0);

	if (LaserSightActive)
		DrawLaserSightSprite();

	for (int i = 0; i < number_draw_rooms; i++)
		PrintObjects(draw_rooms[i]);
}

void RenderIt(short CurrentRoom)
{
	ROOM_INFO* r;

	current_room = CurrentRoom;
	r = &room[CurrentRoom];
	r->test_left = 0;
	r->test_top = 0;
	phd_left = 0;
	phd_top = 0;
	phd_right = phd_winxmax;
	phd_bottom = phd_winymax;
	r->test_right = phd_winxmax;
	r->test_bottom = phd_winymax;
	outside = r->flags & ROOM_OUTSIDE;
	camera_underwater = r->flags & ROOM_UNDERWATER;
	r->bound_active = 2;
	draw_room_list[0] = CurrentRoom;
	room_list_start = 0;
	room_list_end = 1;
	number_draw_rooms = 0;

	if (outside)
	{
		outside_top = 0;
		outside_left = 0;
		outside_right = phd_winxmax;
		outside_bottom = phd_winymax;
	}
	else
	{
		outside_left = phd_winxmax;
		outside_top = phd_winymax;
		outside_bottom = 0;
		outside_right = 0;
	}

	GetRoomBounds();
	InitialiseFogBulbs();
	CreateFXBulbs();

	if (outside)
	{
		if (!objects[HORIZON].loaded)
			outside = -1;
		else
		{
			if (BinocularRange)
				AlterFOV(14560 - (short)BinocularRange);

			phd_PushMatrix();
			phd_TranslateAbs(camera.pos.x, camera.pos.y, camera.pos.z);
			nPolyType = 6;
			phd_PushMatrix();

			if (gfLevelFlags & GF_LAYER1)
			{
				phd_RotY(32760);

				if (gfLevelFlags & GF_LIGHTNING)
					DrawFlatSky(RGBA(LightningRGB[0], LightningRGB[1], LightningRGB[2], 44), SkyPos, -1536, 4);
				else
					DrawFlatSky(*(ulong*)&gfLayer1Col, SkyPos, -1536, 4);
			}

			if (gfLevelFlags & GF_LAYER2)
				DrawFlatSky(0xFF000000 | *(ulong*)&gfLayer2Col, SkyPos2, -1536, 2);

			if (gfLevelFlags & GF_LAYER1 || gfLevelFlags & GF_LAYER2)
				OutputSky();

			phd_PopMatrix();

			if (gfLevelFlags & GF_HORIZON)
			{
				phd_PutPolygonSkyMesh(meshes[objects[HORIZON].mesh_index], -1);
				OutputSky();
			}

			phd_PopMatrix();
		}
	}

	InitDynamicLighting();
	nPolyType = 0;

	for (int i = 0; i < number_draw_rooms; i++)
		S_InsertRoom(draw_rooms[i]);

	if (gfLevelFlags & GF_TRAIN)
		DrawTrainFloor();

	for (int i = 0; i < number_draw_rooms; i++)
		PrintObjects(draw_rooms[i]);
}

long DrawPhaseGame()
{
	CalcLaraMatrices(0);
	phd_PushUnitMatrix();
	CalcLaraMatrices(1);
	phd_PopMatrix();

	if (GLOBAL_playing_cutseq)
		frigup_lara();

	SetLaraUnderwaterNodes();
	DrawRooms(camera.pos.room_number);
	DrawGameInfo(1);
	S_OutputPolyList();
	camera.number_frames = S_DumpScreen();
	S_AnimateTextures(camera.number_frames);
	return camera.number_frames;
}

void GetRoomBounds()
{
	ROOM_INFO* r;
	short* door;
	long rn, drn;

	while (room_list_start != room_list_end)
	{
		rn = draw_room_list[room_list_start % 128];
		room_list_start++;
		r = &room[rn];
		r->bound_active -= 2;

		if (r->test_left < r->left)
			r->left = r->test_left;

		if (r->test_top < r->top)
			r->top = r->test_top;

		if (r->test_right > r->right)
			r->right = r->test_right;

		if (r->test_bottom > r->bottom)
			r->bottom = r->test_bottom;

		if (!(r->bound_active & 1))
		{
			draw_rooms[number_draw_rooms] = (short)rn;
			number_draw_rooms++;
			r->bound_active |= 1;

			if (r->flags & ROOM_OUTSIDE)
				outside = ROOM_OUTSIDE;
		}

		if (r->flags & ROOM_OUTSIDE)
		{
			if (r->left < outside_left)
				outside_left = r->left;

			if (r->right > outside_right)
				outside_right = r->right;

			if (r->top < outside_top)
				outside_top = r->top;

			if (r->bottom > outside_bottom)
				outside_bottom = r->bottom;
		}

		phd_PushMatrix();
		phd_TranslateAbs(r->x, r->y, r->z);
		door = r->door;

		if (door)
		{
			for (drn = *door++; drn > 0; drn--)
			{
				rn = *door++;

				if (door[0] * (r->x + door[3] - w2v_matrix[M03]) +
					door[1] * (r->y + door[4] - w2v_matrix[M13]) +
					door[2] * (r->z + door[5] - w2v_matrix[M23]) < 0)
					SetRoomBounds(door, rn, r);

				door += 15;
			}
		}

		phd_PopMatrix();
	}
}

void SetRoomBounds(short* door, long rn, ROOM_INFO* actualRoom)
{
	ROOM_INFO* r;
	PHD_VECTOR* v;
	PHD_VECTOR* lastV;
	static PHD_VECTOR vbuf[4];
	long x, y, z, tooNear, tooFar, tL, tR, tT, tB;

	r = &room[rn];

	if (r->left <= actualRoom->test_left && r->right >= actualRoom->test_right && r->top <= actualRoom->test_top && r->bottom >= actualRoom->test_bottom)
		return;
	
	tL = actualRoom->test_right;
	tR = actualRoom->test_left;
	tB = actualRoom->test_top;
	tT = actualRoom->test_bottom;
	door += 3;
	v = vbuf;
	tooNear = 0;
	tooFar = 0;

	for (int i = 0; i < 4; i++, v++, door += 3)
	{
		v->x = phd_mxptr[M00] * door[0] + phd_mxptr[M01] * door[1] + phd_mxptr[M02] * door[2] + phd_mxptr[M03];
		v->y = phd_mxptr[M10] * door[0] + phd_mxptr[M11] * door[1] + phd_mxptr[M12] * door[2] + phd_mxptr[M13];
		v->z = phd_mxptr[M20] * door[0] + phd_mxptr[M21] * door[1] + phd_mxptr[M22] * door[2] + phd_mxptr[M23];
		x = v->x;
		y = v->y;
		z = v->z;

		if (z <= 0)
			tooNear++;
		else
		{
			if (z > phd_zfar)
				tooFar++;

			z /= phd_persp;

			if (z)
			{
				x = x / z + phd_centerx;
				y = y / z + phd_centery;
			}
			else
			{
				if (x < 0)
					x = phd_left;
				else
					x = phd_right;

				if (y < 0)
					y = phd_top;
				else
					y = phd_bottom;
			}

			if (x - 1 < tL)
				tL = x - 1;

			if (x + 1 > tR)
				tR = x + 1;

			if (y - 1 < tT)
				tT = y - 1;

			if (y + 1 > tB)
				tB = y + 1;
		}
	}

	if (tooNear == 4 || (tooFar == 4 && !outside))
		return;

	if (tooNear > 0)
	{
		v = vbuf;
		lastV = &vbuf[3];

		for (int i = 0; i < 4; i++, lastV = v, v++)
		{
			if (lastV->z <= 0 == v->z <= 0)
				continue;

			if (v->x < 0 && lastV->x < 0)
				tL = 0;
			else if (v->x > 0 && lastV->x > 0)
				tR = phd_winxmax;
			else
			{
				tL = 0;
				tR = phd_winxmax;
			}

			if (v->y < 0 && lastV->y < 0)
				tT = 0;
			else if (v->y > 0 && lastV->y > 0)
				tB = phd_winymax;
			else
			{
				tT = 0;
				tB = phd_winymax;
			}
		}
	}

	if (tL < actualRoom->test_left)
		tL = actualRoom->test_left;

	if (tR > actualRoom->test_right)
		tR = actualRoom->test_right;

	if (tT < actualRoom->test_top)
		tT = actualRoom->test_top;

	if (tB > actualRoom->test_bottom)
		tB = actualRoom->test_bottom;

	if (tL >= tR || tT >= tB)
		return;

	if (r->bound_active & 2)
	{
		if (tL < r->test_left)
			r->test_left = (short)tL;

		if (tT < r->test_top)
			r->test_top = (short)tT;

		if (tR > r->test_right)
			r->test_right = (short)tR;

		if (tB > r->test_bottom)
			r->test_bottom = (short)tB;
	}
	else
	{
		draw_room_list[room_list_end % 128] = rn;
		room_list_end++;
		r->bound_active |= 2;
		r->test_left = (short)tL;
		r->test_right = (short)tR;
		r->test_top = (short)tT;
		r->test_bottom = (short)tB;
	}
}

void DrawEffect(short fx_num)
{
	FX_INFO* fx;
	OBJECT_INFO* obj;
	short* meshp;

	fx = &effects[fx_num];
	obj = &objects[fx->object_number];

	if (obj->draw_routine && obj->loaded)
	{
		phd_PushMatrix();
		phd_TranslateAbs(fx->pos.x_pos, fx->pos.y_pos, fx->pos.z_pos);

		if (phd_mxptr[M23] > phd_znear && phd_mxptr[M23] < phd_zfar)
		{
			phd_RotYXZ(fx->pos.y_rot, fx->pos.x_rot, fx->pos.z_rot);
			
			if (obj->nmeshes)
				meshp = meshes[obj->mesh_index];
			else
				meshp = meshes[fx->frame_number];

			//empty func call here
			phd_PutPolygons(meshp, -1);
		}

		phd_PopMatrix();
	}
}

void PrintObjects(short room_number)
{
	ROOM_INFO* r;
	MESH_INFO* mesh;
	STATIC_INFO* sinfo;
	ITEM_INFO* item;
	OBJECT_INFO* obj;
	FX_INFO* fx;
	long clip;
	short item_number, fx_number;

	current_room = room_number;
	nPolyType = 1;
	r = &room[room_number];
	r->bound_active = 0;
	phd_PushMatrix();
	phd_TranslateAbs(r->x, r->y, r->z);

	if (gfLevelFlags & GF_TRAIN)
	{
		phd_left = 0;
		phd_top = 0;
		phd_right = phd_winxmax + 1;
		phd_bottom = phd_winymax + 1;
	}
	else
	{
		phd_left = r->left;
		phd_right = r->right;
		phd_top = r->top;
		phd_bottom = r->bottom;
	}

	mesh = r->mesh;

	for (int i = r->num_meshes; i > 0; i--, mesh++)
	{
		if (mesh->Flags & 1)
		{
			phd_PushMatrix();
			phd_TranslateAbs(mesh->x, mesh->y, mesh->z);
			phd_RotY(mesh->y_rot);
			sinfo = &static_objects[mesh->static_number];
			clip = S_GetObjectBounds(&sinfo->x_minp);

			if (clip)
			{
				S_CalculateStaticMeshLight(mesh->x, mesh->y, mesh->z, mesh->shade, r);
				phd_PutPolygons(meshes[sinfo->mesh_number], clip);
			}

			phd_PopMatrix();
		}
	}

	nPolyType = 2;
	phd_left = 0;
	phd_top = 0;
	phd_right = phd_winxmax + 1;
	phd_bottom = phd_winymax + 1;

	for (item_number = r->item_number; item_number != NO_ITEM; item_number = item->next_item)
	{
		ClipRoomNum = room_number;
		item = &items[item_number];
		obj = &objects[item->object_number];

		if (item->status != ITEM_INVISIBLE)
		{
			if (item->after_death)
				GlobalAlpha = 0xFE000000 * item->after_death;	//mmmm

			if (obj->draw_routine)
				obj->draw_routine(item);

			if (obj->draw_routine_extra)
				obj->draw_routine_extra(item);

			GlobalAlpha = 0xFF000000;
		}

		if (item->after_death < 128 && item->after_death > 0)
			item->after_death++;

		if (item->after_death == 128)
			KillItem(item_number);
	}

	nPolyType = 3;

	for (fx_number = r->fx_number; fx_number != NO_ITEM; fx_number = fx->next_fx)
	{
		fx = &effects[fx_number];
		DrawEffect(fx_number);
	}

	phd_PopMatrix();
	r->left = phd_winxmax;
	r->top = phd_winymax;
	r->right = 0;
	r->bottom = 0;
}

long GetFrames(ITEM_INFO* item, short* frm[], long* rate)
{
	ANIM_STRUCT* anim;
	long frame, size, frac, num;

	anim = &anims[item->anim_number];
	frm[0] = anim->frame_ptr;
	frm[1] = anim->frame_ptr;
	*rate = anim->interpolation & 0xFF;
	frame = item->frame_number - anim->frame_base;
	size = anim->interpolation >> 8;
	frm[0] += size * (frame / *rate);
	frm[1] = frm[0] + size;
	frac = frame % *rate;

	if (!frac)
		return 0;

	num = *rate * (frame / *rate + 1);

	if (num > anim->frame_end)
		*rate = *rate + anim->frame_end - num;

	return frac;
}

short* GetBoundsAccurate(ITEM_INFO* item)
{
	short* bptr;
	short* frmptr[2];
	long rate, frac;
	static short interpolated_bounds[6];

	frac = GetFrames(item, frmptr, &rate);

	if (!frac)
		return frmptr[0];

	bptr = interpolated_bounds;

	for (int i = 0; i < 6; i++)
	{
		bptr[i] = short(*frmptr[0] + (*frmptr[1] - *frmptr[0]) * frac / rate);
		frmptr[0]++;
		frmptr[1]++;
	}

	return interpolated_bounds;
}

short* GetBestFrame(ITEM_INFO* item)
{
	short* frm[2];
	long rate, frac;

	frac = GetFrames(item, frm, &rate);

	if (frac > rate >> 1)
		return frm[1];
	else
		return frm[0];
}

void UpdateSkyLightning()
{
	if (LightningCount <= 0)
	{
		if (LightningRand < 4)
			LightningRand = 0;
		else
			LightningRand -= LightningRand >> 2;
	}
	else
	{
		LightningCount--;

		if (LightningCount)
		{
			dLightningRand = GetRandomDraw() & 0x1FF;
			LightningRand += (dLightningRand - LightningRand) >> 1;
		}
		else
		{
			dLightningRand = 0;
			LightningRand = (GetRandomDraw() & 0x7F) + 400;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		LightningRGB[i] = LightningRGBs[i] + ((LightningRGBs[i] * LightningRand) >> 8);

		if (LightningRGB[i] > 255)
			LightningRGB[i] = 255;
	}
}

void mRotBoundingBoxNoPersp(short* bounds, short* rotatedBounds)
{
	PHD_VECTOR pos[8];
	long x, y, z;
	short xMin, xMax, yMin, yMax, zMin, zMax;

	xMin = bounds[0];
	xMax = bounds[1];
	yMin = bounds[2];
	yMax = bounds[3];
	zMin = bounds[4];
	zMax = bounds[5];

	pos[0].x = xMin;
	pos[0].y = yMin;
	pos[0].z = zMin;

	pos[1].x = xMax;
	pos[1].y = yMin;
	pos[1].z = zMin;

	pos[2].x = xMin;
	pos[2].y = yMax;
	pos[2].z = zMin;

	pos[3].x = xMax;
	pos[3].y = yMax;
	pos[3].z = zMin;

	pos[4].x = xMin;
	pos[4].y = yMin;
	pos[4].z = zMax;

	pos[5].x = xMax;
	pos[5].y = yMin;
	pos[5].z = zMax;

	pos[6].x = xMin;
	pos[6].y = yMax;
	pos[6].z = zMax;

	pos[7].x = xMax;
	pos[7].y = yMax;
	pos[7].z = zMax;

	xMin = 0x7FFF;
	yMin = 0x7FFF;
	zMin = 0x7FFF;
	xMax = -0x7FFF;
	yMax = -0x7FFF;
	zMax = -0x7FFF;

	for (int i = 0; i < 8; i++)
	{
		x = pos[i].x * phd_mxptr[M00] + pos[i].y * phd_mxptr[M01] + pos[i].z * phd_mxptr[M02] + phd_mxptr[M03];
		y = pos[i].x * phd_mxptr[M10] + pos[i].y * phd_mxptr[M11] + pos[i].z * phd_mxptr[M12] + phd_mxptr[M13];
		z = pos[i].x * phd_mxptr[M20] + pos[i].y * phd_mxptr[M21] + pos[i].z * phd_mxptr[M22] + phd_mxptr[M23];

		if (x < xMin)
			xMin = (short)x;

		if (x > xMax)
			xMax = (short)x;

		if (y < yMin)
			yMin = (short)y;

		if (y > yMax)
			yMax = (short)y;

		if (z < zMin)
			zMin = (short)z;

		if (z > zMax)
			zMax = (short)z;
	}

	rotatedBounds[0] = xMin;
	rotatedBounds[1] = xMax;
	rotatedBounds[2] = yMin;
	rotatedBounds[3] = yMax;
	rotatedBounds[4] = zMin;
	rotatedBounds[5] = zMax;
}

void calc_animating_item_clip_window(ITEM_INFO* item, short* bounds)
{
	ROOM_INFO* r;
	short* door;
	long xMin, xMax, yMin, yMax, zMin, zMax;		//object bounds
	long xMinR, xMaxR, yMinR, yMaxR, zMinR, zMaxR;	//room bounds
	long xMinD, xMaxD, yMinD, yMaxD, zMinD, zMaxD;	//door bounds
	short rotatedBounds[6];
	short nDoors;

	r = &room[ClipRoomNum];

	if (item->object_number >= ANIMATING1 && item->object_number <= ANIMATING16 ||
		item->object_number >= DOOR_TYPE1 && item->object_number <= DOOR_TYPE8)
	{
		phd_left = r->left;
		phd_right = r->right;
		phd_top = r->top;
		phd_bottom = r->bottom;
		return;
	}

	phd_PushUnitMatrix();
	phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
	phd_mxptr[M03] = 0;
	phd_mxptr[M13] = 0;
	phd_mxptr[M23] = 0;
	mRotBoundingBoxNoPersp(bounds, rotatedBounds);
	phd_PopMatrix();

	xMin = item->pos.x_pos + rotatedBounds[0];
	xMax = item->pos.x_pos + rotatedBounds[1];
	yMin = item->pos.y_pos + rotatedBounds[2];
	yMax = item->pos.y_pos + rotatedBounds[3];
	zMin = item->pos.z_pos + rotatedBounds[4];
	zMax = item->pos.z_pos + rotatedBounds[5];

	xMinR = r->x + 1024;
	xMaxR = xMinR + ((r->y_size - 2) << 10);
	yMinR = r->maxceiling;
	yMaxR = r->minfloor;
	zMinR = r->z + 1024;
	zMaxR = zMinR + ((r->x_size - 2) << 10);

	if (xMin >= xMinR && xMax <= xMaxR && yMin >= yMinR && yMax <= yMaxR && zMin >= zMinR && zMax <= zMaxR)
	{
		phd_left = r->left;
		phd_right = r->right;
		phd_top = r->top;
		phd_bottom = r->bottom;
		return;
	}

	if (camera.pos.room_number != item->room_number && camera.pos.room_number != ClipRoomNum)
	{
		door = r->door;
		nDoors = *door++;

		for (; nDoors > 0; nDoors--, door += 16)
		{
			if (door[0] != camera.pos.room_number)
				continue;

			xMinD = door[4];	//skip door normal
			xMaxD = door[4];
			yMinD = door[5];
			yMaxD = door[5];
			zMinD = door[6];
			zMaxD = door[6];

			if (door[7] < xMinD)
				xMinD = door[7];
			else if (door[7] > xMaxD)
				xMaxD = door[7];

			if (door[8] < yMinD)
				yMinD = door[8];
			else if (door[8] > yMaxD)
				yMaxD = door[8];

			if (door[9] < zMinD)
				zMinD = door[9];
			else if (door[9] > zMaxD)
				zMaxD = door[9];

			if (door[10] < xMinD)
				xMinD = door[10];
			else if (door[10] > xMaxD)
				xMaxD = door[10];

			if (door[11] < yMinD)
				yMinD = door[11];
			else if (door[11] > yMaxD)
				yMaxD = door[11];

			if (door[12] < zMinD)
				zMinD = door[12];
			else if (door[12] > zMaxD)
				zMaxD = door[12];

			if (door[13] < xMinD)
				xMinD = door[13];
			else if (door[13] > xMaxD)
				xMaxD = door[13];

			if (door[14] < yMinD)
				yMinD = door[14];
			else if (door[14] > yMaxD)
				yMaxD = door[14];

			if (door[15] < zMinD)
				zMinD = door[15];
			else if (door[15] > zMaxD)
				zMaxD = door[15];

			xMinD += xMinR - 1024;
			xMaxD += xMinR - 1024;
			yMinD += yMinR;
			yMaxD += yMinR;
			zMinD += zMinR - 1024;
			zMaxD += zMinR - 1024;

			if (xMinD <= xMax && xMaxD >= xMin && yMinD <= yMax && yMaxD >= yMin && zMinD <= zMax && zMaxD >= zMin)
				break;
		}

		if (!nDoors)
		{
			phd_left = r->left;
			phd_right = r->right;
			phd_top = r->top;
			phd_bottom = r->bottom;
		}
	}
}
