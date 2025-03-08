#include "../tomb4/pch.h"
#include "laramisc.h"
#include "objects.h"
#include "laraswim.h"
#include "lara_states.h"
#include "control.h"
#include "collide.h"
#include "sound.h"
#include "effects.h"
#include "rope.h"
#include "../specific/audio.h"
#include "lara.h"
#include "larasurf.h"
#include "../specific/3dmath.h"
#include "spotcam.h"
#include "camera.h"
#include "../specific/input.h"
#include "savegame.h"
#include "gameflow.h"
#include "../specific/file.h"

COLL_INFO mycoll;

static COLL_INFO* lara_coll = &mycoll;
static short cheat_hit_points;

void LaraCheatGetStuff()
{
	lara.num_flares = -1;
	lara.num_small_medipack = -1;
	lara.num_large_medipack = -1;

	if (objects[CROWBAR_ITEM].loaded)
		lara.crowbar = 1;

	lara.lasersight = 1;
	lara.uzis_type_carried = 9;
	lara.shotgun_type_carried = 9;
	lara.sixshooter_type_carried = 9;
	lara.num_uzi_ammo = -1;
	lara.num_revolver_ammo = -1;
	lara.num_shotgun_ammo1 = -1;
}

void LaraCheatyBits()
{
	if (!Gameflow->CheatEnabled)
		return;

	if (input & IN_D)
	{
		LaraCheatGetStuff();
		lara_item->hit_points = 1000;
	}

	if (input & IN_CHEAT)
	{
		lara_item->pos.y_pos -= 128;

		if (lara.water_status != LW_FLYCHEAT)
		{
			lara.water_status = LW_FLYCHEAT;
			lara_item->frame_number = anims[ANIM_SWIMCHEAT].frame_base;
			lara_item->anim_number = ANIM_SWIMCHEAT;
			lara_item->current_anim_state = AS_SWIM;
			lara_item->goal_anim_state = AS_SWIM;
			lara_item->gravity_status = 0;
			lara_item->pos.x_rot = 5460;
			lara_item->fallspeed = 30;
			lara.air = 1800;
			lara.death_count = 0;
			lara.torso_y_rot = 0;
			lara.torso_x_rot = 0;
			lara.head_y_rot = 0;
			lara.head_x_rot = 0;
			cheat_hit_points = lara_item->hit_points;
		}
	}
}

void LaraCheat(ITEM_INFO* item, COLL_INFO* coll)
{
	lara_item->hit_points = 1000;
	LaraUnderWater(item, coll);

	if (input & IN_WALK && !(input & IN_LOOK))
	{
		lara.water_status = LW_ABOVE_WATER;
		item->frame_number = anims[ANIM_STOP].frame_base;
		item->anim_number = ANIM_STOP;
		item->pos.z_rot = 0;
		item->pos.x_rot = 0;
		lara.torso_y_rot = 0;
		lara.torso_x_rot = 0;
		lara.head_y_rot = 0;
		lara.head_x_rot = 0;
		lara.gun_status = 0;
		LaraInitialiseMeshes();
		lara.mesh_effects = 0;
		lara_item->hit_points = cheat_hit_points;
	}
}

void InitialiseLaraLoad(short item_number)
{
	lara.item_number = item_number;
	lara_item = &items[item_number];
}

void InitialiseLaraAnims(ITEM_INFO* item)
{
	if (room[item->room_number].flags & ROOM_UNDERWATER)
	{
		item->anim_number = ANIM_TREAD;
		item->frame_number = anims[ANIM_TREAD].frame_base;
		item->current_anim_state = AS_TREAD;
		item->goal_anim_state = AS_TREAD;
		lara.water_status = LW_UNDERWATER;
		item->fallspeed = 0;
	}
	else
	{
		item->anim_number = ANIM_STOP;
		item->frame_number = anims[ANIM_STOP].frame_base;
		item->current_anim_state = AS_STOP;
		item->goal_anim_state = AS_STOP;
		lara.water_status = LW_ABOVE_WATER;
	}
}

void LaraInitialiseMeshes()
{
	for (int i = 0; i < 15; i++)
	{
		meshes[objects[LARA].mesh_index + i * 2] = meshes[objects[LARA_SKIN].mesh_index + i * 2];
		lara.mesh_ptrs[i] = meshes[objects[LARA].mesh_index + i * 2];
	}

	if (lara.gun_type == WEAPON_GRENADE)
		lara.back_gun = GRENADE_GUN_ANIM;
	else if (lara.shotgun_type_carried)
		lara.back_gun = SHOTGUN_ANIM;
	else if (lara.grenade_type_carried)
		lara.back_gun = GRENADE_GUN_ANIM;

	lara.gun_status = LG_NO_ARMS;
	lara.left_arm.frame_number = 0;
	lara.right_arm.frame_number = 0;
	lara.target = 0;
	lara.right_arm.lock = 0;
	lara.left_arm.lock = 0;
}

void AnimateLara(ITEM_INFO* item)
{
	ANIM_STRUCT* anim;
	short* cmd;
	long speed;
	ushort type;

	item->frame_number++;
	anim = &anims[item->anim_number];

	if (anim->number_changes > 0)
	{
		if (GetChange(item, anim))
		{
			anim = &anims[item->anim_number];
			item->current_anim_state = anim->current_anim_state;
		}
	}

	if (item->frame_number > anim->frame_end)
	{
		if (anim->number_commands > 0)
		{
			cmd = &commands[anim->command_index];

			for (int i = anim->number_commands; i > 0; i--)
			{
				switch (*cmd++)
				{
				case ACMD_SETPOS:
					TranslateItem(item, *cmd, cmd[1], cmd[2]);
					UpdateLaraRoom(item, -381);
					cmd += 3;
					break;

				case ACMD_JUMPVEL:
					item->fallspeed = cmd[0];
					item->speed = cmd[1];
					item->gravity_status = 1;

					if (lara.calc_fallspeed)
					{
						item->fallspeed = lara.calc_fallspeed;
						lara.calc_fallspeed = 0;
					}

					cmd += 2;
					break;

				case ACMD_FREEHANDS:

					if (lara.gun_status != LG_FLARE)
						lara.gun_status = LG_NO_ARMS;

					break;

				case ACMD_PLAYSFX:
				case ACMD_FLIPEFFECT:
					cmd += 2;
					break;
				}
			}
		}

		item->anim_number = anim->jump_anim_num;
		item->frame_number = anim->jump_frame_num;
		anim = &anims[item->anim_number];
		item->current_anim_state = anim->current_anim_state;
	}

	if (anim->number_commands > 0)
	{
		cmd = &commands[anim->command_index];

		for (int i = anim->number_commands; i > 0; i--)
		{
			switch (*cmd++)
			{
			case ACMD_SETPOS:
				cmd += 3;
				break;

			case ACMD_JUMPVEL:
				cmd += 2;
				break;

			case ACMD_PLAYSFX:

				if (item->frame_number == cmd[0])
				{
					type = cmd[1] & 0xC000;

					if (type == SFX_LANDANDWATER ||
						(type == SFX_LANDONLY && (lara.water_surface_dist >= 0 || lara.water_surface_dist == NO_HEIGHT)) ||
						(type == SFX_WATERONLY && lara.water_surface_dist < 0 && lara.water_surface_dist != NO_HEIGHT))
						SoundEffect(cmd[1] & 0x3FFF, &item->pos, SFX_ALWAYS);
				}

				cmd += 2;
				break;

			case ACMD_FLIPEFFECT:

				if (item->frame_number == *cmd)
				{
					FXType = cmd[1] & 0xC000;
					effect_routines[cmd[1] & 0x3FFF](item);
				}

				cmd += 2;
				break;
			}
		}
	}

	if (item->gravity_status)
	{
		speed = anim->velocity + anim->acceleration * (item->frame_number - anim->frame_base - 1);
		item->speed -= speed >> 16;
		speed += anim->acceleration;
		item->speed += speed >> 16;
		item->fallspeed += item->fallspeed < 128 ? 6 : 1;
		item->pos.y_pos += item->fallspeed;
	}
	else
	{
		speed = anim->velocity;

		if (anim->acceleration)
			speed += anim->acceleration * (item->frame_number - anim->frame_base);

		item->speed = speed >> 16;
	}

	if (lara.RopePtr != -1)
		AlignLaraToRope(item);

	if (!lara.IsMoving)
	{
		item->pos.x_pos += (item->speed * phd_sin(lara.move_angle)) >> W2V_SHIFT;
		item->pos.z_pos += (item->speed * phd_cos(lara.move_angle)) >> W2V_SHIFT;
	}
}

void LaraControl(short item_number)
{
	ITEM_INFO* l;
	long oldx, oldy, oldz;
	long wh, wd, hfw, room_water_state;

	l = lara_item;

	if (lara.IsMoving && lara.MoveCount++ > 90)
	{
		lara.IsMoving = 0;
		lara.gun_status = LG_NO_ARMS;
	}

	LaraCheatyBits();

	if (!bDisableLaraControl)
		lara.locationPad = -128;

	oldx = l->pos.x_pos;
	oldy = l->pos.y_pos;
	oldz = l->pos.z_pos;

	if (lara.gun_status == LG_HANDS_BUSY && l->current_anim_state == AS_STOP && l->goal_anim_state == AS_STOP
		&& l->anim_number == ANIM_BREATH && !l->gravity_status)
		lara.gun_status = LG_NO_ARMS;

	if (l->current_anim_state != AS_DASH && DashTimer < 120)
		DashTimer++;

	lara.IsDucked = 0;
	room_water_state = room[l->room_number].flags & ROOM_UNDERWATER;
	wd = GetWaterDepth(l->pos.x_pos, l->pos.y_pos, l->pos.z_pos, l->room_number);
	wh = GetWaterHeight(l->pos.x_pos, l->pos.y_pos, l->pos.z_pos, l->room_number);

	if (wh != NO_HEIGHT)
		hfw = l->pos.y_pos - wh;
	else
		hfw = NO_HEIGHT;

	lara.water_surface_dist = -hfw;

	if (lara.vehicle == NO_ITEM)
	{
		WadeSplash(l, wh, wd);

		if (lara.vehicle == NO_ITEM)
		{
			switch (lara.water_status)
			{
			case LW_ABOVE_WATER:

				if (hfw != NO_HEIGHT && hfw >= 265)
				{
					if (wd <= 474)
					{
						if (hfw > 256)
						{
							lara.water_status = LW_WADE;

							if (!l->gravity_status)
								l->goal_anim_state = AS_STOP;
						}
					}
					else if (room_water_state)
					{
						lara.air = 1800;
						lara.water_status = LW_UNDERWATER;
						l->gravity_status = 0;
						l->pos.y_pos += 100;
						UpdateLaraRoom(lara_item, 0);
						StopSoundEffect(SFX_LARA_FALL);

						if (l->current_anim_state == AS_SWANDIVE)
						{
							l->pos.x_rot = -8190;
							l->goal_anim_state = AS_DIVE;
							AnimateLara(l);
							l->fallspeed *= 2;
						}
						else if (l->current_anim_state == AS_FASTDIVE)
						{
							l->pos.x_rot = -15470;
							l->goal_anim_state = AS_DIVE;
							AnimateLara(l);
							l->fallspeed *= 2;
						}
						else
						{
							l->pos.x_rot = -8190;
							l->anim_number = 112;
							l->frame_number = anims[112].frame_base;
							l->current_anim_state = AS_DIVE;
							l->goal_anim_state = AS_SWIM;
							l->fallspeed = 3 * l->fallspeed / 2;
						}

						lara.torso_x_rot = 0;
						lara.torso_y_rot = 0;
						lara.head_x_rot = 0;
						lara.head_y_rot = 0;
						Splash(lara_item);
					}
				}
				else
					break;

			case LW_WADE:
				camera.target_elevation = -4004;

				if (hfw >= 256)
				{
					if (hfw > 730)
					{
						lara.water_status = LW_SURFACE;
						l->pos.y_pos += 1 - hfw;

						switch (l->current_anim_state)
						{
						case AS_BACK:
							l->current_anim_state = AS_SURFBACK;
							l->goal_anim_state = AS_SURFBACK;
							l->anim_number = 140;
							l->frame_number = anims[140].frame_base;
							break;

						case AS_STEPRIGHT:
							l->current_anim_state = AS_SURFRIGHT;
							l->goal_anim_state = AS_SURFRIGHT;
							l->anim_number = 144;
							l->frame_number = anims[144].frame_base;
							break;

						case AS_STEPLEFT:
							l->current_anim_state = AS_SURFLEFT;
							l->goal_anim_state = AS_SURFLEFT;
							l->anim_number = 143;
							l->frame_number = anims[143].frame_base;
							break;

						default:
							l->current_anim_state = AS_SURFSWIM;
							l->goal_anim_state = AS_SURFSWIM;
							l->anim_number = 116;
							l->frame_number = anims[116].frame_base;
							break;
						}

						l->gravity_status = 0;
						l->fallspeed = 0;
						lara.dive_count = 0;
						l->pos.x_rot = 0;
						lara_item->pos.z_rot = 0;
						lara.torso_x_rot = 0;
						lara.torso_y_rot = 0;
						lara.head_x_rot = 0;
						lara.head_y_rot = 0;
						UpdateLaraRoom(l, 0);
					}
				}
				else
				{
					lara.water_status = LW_ABOVE_WATER;

					if (l->current_anim_state == AS_WADE)
						l->goal_anim_state = AS_RUN;
				}

				break;

			case LW_UNDERWATER:

				if (!room_water_state)
				{
					if (wd != NO_HEIGHT && abs(hfw) < 256)
					{
						lara.water_status = LW_SURFACE;
						l->pos.y_pos = wh;
						l->anim_number = 114;
						l->frame_number = anims[114].frame_base;
						l->goal_anim_state = AS_SURFTREAD;
						l->current_anim_state = AS_SURFTREAD;
						l->fallspeed = 0;
						lara.dive_count = 11;
						l->pos.x_rot = 0;
						lara_item->pos.z_rot = 0;
						lara.torso_x_rot = 0;
						lara.torso_y_rot = 0;
						lara.head_x_rot = 0;
						lara.head_y_rot = 0;
						UpdateLaraRoom(l, -381);
						SoundEffect(SFX_LARA_BREATH, &l->pos, SFX_ALWAYS);
					}
					else
					{
						lara.water_status = LW_ABOVE_WATER;
						l->anim_number = ANIM_FALLDOWN;
						l->frame_number = anims[ANIM_FALLDOWN].frame_base;
						l->goal_anim_state = AS_FORWARDJUMP;
						l->current_anim_state = AS_FORWARDJUMP;
						l->speed = l->fallspeed / 4;
						l->gravity_status = 1;
						l->fallspeed = 0;
						l->pos.x_rot = 0;
						lara_item->pos.z_rot = 0;
						lara.torso_x_rot = 0;
						lara.torso_y_rot = 0;
						lara.head_x_rot = 0;
						lara.head_y_rot = 0;
					}
				}

				break;

			case LW_SURFACE:

				if (!room_water_state)
				{
					if (hfw <= 256)
					{
						lara.water_status = LW_ABOVE_WATER;
						l->anim_number = ANIM_FALLDOWN;
						l->frame_number = anims[ANIM_FALLDOWN].frame_base;
						l->goal_anim_state = AS_FORWARDJUMP;
						l->current_anim_state = AS_FORWARDJUMP;
						l->speed = l->fallspeed / 4;
						l->gravity_status = 1;
					}
					else
					{
						lara.water_status = LW_WADE;
						l->anim_number = ANIM_BREATH;
						l->frame_number = anims[ANIM_BREATH].frame_base;
						l->current_anim_state = AS_STOP;
						l->goal_anim_state = AS_WADE;
						AnimateItem(l);
					}

					l->fallspeed = 0;
					l->pos.x_rot = 0;
					lara_item->pos.z_rot = 0;
					lara.torso_x_rot = 0;
					lara.torso_y_rot = 0;
					lara.head_x_rot = 0;
					lara.head_y_rot = 0;
				}

				break;
			}
		}
	}

//	S_SetReverbType(room[l->room_number].ReverbType);	//empty func call

	if (l->hit_points <= 0)
	{
		l->hit_points = -1;

		if (!lara.death_count)
			S_CDStop();

		lara.death_count++;

		if (lara_item->flags & IFL_INVISIBLE)
		{
			lara.death_count++;
			return;
		}
	}

	switch (lara.water_status)
	{
	case LW_ABOVE_WATER:
	case LW_WADE:

		if (lara.vehicle == NO_ITEM)
			lara.air = 1800;

		LaraAboveWater(l, lara_coll);
		break;

	case LW_UNDERWATER:

		if (l->hit_points >= 0)
		{
			lara.air--;

			if (lara.air < 0)
			{
				lara.air = -1;
				l->hit_points -= 5;
			}
		}

		LaraUnderWater(lara_item, lara_coll);
		break;

	case LW_SURFACE:

		if (l->hit_points >= 0)
		{
			lara.air += 10;

			if (lara.air > 1800)
				lara.air = 1800;
		}

		LaraSurface(l, lara_coll);
		break;

	case LW_FLYCHEAT:
		LaraCheat(l, lara_coll);
		break;
	}

	savegame.Game.Distance += phd_sqrt(SQUARE(l->pos.x_pos - oldx) + SQUARE(l->pos.y_pos - oldy) + SQUARE(l->pos.z_pos - oldz));
}
