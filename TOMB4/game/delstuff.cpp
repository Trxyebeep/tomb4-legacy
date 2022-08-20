#include "../tomb4/pch.h"
#include "delstuff.h"
#include "../specific/specificfx.h"
#include "../specific/3dmath.h"
#include "draw.h"
#include "../specific/output.h"
#include "hair.h"
#include "objects.h"
#include "control.h"
#include "lara_states.h"
#include "../specific/input.h"
#include "../specific/function_stubs.h"
#include "lara.h"
#include "gameflow.h"

short* GLaraShadowframe;
long lara_matrices[180];
long lara_joint_matrices[180];
long LaraNodeAmbient[2];
long bLaraUnderWater;
uchar LaraNodeUnderwater[15];
char SkinVertNums[40][12];
char ScratchVertNums[40][12];
char bLaraInWater;

char HairRotScratchVertNums[5][12] =
{
	{ 4, 5, 6, 7, -1, 0, 0, 0, 0, 0, 0, 0 },
	{ 5, 6, 7, 4, -1, 0, 0, 0, 0, 0, 0, 0 },
	{ 6, 7, 4, 5, -1, 0, 0, 0, 0, 0, 0, 0 },
	{ 7, 4, 5, 6, -1, 0, 0, 0, 0, 0, 0, 0 },
	{ 4, 5, 6, 7, -1, 0, 0, 0, 0, 0, 0, 0 }
};

static long lara_mesh_sweetness_table[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 8, 9, 10, 11, 12, 13 };
static char lara_underwater_skin_sweetness_table[15] = { 0, 2, 3, 0, 5, 6, 7, 9, 10, 11, 12, 13, 14, 8, 0 };

static char NodesToStashToScratch[14][2] =
{
	{1, 3},
	{4, 5},
	{6, 7},
	{2, 8},
	{9, 10},
	{11, 12},
	{0, 13},
	{14, 17},
	{18, 19},
	{20, 21},
	{15, 22},
	{23, 24},
	{25, 26},
	{16, 27}
};

static char NodesToStashFromScratch[15][4] =
{
	{0, 1, 2, -1},
	{3, 4, -1, 0},
	{5, 6, -1, 0},
	{7, -1, 0, 0},
	{8, 9, -1, 0},
	{10, 11, -1, 0},
	{12, -1, 0, 0},
	{13, 16, 14, 15},
	{27, 28, 34, -1},
	{17, 18, -1, 0},
	{19, 20, -1, 0},
	{21, -1, 0, 0},
	{22, 23, -1, 0},
	{24, 25, -1, 0},
	{26, -1, 0, 0}
};

static uchar SkinUseMatrix[14][2] =
{
	{255, 255},
	{1, 2},
	{255, 255},
	{255, 255},
	{4, 5},
	{255, 255},
	{255, 255},
	{255, 255},
	{9, 10},
	{255, 255},
	{255, 255},
	{12, 13},
	{255, 255},
	{255, 255}
};

static long in_joints;

void DrawLara(ITEM_INFO* item, long mirror)
{
	OBJECT_INFO* obj;
	PHD_VECTOR v0;
	PHD_VECTOR v1;
	short** meshpp;
	long* bone;
	short* rot;
	long top, bottom, left, right, dx, dy, dz, dist, stash, cos, sin, xRot;
	static long a = 255;

	top = phd_top;
	bottom = phd_bottom;
	left = phd_left;
	right = phd_right;
	phd_top = 0;
	phd_left = 0;
	phd_bottom = phd_winymax;
	phd_right = phd_winxmax;
	phd_PushMatrix();
	obj = &objects[item->object_number];

	if (lara.vehicle == NO_ITEM)
		S_PrintShadow(obj->shadow_size, GLaraShadowframe, item);

	if (input & IN_LOOK)
	{
		dx = lara_item->pos.x_pos - CamPos.x;
		dy = lara_item->pos.y_pos - CamPos.y - 512;
		dz = lara_item->pos.z_pos - CamPos.z;
		dist = phd_sqrt(SQUARE(dx) + SQUARE(dy) + SQUARE(dz));
		a = dist >> 2;

		if (a < 0)
			a = 0;

		if (a > 255)
			a = 255;

		GlobalAlpha = a << 24;
	}
	else
	{
		if (a < 255)
		{
			a += 8;

			if (a > 255)
				a = 255;
		}

		GlobalAlpha = a << 24;
	}

	if (!mirror)
		CalculateObjectLightingLara();

	for (int i = 0; i < 15; i++)//skin
	{
		phd_mxptr[M00] = lara_matrices[i * 12 + M00];
		phd_mxptr[M01] = lara_matrices[i * 12 + M01];
		phd_mxptr[M02] = lara_matrices[i * 12 + M02];
		phd_mxptr[M03] = lara_matrices[i * 12 + M03];
		phd_mxptr[M10] = lara_matrices[i * 12 + M10];
		phd_mxptr[M11] = lara_matrices[i * 12 + M11];
		phd_mxptr[M12] = lara_matrices[i * 12 + M12];
		phd_mxptr[M13] = lara_matrices[i * 12 + M13];
		phd_mxptr[M20] = lara_matrices[i * 12 + M20];
		phd_mxptr[M21] = lara_matrices[i * 12 + M21];
		phd_mxptr[M22] = lara_matrices[i * 12 + M22];
		phd_mxptr[M23] = lara_matrices[i * 12 + M23];

		if (LaraNodeUnderwater[i])
			bLaraUnderWater = i;
		else
			bLaraUnderWater = -1;

		phd_PutPolygons(lara.mesh_ptrs[lara_mesh_sweetness_table[i]], -1);	//no meshbits checks?

		for (int j = 0; j < 4; j++)
		{
			stash = (uchar)NodesToStashFromScratch[i][j];

			if (stash == 255)
				break;

			StashSkinVertices(stash);
		}
	}

	in_joints = 1;
	phd_PopMatrix();
	bLaraUnderWater = LaraNodeUnderwater[8] != 0 ? 8 : -1;
	DrawHair();
	phd_PushMatrix();
	obj = &objects[LARA_SKIN_JOINTS];
	meshpp = &meshes[obj->mesh_index];
	meshpp += 2;

	for (int i = 0; i < 14; i++)//joints
	{
		SkinVerticesToScratch(NodesToStashToScratch[i][0]);
		SkinVerticesToScratch(NodesToStashToScratch[i][1]);

		if (LaraNodeUnderwater[lara_underwater_skin_sweetness_table[i]])
			bLaraUnderWater = lara_underwater_skin_sweetness_table[i];
		else
			bLaraUnderWater = -1;

		if (SkinUseMatrix[i][0] >= 255)
			phd_PutPolygons(*meshpp, -1);
		else
		{
			phd_mxptr[M00] = lara_matrices[SkinUseMatrix[i][1] * 12 + M00];
			phd_mxptr[M01] = lara_matrices[SkinUseMatrix[i][1] * 12 + M01];
			phd_mxptr[M02] = lara_matrices[SkinUseMatrix[i][1] * 12 + M02];
			phd_mxptr[M03] = lara_matrices[SkinUseMatrix[i][1] * 12 + M03];
			phd_mxptr[M10] = lara_matrices[SkinUseMatrix[i][1] * 12 + M10];
			phd_mxptr[M11] = lara_matrices[SkinUseMatrix[i][1] * 12 + M11];
			phd_mxptr[M12] = lara_matrices[SkinUseMatrix[i][1] * 12 + M12];
			phd_mxptr[M13] = lara_matrices[SkinUseMatrix[i][1] * 12 + M13];
			phd_mxptr[M20] = lara_matrices[SkinUseMatrix[i][1] * 12 + M20];
			phd_mxptr[M21] = lara_matrices[SkinUseMatrix[i][1] * 12 + M21];
			phd_mxptr[M22] = lara_matrices[SkinUseMatrix[i][1] * 12 + M22];
			phd_mxptr[M23] = lara_matrices[SkinUseMatrix[i][1] * 12 + M23];
			phd_PushMatrix();
			v0.x = lara_matrices[12 * SkinUseMatrix[i][0] + M01];
			v0.y = lara_matrices[12 * SkinUseMatrix[i][0] + M11];
			v0.z = lara_matrices[12 * SkinUseMatrix[i][0] + M21];
			v1.x = lara_matrices[12 * SkinUseMatrix[i][1] + M01];
			v1.y = lara_matrices[12 * SkinUseMatrix[i][1] + M11];
			v1.z = lara_matrices[12 * SkinUseMatrix[i][1] + M21];
			cos = ((v0.x * v1.x) + (v0.y * v1.y) + (v0.z * v1.z)) >> W2V_SHIFT;
			sin = phd_sqrt(16777216 - SQUARE(cos));

			if (i == 1 || i == 4)
				xRot = -phd_atan(cos, sin);
			else
				xRot = phd_atan(cos, sin);

			phd_RotX(-(short)xRot >> 1);
			phd_PutPolygons(*meshpp, -1);
			phd_PopMatrix();
		}

		meshpp += 2;
	}

	in_joints = 0;
	bLaraUnderWater = (LaraNodeUnderwater[0] != 0) - 1;

	if (!(gfLevelFlags & GF_YOUNGLARA))
	{
		obj = &objects[lara.holster];
		meshpp = &meshes[obj->mesh_index];
		meshpp += 8;
		phd_mxptr[M00] = lara_matrices[1 * 12 + M00];
		phd_mxptr[M01] = lara_matrices[1 * 12 + M01];
		phd_mxptr[M02] = lara_matrices[1 * 12 + M02];
		phd_mxptr[M03] = lara_matrices[1 * 12 + M03];
		phd_mxptr[M10] = lara_matrices[1 * 12 + M10];
		phd_mxptr[M11] = lara_matrices[1 * 12 + M11];
		phd_mxptr[M12] = lara_matrices[1 * 12 + M12];
		phd_mxptr[M13] = lara_matrices[1 * 12 + M13];
		phd_mxptr[M20] = lara_matrices[1 * 12 + M20];
		phd_mxptr[M21] = lara_matrices[1 * 12 + M21];
		phd_mxptr[M22] = lara_matrices[1 * 12 + M22];
		phd_mxptr[M23] = lara_matrices[1 * 12 + M23];
		phd_PutPolygons(*meshpp, -1);

		meshpp += 8;
		phd_mxptr[M00] = lara_matrices[4 * 12 + M00];
		phd_mxptr[M01] = lara_matrices[4 * 12 + M01];
		phd_mxptr[M02] = lara_matrices[4 * 12 + M02];
		phd_mxptr[M03] = lara_matrices[4 * 12 + M03];
		phd_mxptr[M10] = lara_matrices[4 * 12 + M10];
		phd_mxptr[M11] = lara_matrices[4 * 12 + M11];
		phd_mxptr[M12] = lara_matrices[4 * 12 + M12];
		phd_mxptr[M13] = lara_matrices[4 * 12 + M13];
		phd_mxptr[M20] = lara_matrices[4 * 12 + M20];
		phd_mxptr[M21] = lara_matrices[4 * 12 + M21];
		phd_mxptr[M22] = lara_matrices[4 * 12 + M22];
		phd_mxptr[M23] = lara_matrices[4 * 12 + M23];
		phd_PutPolygons(*meshpp, -1);

		if (lara.back_gun)
		{
			phd_PushMatrix();
			phd_mxptr[M00] = lara_matrices[84 + M00];
			phd_mxptr[M01] = lara_matrices[84 + M01];
			phd_mxptr[M02] = lara_matrices[84 + M02];
			phd_mxptr[M03] = lara_matrices[84 + M03];
			phd_mxptr[M10] = lara_matrices[84 + M10];
			phd_mxptr[M11] = lara_matrices[84 + M11];
			phd_mxptr[M12] = lara_matrices[84 + M12];
			phd_mxptr[M13] = lara_matrices[84 + M13];
			phd_mxptr[M20] = lara_matrices[84 + M20];
			phd_mxptr[M21] = lara_matrices[84 + M21];
			phd_mxptr[M22] = lara_matrices[84 + M22];
			phd_mxptr[M23] = lara_matrices[84 + M23];
			obj = &objects[lara.back_gun];
			bone = &bones[obj->bone_index];
			meshpp = &meshes[obj->mesh_index];
			meshpp += 28;
			phd_TranslateRel(bone[53], bone[54], bone[55]);
			rot = objects[lara.back_gun].frame_base + 9;
			gar_RotYXZsuperpack(&rot, 14);
			phd_PutPolygons(*meshpp, -1);
			phd_PopMatrix();
		}
	}

	bLaraUnderWater = 0;
	phd_PopMatrix();
	phd_top = top;
	phd_bottom = bottom;
	phd_left = left;
	phd_right = right;
	GlobalAlpha = 0xFF000000;
}

void GetLaraJointPos(PHD_VECTOR* pos, long node)
{
	phd_PushMatrix();
	phd_mxptr[M00] = lara_joint_matrices[node * 12 + M00];
	phd_mxptr[M01] = lara_joint_matrices[node * 12 + M01];
	phd_mxptr[M02] = lara_joint_matrices[node * 12 + M02];
	phd_mxptr[M03] = lara_joint_matrices[node * 12 + M03];
	phd_mxptr[M10] = lara_joint_matrices[node * 12 + M10];
	phd_mxptr[M11] = lara_joint_matrices[node * 12 + M11];
	phd_mxptr[M12] = lara_joint_matrices[node * 12 + M12];
	phd_mxptr[M13] = lara_joint_matrices[node * 12 + M13];
	phd_mxptr[M20] = lara_joint_matrices[node * 12 + M20];
	phd_mxptr[M21] = lara_joint_matrices[node * 12 + M21];
	phd_mxptr[M22] = lara_joint_matrices[node * 12 + M22];
	phd_mxptr[M23] = lara_joint_matrices[node * 12 + M23];
	phd_TranslateRel(pos->x, pos->y, pos->z);
	pos->x = phd_mxptr[M03] >> W2V_SHIFT;
	pos->y = phd_mxptr[M13] >> W2V_SHIFT;
	pos->z = phd_mxptr[M23] >> W2V_SHIFT;
	pos->x += lara_item->pos.x_pos;
	pos->y += lara_item->pos.y_pos;
	pos->z += lara_item->pos.z_pos;
	phd_PopMatrix();
}

void SetLaraUnderwaterNodes()
{
	PHD_VECTOR pos;
	long bit;
	short room_num;

	pos.x = lara_item->pos.x_pos;
	pos.y = lara_item->pos.y_pos;
	pos.z = lara_item->pos.z_pos;
	room_num = lara_item->room_number;
	GetFloor(pos.x, pos.y, pos.z, &room_num);
	bLaraInWater = room[room_num].flags & ROOM_UNDERWATER ? 1 : 0;
	bit = 0;

	for (int i = 14; i >= 0; i--)
	{
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
		GetLaraJointPos(&pos, i);

		if (lara_mesh_sweetness_table[i] == 7)
			pos.y -= 120;

		if (lara_mesh_sweetness_table[i] == 14)
			pos.y -= 60;

		room_num = lara_item->room_number;
		GetFloor(pos.x, pos.y, pos.z, &room_num);
		LaraNodeUnderwater[i] = room[room_num].flags & ROOM_UNDERWATER;

		if (room[room_num].flags & ROOM_UNDERWATER)
		{
			lara.wet[i] = 252;

			if (!(bit & 1))
			{
				LaraNodeAmbient[1] = room[room_num].ambient;
				bit |= 1;
			}
		}
		else if (!(bit & 2))
		{
			LaraNodeAmbient[0] = room[room_num].ambient;
			bit |= 2;
		}
	}
}

void Rich_CalcLaraMatrices_Normal(short* frame, long* bone, long flag)
{
	PHD_VECTOR vec;
	long* matrix;
	short* rot;
	short* rot2;
	short gun;

	if (flag == 1)
		matrix = lara_joint_matrices;
	else
		matrix = lara_matrices;

	phd_PushMatrix();

	if (!flag || flag == 2)
		phd_TranslateAbs(lara_item->pos.x_pos, lara_item->pos.y_pos, lara_item->pos.z_pos);
	else
	{
		phd_mxptr[M03] = 0;
		phd_mxptr[M13] = 0;
		phd_mxptr[M23] = 0;
	}

	phd_RotYXZ(lara_item->pos.y_rot, lara_item->pos.x_rot, lara_item->pos.z_rot);

	if (flag == 2)
	{
		vec.x = -16384;
		vec.y = -16384;
		vec.z = -16384;
		ScaleCurrentMatrix(&vec);
	}

	phd_PushMatrix();
	rot = &frame[9];
	phd_TranslateRel(frame[6], frame[7], frame[8]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;

	phd_PushMatrix();
	phd_TranslateRel(bone[1], bone[2], bone[3]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;

	phd_TranslateRel(bone[5], bone[6], bone[7]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;

	phd_TranslateRel(bone[9], bone[10], bone[11]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_PushMatrix();
	phd_TranslateRel(bone[13], bone[14], bone[15]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;

	phd_TranslateRel(bone[17], bone[18], bone[19]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;

	phd_TranslateRel(bone[21], bone[22], bone[23]);
	gar_RotYXZsuperpack(&rot, 0);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_TranslateRel(bone[25], bone[26], bone[27]);
	gar_RotYXZsuperpack(&rot, 0);
	phd_RotYXZ(lara.torso_y_rot, lara.torso_x_rot, lara.torso_z_rot);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;

	phd_PushMatrix();
	phd_TranslateRel(bone[53], bone[54], bone[55]);
	rot2 = rot;
	gar_RotYXZsuperpack(&rot2, 6);
	phd_RotYXZ(lara.head_y_rot, lara.head_x_rot, lara.head_z_rot);
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	gun = WEAPON_NONE;

	if (lara.gun_status == LG_READY || lara.gun_status == LG_FLARE || lara.gun_status == LG_DRAW_GUNS || lara.gun_status == LG_UNDRAW_GUNS)
		gun = lara.gun_type;

	switch (gun)
	{
	case WEAPON_NONE:
	case WEAPON_FLARE:
	case WEAPON_TORCH:
		phd_PushMatrix();
		phd_TranslateRel(bone[29], bone[30], bone[31]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_PushMatrix();
		phd_TranslateRel(bone[41], bone[42], bone[43]);

		if (lara.flare_control_left)
		{
			rot = &lara.left_arm.frame_base[(anims[lara.left_arm.anim_number].interpolation >> 8) *
				(lara.left_arm.frame_number - anims[lara.left_arm.anim_number].frame_base) + 9];

			gar_RotYXZsuperpack(&rot, 11);
		}
		else
			gar_RotYXZsuperpack(&rot, 0);

		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		break;

	case WEAPON_PISTOLS:
	case WEAPON_UZI:
		phd_PushMatrix();
		phd_TranslateRel(bone[29], bone[30], bone[31]);

		phd_mxptr[M00] = phd_mxptr[-2 * indices_count + M00];
		phd_mxptr[M01] = phd_mxptr[-2 * indices_count + M01];
		phd_mxptr[M02] = phd_mxptr[-2 * indices_count + M02];
		phd_mxptr[M10] = phd_mxptr[-2 * indices_count + M10];
		phd_mxptr[M11] = phd_mxptr[-2 * indices_count + M11];
		phd_mxptr[M12] = phd_mxptr[-2 * indices_count + M12];
		phd_mxptr[M20] = phd_mxptr[-2 * indices_count + M20];
		phd_mxptr[M21] = phd_mxptr[-2 * indices_count + M21];
		phd_mxptr[M22] = phd_mxptr[-2 * indices_count + M22];

		phd_RotYXZ(lara.right_arm.y_rot, lara.right_arm.x_rot, lara.right_arm.z_rot);
		rot = &lara.right_arm.frame_base[(anims[lara.right_arm.anim_number].interpolation >> 8) *
			(lara.right_arm.frame_number - anims[lara.right_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 8);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_PushMatrix();
		phd_TranslateRel(bone[41], bone[42], bone[43]);

		phd_mxptr[M00] = phd_mxptr[-2 * indices_count + M00];
		phd_mxptr[M01] = phd_mxptr[-2 * indices_count + M01];
		phd_mxptr[M02] = phd_mxptr[-2 * indices_count + M02];
		phd_mxptr[M10] = phd_mxptr[-2 * indices_count + M10];
		phd_mxptr[M11] = phd_mxptr[-2 * indices_count + M11];
		phd_mxptr[M12] = phd_mxptr[-2 * indices_count + M12];
		phd_mxptr[M20] = phd_mxptr[-2 * indices_count + M20];
		phd_mxptr[M21] = phd_mxptr[-2 * indices_count + M21];
		phd_mxptr[M22] = phd_mxptr[-2 * indices_count + M22];

		phd_RotYXZ(lara.left_arm.y_rot, lara.left_arm.x_rot, lara.left_arm.z_rot);
		rot = &lara.left_arm.frame_base[(anims[lara.left_arm.anim_number].interpolation >> 8) *
			(lara.left_arm.frame_number - anims[lara.left_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 11);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		break;

	case WEAPON_REVOLVER:
		phd_PushMatrix();
		phd_TranslateRel(bone[29], bone[30], bone[31]);

		phd_mxptr[M00] = phd_mxptr[-2 * indices_count + M00];
		phd_mxptr[M01] = phd_mxptr[-2 * indices_count + M01];
		phd_mxptr[M02] = phd_mxptr[-2 * indices_count + M02];
		phd_mxptr[M10] = phd_mxptr[-2 * indices_count + M10];
		phd_mxptr[M11] = phd_mxptr[-2 * indices_count + M11];
		phd_mxptr[M12] = phd_mxptr[-2 * indices_count + M12];
		phd_mxptr[M20] = phd_mxptr[-2 * indices_count + M20];
		phd_mxptr[M21] = phd_mxptr[-2 * indices_count + M21];
		phd_mxptr[M22] = phd_mxptr[-2 * indices_count + M22];

		phd_RotYXZ(lara.torso_y_rot, lara.torso_x_rot, lara.torso_z_rot);
		rot = &lara.right_arm.frame_base[(anims[lara.right_arm.anim_number].interpolation >> 8) *
			(lara.right_arm.frame_number - anims[lara.right_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 8);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_PushMatrix();
		phd_TranslateRel(bone[41], bone[42], bone[43]);

		phd_mxptr[M00] = phd_mxptr[-2 * indices_count + M00];
		phd_mxptr[M01] = phd_mxptr[-2 * indices_count + M01];
		phd_mxptr[M02] = phd_mxptr[-2 * indices_count + M02];
		phd_mxptr[M10] = phd_mxptr[-2 * indices_count + M10];
		phd_mxptr[M11] = phd_mxptr[-2 * indices_count + M11];
		phd_mxptr[M12] = phd_mxptr[-2 * indices_count + M12];
		phd_mxptr[M20] = phd_mxptr[-2 * indices_count + M20];
		phd_mxptr[M21] = phd_mxptr[-2 * indices_count + M21];
		phd_mxptr[M22] = phd_mxptr[-2 * indices_count + M22];

		phd_RotYXZ(lara.torso_y_rot, lara.torso_x_rot, lara.torso_z_rot);
		rot = &lara.left_arm.frame_base[(anims[lara.left_arm.anim_number].interpolation >> 8) *
			(lara.left_arm.frame_number - anims[lara.left_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 11);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		break;

	case WEAPON_SHOTGUN:
	case WEAPON_GRENADE:
	case WEAPON_CROSSBOW:
		phd_PushMatrix();
		phd_TranslateRel(bone[29], bone[30], bone[31]);
		rot = &lara.right_arm.frame_base[lara.right_arm.frame_number *
			(anims[lara.right_arm.anim_number].interpolation >> 8) + 9];
		gar_RotYXZsuperpack(&rot, 8);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_PushMatrix();
		phd_TranslateRel(bone[41], bone[42], bone[43]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		break;
	}

	phd_PopMatrix();
	phd_PopMatrix();
	phd_PopMatrix();
	GLaraShadowframe = frame;
}

void Rich_CalcLaraMatrices_Interpolated(short* frame1, short* frame2, long frac, long rate, long* bone, long flag)
{
	PHD_VECTOR vec;
	long* matrix;
	long* arms;
	short* rot;
	short* rot2;
	short* rotcopy;
	short* rot2copy;
	short gun;

	if (flag == 1)
		matrix = lara_joint_matrices;
	else
		matrix = lara_matrices;

	phd_PushMatrix();

	if (!flag || flag == 2)
		phd_TranslateAbs(lara_item->pos.x_pos, lara_item->pos.y_pos, lara_item->pos.z_pos);
	else
	{
		phd_mxptr[M03] = 0;
		phd_mxptr[M13] = 0;
		phd_mxptr[M23] = 0;
	}

	phd_RotYXZ(lara_item->pos.y_rot, lara_item->pos.x_rot, lara_item->pos.z_rot);

	if (flag == 2)
	{
		vec.x = -16384;
		vec.y = -16384;
		vec.z = -16384;
		ScaleCurrentMatrix(&vec);
	}

	arms = phd_mxptr;
	phd_PushMatrix();
	rot = frame1 + 9;
	rot2 = frame2 + 9;
	InitInterpolate(frac, rate);
	phd_TranslateRel_ID(frame1[6], frame1[7], frame1[8], frame2[6], frame2[7], frame2[8]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);

	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_PushMatrix_I();
	phd_TranslateRel_I(bone[1], bone[2], bone[3]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_TranslateRel_I(bone[5], bone[6], bone[7]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_TranslateRel_I(bone[9], bone[10], bone[11]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();
	phd_PopMatrix_I();

	phd_PushMatrix_I();
	phd_TranslateRel_I(bone[13], bone[14], bone[15]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_TranslateRel_I(bone[17], bone[18], bone[19]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_TranslateRel_I(bone[21], bone[22], bone[23]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();
	phd_PopMatrix_I();

	phd_TranslateRel_I(bone[25], bone[26], bone[27]);
	gar_RotYXZsuperpack_I(&rot, &rot2, 0);
	phd_RotYXZ_I(lara.torso_y_rot, lara.torso_x_rot, lara.torso_z_rot);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();

	phd_PushMatrix_I();
	phd_TranslateRel_I(bone[53], bone[54], bone[55]);
	rotcopy = rot;
	rot2copy = rot2;
	gar_RotYXZsuperpack_I(&rotcopy, &rot2copy, 6);
	phd_RotYXZ_I(lara.head_y_rot, lara.head_x_rot, lara.head_z_rot);
	phd_PushMatrix();
	InterpolateMatrix();
	memcpy(matrix, phd_mxptr, 48);
	matrix += 12;
	phd_PopMatrix();
	phd_PopMatrix_I();

	gun = WEAPON_NONE;

	if (lara.gun_status == LG_READY || lara.gun_status == LG_FLARE || lara.gun_status == LG_DRAW_GUNS || lara.gun_status == LG_UNDRAW_GUNS)
		gun = lara.gun_type;

	switch (gun)
	{
	case WEAPON_NONE:
	case WEAPON_FLARE:
	case WEAPON_TORCH:
		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[29], bone[30], bone[31]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();
		phd_PopMatrix_I();

		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[41], bone[42], bone[43]);

		if (lara.flare_control_left)
		{
			rot2 = &lara.left_arm.frame_base[(anims[lara.left_arm.anim_number].interpolation >> 8) *
				(lara.left_arm.frame_number - anims[lara.left_arm.anim_number].frame_base) + 9];
			rot = rot2;
			gar_RotYXZsuperpack_I(&rot, &rot2, 11);
		}
		else
			gar_RotYXZsuperpack_I(&rot, &rot2, 0);

		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();
		break;

	case WEAPON_PISTOLS:
	case WEAPON_UZI:
		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[29], bone[30], bone[31]);
		InterpolateArmMatrix(arms);
		phd_RotYXZ(lara.right_arm.y_rot, lara.right_arm.x_rot, lara.right_arm.z_rot);
		rot = &lara.right_arm.frame_base[(anims[lara.right_arm.anim_number].interpolation >> 8) *
			(lara.right_arm.frame_number - anims[lara.right_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 8);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		
		phd_TranslateRel(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		
		phd_TranslateRel(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix_I();

		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[41], bone[42], bone[43]);
		InterpolateArmMatrix(arms);
		phd_RotYXZ(lara.left_arm.y_rot, lara.left_arm.x_rot, lara.left_arm.z_rot);
		rot = &lara.left_arm.frame_base[(anims[lara.left_arm.anim_number].interpolation >> 8) *
			(lara.left_arm.frame_number - anims[lara.left_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 11);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		break;

	case WEAPON_REVOLVER:
		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[29], bone[30], bone[31]);
		InterpolateArmMatrix(arms);
		phd_RotYXZ_I(lara.torso_y_rot, lara.torso_x_rot, lara.torso_z_rot);
		rot = &lara.right_arm.frame_base[(anims[lara.right_arm.anim_number].interpolation >> 8) *
			(lara.right_arm.frame_number - anims[lara.right_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 8);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix_I();

		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[41], bone[42], bone[43]);
		InterpolateArmMatrix(arms);
		phd_RotYXZ_I(lara.torso_y_rot, lara.torso_x_rot, lara.torso_z_rot);
		rot = &lara.left_arm.frame_base[(anims[lara.left_arm.anim_number].interpolation >> 8) *
			(lara.left_arm.frame_number - anims[lara.left_arm.anim_number].frame_base) + 9];
		gar_RotYXZsuperpack(&rot, 11);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;

		phd_TranslateRel(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack(&rot, 0);
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		break;

	case WEAPON_SHOTGUN:
	case WEAPON_GRENADE:
	case WEAPON_CROSSBOW:
		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[29], bone[30], bone[31]);
		rot2 = &lara.right_arm.frame_base[lara.right_arm.frame_number *
			(anims[lara.right_arm.anim_number].interpolation >> 8) + 9];
		rot = rot2;
		gar_RotYXZsuperpack_I(&rot, &rot2, 8);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[33], bone[34], bone[35]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[37], bone[38], bone[39]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();
		phd_PopMatrix_I();

		phd_PushMatrix_I();
		phd_TranslateRel_I(bone[41], bone[42], bone[43]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[45], bone[46], bone[47]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();

		phd_TranslateRel_I(bone[49], bone[50], bone[51]);
		gar_RotYXZsuperpack_I(&rot, &rot2, 0);
		phd_PushMatrix();
		InterpolateMatrix();
		memcpy(matrix, phd_mxptr, 48);
		matrix += 12;
		phd_PopMatrix();
		break;
	}

	phd_PopMatrix();
	phd_PopMatrix();
	phd_PopMatrix();
}

void CalcLaraMatrices(long flag)
{
	long* bone;
	short* frame;
	short* frmptr[2];
	long rate, frac;
	short jerk;

	bone = &bones[objects[lara_item->object_number].bone_index];
	frac = GetFrames(lara_item, frmptr, &rate);

	if (lara.hit_direction < 0)
	{
		if (frac)
		{
			GLaraShadowframe = GetBoundsAccurate(lara_item);
			Rich_CalcLaraMatrices_Interpolated(frmptr[0], frmptr[1], frac, rate, bone, flag);
			return;
		}
	}

	if (lara.hit_direction < 0)
		frame = *frmptr;
	else
	{
		if (lara.hit_direction == 0)
			jerk = lara.IsDucked ? ANIM_JERK_DUCKF : ANIM_JERK_FORWARD;
		else if (lara.hit_direction == 1)
			jerk = lara.IsDucked ? ANIM_JERK_DUCKR : ANIM_JERK_RIGHT;
		else if (lara.hit_direction == 2)
			jerk = lara.IsDucked ? ANIM_JERK_DUCKB : ANIM_JERK_BACK;
		else
			jerk = lara.IsDucked ? ANIM_JERK_DUCKL : ANIM_JERK_LEFT;

		frame = &anims[jerk].frame_ptr[lara.hit_frame * (anims[jerk].interpolation >> 8)];
	}

	Rich_CalcLaraMatrices_Normal(frame, bone, flag);
}
