#include "../tomb4/pch.h"
#include "lara.h"
#include "lara_states.h"
#include "objects.h"
#include "laramisc.h"
#include "collide.h"
#include "larafire.h"
#include "control.h"
#include "bike.h"
#include "jeep.h"
#include "draw.h"
#include "../specific/audio.h"
#include "../specific/3dmath.h"
#include "sound.h"

void LaraAboveWater(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->old.x = item->pos.x_pos;
	coll->old.y = item->pos.y_pos;
	coll->old.z = item->pos.z_pos;
	coll->old_anim_state = item->current_anim_state;
	coll->old_anim_number = item->anim_number;
	coll->old_frame_number = item->frame_number;
	coll->radius = 100;
	coll->trigger = 0;
	coll->slopes_are_walls = 0;
	coll->slopes_are_pits = 0;
	coll->lava_is_pit = 0;
	coll->enable_baddie_push = 1;
	coll->enable_spaz = 1;

	if (input & IN_LOOK && lara.look)
		LookLeftRight();
	else
		ResetLook();

	lara.look = 1;

	if (lara.vehicle != NO_ITEM)
	{
		if (items[lara.vehicle].object_number == MOTORBIKE)
			BikeControl(lara.vehicle);
		else
			JeepControl(lara.vehicle);

		return;
	}

	if (item->current_anim_state != AS_DEATH && gfLevelFlags & GF_TRAIN && item->pos.y_pos >= 0)
	{
		if (item->pos.z_pos <= 51900 || item->pos.z_pos >= 53554)
		{
			ForcedFixedCamera.y = -1024;
			ForcedFixedCamera.z = item->pos.z_pos > 51900 ? 55296 : 50176;
		}
		else
		{
			ForcedFixedCamera.y = -384;
			ForcedFixedCamera.z = 52736;
		}

		ForcedFixedCamera.x = item->pos.x_pos;
		ForcedFixedCamera.room_number = item->room_number;
		UseForcedFixedCamera = 1;
		item->anim_number = ANIM_TRAINDEATH;
		item->frame_number = anims[ANIM_TRAINDEATH].frame_base;
		item->current_anim_state = AS_DEATH;
		item->pos.y_rot = -16384;
		item->gravity_status = 0;
		item->fallspeed = 0;
		item->speed = 0;
	}

	lara_control_routines[item->current_anim_state](item, coll);

	if (item->pos.z_rot < -182)
		item->pos.z_rot += 182;
	else if (item->pos.z_rot > 182)
		item->pos.z_rot -= 182;
	else item->pos.z_rot = 0;

	if (lara.turn_rate < -364)
		lara.turn_rate += 364;
	else if (lara.turn_rate > 364)
		lara.turn_rate -= 364;
	else
		lara.turn_rate = 0;

	item->pos.y_rot += lara.turn_rate;
	AnimateLara(item);
	LaraBaddieCollision(item, coll);

	if (lara.vehicle == NO_ITEM)
		lara_collision_routines[item->current_anim_state](item, coll);

	UpdateLaraRoom(item, -381);
	LaraGun();
	TestTriggers(coll->trigger, 0, 0);
}

void SetCornerAnim(ITEM_INFO* item, COLL_INFO* coll, short rot, short flip)
{
	lara.look = 0;
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;

#ifdef GENERAL_FIXES
	if (lara_item->hit_points > 0)
#else
	if (input & IN_ACTION && lara_item->hit_points > 0)
#endif
	{
		if (flip)
		{
			if (lara.IsClimbing)
			{
				item->anim_number = ANIM_CLIMBSTNC;
				item->frame_number = anims[ANIM_CLIMBSTNC].frame_base;
				item->goal_anim_state = AS_CLIMBSTNC;
				item->current_anim_state = AS_CLIMBSTNC;
			}
			else
			{
				item->anim_number = ANIM_GRABLEDGE;
				item->frame_number = anims[ANIM_GRABLEDGE].frame_base + 21;
				item->goal_anim_state = AS_HANG;
				item->current_anim_state = AS_HANG;
			}

			coll->old.x = lara.CornerX;
			item->pos.x_pos = lara.CornerX;
			coll->old.z = lara.CornerZ;
			item->pos.z_pos = lara.CornerZ;
			item->pos.y_rot += rot;
		}
	}
	else
	{
		item->anim_number = ANIM_FALLDOWN;
		item->frame_number = anims[ANIM_FALLDOWN].frame_base;
		item->goal_anim_state = AS_FORWARDJUMP;
		item->current_anim_state = AS_FORWARDJUMP;
		item->gravity_status = 1;
		item->speed = 2;
		item->pos.y_pos += 256;
		item->fallspeed = 1;
		lara.gun_status = LG_NO_ARMS;
		item->pos.y_rot += rot / 2;
	}
}

long CanLaraHangSideways(ITEM_INFO* item, COLL_INFO* coll, short angle)
{
	long oldx, oldz, x, z, res;

	oldx = item->pos.x_pos;
	oldz = item->pos.z_pos;
	x = item->pos.x_pos;
	z = item->pos.z_pos;
	lara.move_angle = angle + item->pos.y_rot;

	switch ((ushort)(lara.move_angle + 8192) >> 14)
	{
	case NORTH:
		z += 16;
		break;

	case EAST:
		x += 16;
		break;

	case SOUTH:
		z -= 16;
		break;

	case WEST:
		x -= 16;
		break;
	}

	item->pos.x_pos = x;
	item->pos.z_pos = z;
	coll->old.y = item->pos.y_pos;
	res = LaraHangTest(item, coll);
	item->pos.x_pos = oldx;
	item->pos.z_pos = oldz;
	lara.move_angle = angle + item->pos.y_rot;
	return !res;
}

void lara_as_hang(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.IsClimbing = 0;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (input & IN_LOOK)
		LookUpDown();

	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = 0;
	camera.target_elevation = -10920;
}

void lara_col_hang(ITEM_INFO* item, COLL_INFO* coll)
{
	long flag;

	item->fallspeed = 0;
	item->gravity_status = 0;

#ifdef GENERAL_FIXES
	if (item->anim_number == ANIM_GRABLEDGE)
#else
	if (item->anim_number == ANIM_GRABLEDGE && item->frame_number == anims[ANIM_GRABLEDGE].frame_base + 21)
#endif
	{
		if (input & IN_LEFT || input & IN_LSTEP)
		{
			if (CanLaraHangSideways(item, coll, -16384))
			{
				item->goal_anim_state = AS_HANGLEFT;
				return;
			}

			flag = LaraHangLeftCornerTest(item, coll);

			if (flag)
			{
				if (flag <= 0)
					item->goal_anim_state = AS_CORNERINTL;
				else
					item->goal_anim_state = AS_CORNEREXTL;

				return;
			}
		}

		if (input & IN_RIGHT || input & IN_RSTEP)
		{
			if (CanLaraHangSideways(item, coll, 16384))
			{
				item->goal_anim_state = AS_HANGRIGHT;
				return;
			}

			flag = LaraHangRightCornerTest(item, coll);

			if (flag)
			{
				if (flag <= 0)
					item->goal_anim_state = AS_CORNERINTR;
				else
					item->goal_anim_state = AS_CORNEREXTR;

				return;
			}
		}
	}

	lara.move_angle = item->pos.y_rot;
	LaraHangTest(item, coll);

#ifdef GENERAL_FIXES
	if (item->anim_number == ANIM_GRABLEDGE)
#else
	if (item->anim_number == ANIM_GRABLEDGE && item->frame_number == anims[ANIM_GRABLEDGE].frame_base + 21)
#endif
	{
		if (input & IN_FORWARD)
		{
			if (coll->front_floor > -850 && coll->front_floor < -650 && ABS(coll->left_floor2 - coll->right_floor2) < 60 && !coll->hit_static)
			{
				if (coll->front_floor - coll->front_ceiling >= 0 &&
					coll->front_floor - coll->left_ceiling2 >= 0 &&
					coll->front_floor - coll->right_ceiling2 >= 0)
				{
					if (input & IN_DUCK)
					{
						item->goal_anim_state = AS_HANG2DUCK;
						item->required_anim_state = AS_DUCK;
					}
					else if (input & IN_WALK)
						item->goal_anim_state = AS_GYMNAST;
					else
						item->goal_anim_state = AS_NULL;

					return;
				}

				if (coll->front_floor - coll->front_ceiling >= -256 &&
					coll->front_floor - coll->left_ceiling2 >= -256 &&
					coll->front_floor - coll->right_ceiling2 >= -256)
				{
#ifndef GENERAL_FIXES
					if (input & IN_DUCK)
#endif
					{
						item->goal_anim_state = AS_HANG2DUCK;
						item->required_anim_state = AS_DUCK;
					}

					return;
				}
			}

			if (lara.climb_status && coll->mid_ceiling <= -256 && ABS(coll->left_ceiling2 - coll->right_ceiling2) < 60)
			{
				if (LaraTestClimbStance(item, coll))
					item->goal_anim_state = AS_CLIMBSTNC;
				else
				{
					item->anim_number = ANIM_HANGUP;
					item->frame_number = anims[ANIM_HANGUP].frame_base;
					item->goal_anim_state = AS_HANG;
					item->current_anim_state = AS_HANG;
				}
			}
			else
			{
				if (input & IN_BACK && lara.climb_status && coll->mid_floor > 344)
				{
					if (input & IN_BACK && lara.climb_status && coll->mid_floor > 344)
					{
						if (LaraTestClimbStance(item, coll))
							item->goal_anim_state = AS_CLIMBSTNC;
						else
						{
							item->anim_number = ANIM_HANGDOWN;
							item->frame_number = anims[ANIM_HANGDOWN].frame_base;
							item->goal_anim_state = AS_HANG;
							item->current_anim_state = AS_HANG;
						}
					}
				}
			}
		}
		else if (input & IN_BACK && lara.climb_status && coll->mid_floor > 344)
		{
			if (LaraTestClimbStance(item, coll))
				item->goal_anim_state = AS_CLIMBSTNC;
			else
			{
				item->anim_number = ANIM_HANGDOWN;
				item->frame_number = anims[ANIM_HANGDOWN].frame_base;
				item->goal_anim_state = AS_HANG;
				item->current_anim_state = AS_HANG;
			}
		}
	}
}

short LaraCeilingFront(ITEM_INFO* item, short ang, long dist, long h)
{
	long x, y, z, height;
	short room_num;

	x = item->pos.x_pos + ((dist * phd_sin(ang)) >> 14);
	y = item->pos.y_pos - h;
	z = item->pos.z_pos + ((dist * phd_cos(ang)) >> 14);
	room_num = item->room_number;
	height = GetCeiling(GetFloor(x, y, z, &room_num), x, y, z);

	if (height != NO_HEIGHT)
		height += h - item->pos.y_pos;

	return (short)height;
}

short LaraFloorFront(ITEM_INFO* item, short ang, long dist)
{
	long x, y, z, height;
	short room_num;

	room_num = item->room_number;
	x = item->pos.x_pos + ((dist * phd_sin(ang)) >> 14);
	y = item->pos.y_pos - 762;
	z = item->pos.z_pos + ((dist * phd_cos(ang)) >> 14);
	height = GetHeight(GetFloor(x, y, z, &room_num), x, y, z);

	if (height != NO_HEIGHT)
		height -= item->pos.y_pos;

	return (short)height;
}

long LaraFallen(ITEM_INFO* item, COLL_INFO* coll)
{
	if (lara.water_status == LW_WADE || coll->mid_floor <= 384)
		return 0;

	item->anim_number = ANIM_FALLDOWN;
	item->current_anim_state = AS_FORWARDJUMP;
	item->goal_anim_state = AS_FORWARDJUMP;
	item->frame_number = anims[ANIM_FALLDOWN].frame_base;
	item->fallspeed = 0;
	item->gravity_status = 1;
	return 1;
}

long TestLaraSlide(ITEM_INFO* item, COLL_INFO* coll)
{
	static short old_ang = 1;
	short ang_diff, ang;

	if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2)
		return 0;

	ang = 0;

	if (coll->tilt_x > 2)
		ang = -16384;
	else if (coll->tilt_x < -2)
		ang = 16384;

	if (coll->tilt_z > 2 && coll->tilt_z > ABS(coll->tilt_x))
		ang = -32768;
	else if (coll->tilt_z < -2 && (-coll->tilt_z > ABS(coll->tilt_x)))
		ang = 0;

	ang_diff = ang - item->pos.y_rot;
	ShiftItem(item, coll);

	if (ang_diff >= -16384 && ang_diff <= 16384)
	{
		if (item->current_anim_state != AS_SLIDE || old_ang != ang)
		{
			item->anim_number = ANIM_SLIDE;
			item->frame_number = anims[ANIM_SLIDE].frame_base;
			item->goal_anim_state = AS_SLIDE;
			item->current_anim_state = AS_SLIDE;
			item->pos.y_rot = ang;
		}
	}
	else
	{
		if (item->current_anim_state != AS_SLIDEBACK || old_ang != ang)
		{
			item->anim_number = ANIM_SLIDEBACK;
			item->frame_number = anims[ANIM_SLIDEBACK].frame_base;
			item->goal_anim_state = AS_SLIDEBACK;
			item->current_anim_state = AS_SLIDEBACK;
			item->pos.y_rot = ang - 32768;
		}
	}

	lara.move_angle = ang;
	old_ang = ang;
	return 1;
}

long LaraHitCeiling(ITEM_INFO* item, COLL_INFO* coll)
{
	if (coll->coll_type != CT_TOP && coll->coll_type != CT_CLAMP)
		return 0;

	item->pos.x_pos = coll->old.x;
	item->pos.y_pos = coll->old.y;
	item->pos.z_pos = coll->old.z;
	item->goal_anim_state = AS_STOP;
	item->current_anim_state = AS_STOP;
	item->anim_number = ANIM_STOP;
	item->frame_number = anims[ANIM_STOP].frame_base;
	item->speed = 0;
	item->fallspeed = 0;
	item->gravity_status = 0;
	return 1;
}

void lara_as_duckl(ITEM_INFO* item, COLL_INFO* coll)
{
	if ((input & (IN_DUCK | IN_LEFT)) != (IN_DUCK | IN_LEFT) || item->hit_points <= 0)
		item->goal_anim_state = AS_DUCK;

	item->pos.y_rot -= 273;
}

void lara_as_duckr(ITEM_INFO* item, COLL_INFO* coll)
{
	if ((input & (IN_DUCK | IN_LEFT)) != (IN_DUCK | IN_LEFT) || item->hit_points <= 0)
		item->goal_anim_state = AS_DUCK;

	item->pos.y_rot += 273;
}

void lara_col_ducklr(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.IsDucked = 1;

	if (input & IN_LOOK)
		LookUpDown();

	item->gravity_status = 0;
	item->fallspeed = 0;
	lara.move_angle = item->pos.y_rot;
	coll->facing = item->pos.y_rot;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_walls = 1;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 400);

	if (LaraFallen(item, coll))
		lara.gun_status = LG_NO_ARMS;
	else if (!TestLaraSlide(item, coll))
	{
		if (coll->mid_ceiling < -362)
			lara.keep_ducked = 0;
		else
			lara.keep_ducked = 1;

		ShiftItem(item, coll);

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_duck(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_spaz = 1;
	coll->enable_baddie_push = 1;
	lara.IsDucked = 1;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_ALL4S;
		return;
	}

	if (input & IN_LOOK)
		LookUpDown();

	if (input & (IN_FORWARD | IN_BACK))
	{
		if (input & IN_DUCK || lara.keep_ducked)
		{
			if (lara.gun_status == LG_NO_ARMS && lara.water_status != LW_WADE)
			{
				if (lara_item->anim_number == ANIM_DUCKBREATHE || lara_item->anim_number == 245)
				{
					if (!(input & (IN_B | IN_DRAW)))
					{
						if (lara.gun_type != WEAPON_FLARE || lara.flare_age < 900 && lara.flare_age)
						{
							lara.torso_y_rot = 0;
							lara.torso_x_rot = 0;
							item->goal_anim_state = AS_ALL4S;
						}
					}
				}
			}
		}
	}
}

void lara_col_duck(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 0;
	item->fallspeed = 0;
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = 384;
	coll->facing = item->pos.y_rot;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_walls = 1;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 400);

	if (LaraFallen(item, coll))
		lara.gun_status = LG_NO_ARMS;
	else if (!TestLaraSlide(item, coll))
	{
		if (coll->mid_ceiling < -362)
			lara.keep_ducked = 0;
		else
			lara.keep_ducked = 1;

		ShiftItem(item, coll);

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;

		if (input & IN_DUCK && lara.water_status != LW_WADE || lara.keep_ducked || item->anim_number != ANIM_DUCKBREATHE)
		{
			if (input & IN_LEFT)
				item->goal_anim_state = AS_DUCKROTL;
			else if (input & IN_RIGHT)
				item->goal_anim_state = AS_DUCKROTR;
		}
		else
			item->goal_anim_state = AS_STOP;
	}
}

void lara_as_all4s(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_DEATH;
		return;
	}

	if (input & IN_LOOK)
		LookUpDown();

	lara.torso_x_rot = 0;
	lara.torso_y_rot = 0;
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 1;

	if (item->anim_number == ANIM_DUCK_TO_ALL4S)
		lara.gun_status = LG_HANDS_BUSY;

	camera.target_elevation = -4186;
}

void lara_col_all4s(ITEM_INFO* item, COLL_INFO* coll)
{
	long slope, x, z, collided;
	short height;

	item->fallspeed = 0;
	item->gravity_status = 0;

	if (item->goal_anim_state == AS_CRAWL2HANG)
		return;

	lara.move_angle = item->pos.y_rot;
	coll->facing = item->pos.y_rot;
	coll->radius = 200;
	coll->bad_pos = 255;
	coll->bad_neg = -255;
	coll->bad_ceiling = 400;
	coll->slopes_are_walls = 1;
	coll->slopes_are_pits = 1;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 400);

	if (LaraFallen(item, coll))
		lara.gun_status = LG_NO_ARMS;
	else if (!TestLaraSlide(item, coll))
	{
		slope = ABS(coll->left_floor2 - coll->right_floor2) >= 120;
		lara.keep_ducked = coll->mid_ceiling >= -362;
		ShiftItem(item, coll);

		if (coll->mid_floor != NO_HEIGHT && coll->mid_floor > -256)
			item->pos.y_pos += coll->mid_floor;

		if (input & IN_DUCK || lara.keep_ducked && (!(input & (IN_B | IN_DRAW)) || input & IN_FORWARD) && lara.water_status != LW_WADE)
		{
			if (item->anim_number == ANIM_ALL4S || item->anim_number == ANIM_ALL4S2)
			{
				if (input & IN_FORWARD)
				{
					height = LaraFloorFront(item, item->pos.y_rot, 256);

					if (height < 255 && height > -255 && height_type != BIG_SLOPE)
						item->goal_anim_state = AS_CRAWL;
				}
				else if (input & IN_BACK)
				{
					height = LaraCeilingFront(item, item->pos.y_rot, -300, 128);

					if (height == NO_HEIGHT || height > 256)
						return;

					height = LaraFloorFront(item, item->pos.y_rot, -300);

					if (height < 255 && height > -255 && height_type != BIG_SLOPE)
						item->goal_anim_state = AS_CRAWLBACK;
					else
					{
						if (input & IN_ACTION && height > 768 && !slope)
						{
							x = item->pos.x_pos;
							z = item->pos.z_pos;
							item->pos.x_pos -= 100 * phd_sin(coll->facing) >> 14;
							item->pos.z_pos -= 100 * phd_cos(coll->facing) >> 14;
							collided = GetCollidedObjects(item, 100, 1, CollidedItems, CollidedStatics, 0);
							item->pos.x_pos = x;
							item->pos.z_pos = z;

							if (!collided)
							{
								switch ((ushort)(item->pos.y_rot + 8192) >> 14)
								{
								case NORTH:
									item->pos.y_rot = 0;
									item->pos.z_pos = (item->pos.z_pos & ~0x3FF) + 225;
									break;

								case EAST:
									item->pos.y_rot = 16384;
									item->pos.x_pos = (item->pos.x_pos & ~0x3FF) + 225;
									break;

								case SOUTH:
									item->pos.y_rot = -32768;
									item->pos.z_pos = (item->pos.z_pos | 0x3FF) - 225;
									break;

								case WEST:
									item->pos.y_rot = -16384;
									item->pos.x_pos = (item->pos.x_pos | 0x3FF) - 225;
									break;
								}

								item->goal_anim_state = AS_CRAWL2HANG;
							}
						}
					}
				}
				else if (input & IN_LEFT)
				{
					item->anim_number = ANIM_ALL4TURNL;
					item->frame_number = anims[ANIM_ALL4TURNL].frame_base;
					item->current_anim_state = AS_ALL4TURNL;
					item->goal_anim_state = AS_ALL4TURNL;
				}
				else if (input & IN_RIGHT)
				{
					item->anim_number = ANIM_ALL4TURNR;
					item->frame_number = anims[ANIM_ALL4TURNR].frame_base;
					item->current_anim_state = AS_ALL4TURNR;
					item->goal_anim_state = AS_ALL4TURNR;
				}
			}
		}
		else
			item->goal_anim_state = AS_DUCK;
	}
}

void lara_as_crawl(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_ALL4S;
		return;
	}

	if (input & IN_LOOK)
		LookUpDown();

	lara.torso_x_rot = 0;
	lara.torso_y_rot = 0;
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 1;
	camera.target_elevation = -4186;

	if (input & IN_FORWARD && (input & IN_DUCK || lara.keep_ducked) && lara.water_status != LW_WADE)
	{
		if (input & IN_LEFT)
		{
			lara.turn_rate -= 409;

			if (lara.turn_rate < -546)
				lara.turn_rate = -546;
		}
		else if (input & IN_RIGHT)
		{
			lara.turn_rate += 409;

			if (lara.turn_rate > 546)
				lara.turn_rate = 546;
		}
	}
	else
		item->goal_anim_state = AS_ALL4S;
}

void lara_col_crawl(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 0;
	item->fallspeed = 0;
	lara.move_angle = item->pos.y_rot;
	coll->radius = 200;
	coll->bad_pos = 255;
	coll->bad_neg = -255;
	coll->bad_ceiling = 400;
	coll->slopes_are_pits = 1;
	coll->slopes_are_walls = 1;
	coll->facing = lara.move_angle;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, -400);

	if (LaraDeflectEdgeDuck(item, coll))
	{
		item->current_anim_state = AS_ALL4S;
		item->goal_anim_state = AS_ALL4S;

		if (item->anim_number != ANIM_ALL4S)
		{
			item->anim_number = ANIM_ALL4S;
			item->frame_number = anims[ANIM_ALL4S].frame_base;
		}
	}
	else if (LaraFallen(item, coll))
		lara.gun_status = LG_NO_ARMS;
	else if (!TestLaraSlide(item, coll))
	{
		ShiftItem(item, coll);

		if (coll->mid_floor != NO_HEIGHT && coll->mid_floor > -256)
			item->pos.y_pos += coll->mid_floor;
	}
}

long LaraDeflectEdgeDuck(ITEM_INFO* item, COLL_INFO* coll)
{
	if (coll->coll_type == CT_FRONT || coll->coll_type == CT_TOP_FRONT)
	{
		ShiftItem(item, coll);
		item->gravity_status = 0;
		item->speed = 0;
		return 1;
	}
	else if (coll->coll_type == CT_LEFT)
	{
		ShiftItem(item, coll);
		item->pos.y_rot += 364;
	}
	else if (coll->coll_type == CT_RIGHT)
	{
		ShiftItem(item, coll);
		item->pos.y_rot -= 364;
	}

	return 0;
}

void lara_as_all4turnl(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 1;
	lara.torso_y_rot = 0;
	lara.torso_x_rot = 0;
	camera.target_elevation = -4186;
	item->pos.y_rot -= 273;

	if (item->hit_points <= 0 || !(input & IN_LEFT))
		item->goal_anim_state = AS_ALL4S;
}

void lara_as_all4turnr(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 1;
	lara.torso_y_rot = 0;
	lara.torso_x_rot = 0;
	camera.target_elevation = -4186;
	item->pos.y_rot += 273;

	if (item->hit_points <= 0 || !(input & IN_RIGHT))
		item->goal_anim_state = AS_ALL4S;
}

void lara_col_all4turnlr(ITEM_INFO* item, COLL_INFO* coll)
{
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 400);

	if (!TestLaraSlide(item, coll) && coll->mid_floor != NO_HEIGHT && coll->mid_floor > -256)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_crawlb(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0 || lara.water_status == LW_WADE)
	{
		item->goal_anim_state = AS_ALL4S;
		return;
	}

	if (input & IN_LOOK)
		LookUpDown();

	coll->enable_spaz = 0;
	coll->enable_baddie_push = 1;
	lara.torso_x_rot = 0;
	lara.torso_y_rot = 0;
	camera.target_elevation = -4186;

	if (input & IN_BACK)
	{
		if (input & IN_RIGHT)
		{
			lara.turn_rate -= 409;

			if (lara.turn_rate < -546)
				lara.turn_rate = -546;
		}
		else if (input & IN_LEFT)
		{
			lara.turn_rate += 409;

			if (lara.turn_rate > 546)
				lara.turn_rate = 546;
		}
	}
	else
		item->goal_anim_state = AS_ALL4S;
}

void lara_col_crawlb(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 0;
	item->fallspeed = 0;
	coll->radius = 250;
	coll->bad_pos = 255;
	coll->bad_neg = -255;
	coll->bad_ceiling = 400;
	coll->slopes_are_pits = 1;
	coll->slopes_are_walls = 1;
	lara.move_angle = item->pos.y_rot + 32768;
	coll->facing = lara.move_angle;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 400);

	if (LaraDeflectEdgeDuck(item, coll))
	{
		item->current_anim_state = AS_ALL4S;
		item->goal_anim_state = AS_ALL4S;

		if (item->anim_number != ANIM_ALL4S)
		{
			item->anim_number = ANIM_ALL4S;
			item->frame_number = anims[ANIM_ALL4S].frame_base;
		}
	}
	else if (LaraFallen(item, coll))
		lara.gun_status = LG_NO_ARMS;
	else if (!TestLaraSlide(item, coll))
	{
		ShiftItem(item, coll);

		if (coll->mid_floor != NO_HEIGHT && coll->mid_floor > -256)
			item->pos.y_pos += coll->mid_floor;

		lara.move_angle = item->pos.y_rot;
	}
}

void lara_col_crawl2hang(ITEM_INFO* item, COLL_INFO* coll)
{
	short* bounds;
	long edge, edge_catch;
	short angle;

	camera.target_angle = 0;
	camera.target_elevation = -10920;
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;

	if (item->anim_number != ANIM_CRAWL_TO_HANG)
		return;

	item->fallspeed = 512;
	item->pos.y_pos += 255;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	lara.move_angle = item->pos.y_rot;
	coll->facing = lara.move_angle;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 870);
	edge_catch = LaraTestEdgeCatch(item, coll, &edge);

	if (edge_catch <= 0 && !LaraTestHangOnClimbWall(item, coll))
		return;

	angle = item->pos.y_rot;

	if (angle >= -6370 && angle <= 6370)
		angle = 0;
	else if (angle >= 10014 && angle <= 22754)
		angle = 16384;
	else if (angle >= 26397 || angle <= -26397)
		angle = -32768;
	else if (angle >= -22754 && angle <= -10014)
		angle = -16384;

	if (angle & 0x3FFF)
		return;

	if (TestHangSwingIn(item, angle))
	{
		lara.head_y_rot = 0;
		lara.head_x_rot = 0;
		lara.torso_y_rot = 0;
		lara.torso_x_rot = 0;
		item->anim_number = ANIM_HANG2STOP;
		item->frame_number = anims[ANIM_HANG2STOP].frame_base;
		item->current_anim_state = AS_HANG2;
		item->goal_anim_state = AS_HANG2;
	}
	else
	{
		item->anim_number = ANIM_GRABLEDGE;
		item->frame_number = anims[ANIM_GRABLEDGE].frame_base;
		item->current_anim_state = AS_HANG;
		item->goal_anim_state = AS_HANG;
	}

	bounds = GetBoundsAccurate(item);

	if (edge_catch <= 0)
		item->pos.y_pos = edge - bounds[2];
	else
	{
		item->pos.y_pos += coll->front_floor - bounds[2];
		item->pos.x_pos += coll->shift.x;
		item->pos.z_pos += coll->shift.z;
	}

	item->gravity_status = 1;
	item->pos.y_rot = angle;
	item->speed = 2;
	item->fallspeed = 1;
	lara.gun_status = LG_HANDS_BUSY;
}

void lara_as_dash(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0 || !DashTimer || !(input & IN_SPRINT) || lara.water_status == LW_WADE)
	{
		item->goal_anim_state = AS_RUN;
		return;
	}

	DashTimer--;

	if (input & IN_DUCK && (lara.gun_status == LG_NO_ARMS || lara.gun_type == WEAPON_NONE || lara.gun_type == WEAPON_PISTOLS ||
		lara.gun_type == WEAPON_REVOLVER || lara.gun_type == WEAPON_UZI || lara.gun_type == WEAPON_FLARE))
	{
		item->goal_anim_state = AS_DUCK;
		return;
	}

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -728)
			lara.turn_rate = -728;

		item->pos.z_rot -= 273;

		if (item->pos.z_rot < -2912)
			item->pos.z_rot = -2912;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate > 728)
			lara.turn_rate = 728;

		item->pos.z_rot += 273;

		if (item->pos.z_rot > 2912)
			item->pos.z_rot = 2912;
	}

	if (!(input & IN_JUMP) || item->gravity_status)
	{
		if (input & IN_FORWARD)
		{
			if (input & IN_WALK)
				item->goal_anim_state = AS_WALK;
			else
				item->goal_anim_state = AS_DASH;
		}
		else if (!(input & (IN_LEFT | IN_RIGHT)))
			item->goal_anim_state = AS_STOP;
	}
	else
		item->goal_anim_state = AS_DASHDIVE;
}

void lara_col_dash(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll) || TestLaraVault(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
	{
		item->pos.z_rot = 0;

		if (TestWall(item, 256, 0, -640))
		{
			item->current_anim_state = AS_SPLAT;
			item->anim_number = ANIM_HITWALLLEFT;
			item->frame_number = anims[ANIM_HITWALLLEFT].frame_base;
			return;
		}

		LaraCollideStop(item, coll);
	}

	if (LaraFallen(item, coll))
		return;

	if (coll->mid_floor >= -384 && coll->mid_floor < -128)
	{
		if (item->frame_number >= 3 && item->frame_number <= 14)
		{
			item->anim_number = ANIM_RUNSTEPUP_LEFT;
			item->frame_number = anims[ANIM_RUNSTEPUP_LEFT].frame_base;
		}
		else
		{
			item->anim_number = ANIM_RUNSTEPUP_RIGHT;
			item->frame_number = anims[ANIM_RUNSTEPUP_RIGHT].frame_base;
		}
	}

	if (TestLaraSlide(item, coll))
		return;

	if (coll->mid_floor < 50)
	{
		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
	else
		item->pos.y_pos += 50;
}

void GetLaraCollisionInfo(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->facing = lara.move_angle;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 762);
}

void lara_as_dashdive(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->goal_anim_state != AS_DEATH && item->goal_anim_state != AS_STOP && item->goal_anim_state != AS_RUN && item->fallspeed > 131)
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_dashdive(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->speed < 0)
		lara.move_angle = item->pos.y_rot - 32768;
	else
		lara.move_angle = item->pos.y_rot;

	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -256;
	coll->bad_ceiling = 192;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);
	LaraDeflectEdgeJump(item, coll);

	if (LaraFallen(item, coll))
		return;

	if (item->speed < 0)
		lara.move_angle = item->pos.y_rot;

	if (coll->mid_floor <= 0 && item->fallspeed > 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else if (lara.water_status == LW_WADE || !(input & IN_FORWARD) || input & IN_WALK)
			item->goal_anim_state = AS_STOP;
		else
			item->goal_anim_state = AS_RUN;

		item->gravity_status = 0;
		item->fallspeed = 0;
		item->pos.y_pos += coll->mid_floor;
		item->speed = 0;
		AnimateLara(item);
	}

	ShiftItem(item, coll);

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

long LaraLandedBad(ITEM_INFO* item, COLL_INFO* coll)
{
	long land_speed;

	land_speed = item->fallspeed - 140;

	if (land_speed <= 0)
		return 0;

	if (land_speed <= 14)
		item->hit_points += short(-1000 * SQUARE(land_speed) / 196);
	else
		item->hit_points = -1;

	return item->hit_points <= 0;
}

void MonkeySwingFall(ITEM_INFO* item)
{
	item->goal_anim_state = AS_UPJUMP;
	item->current_anim_state = AS_UPJUMP;
	item->anim_number = ANIM_STOPHANG;
	item->frame_number = anims[ANIM_STOPHANG].frame_base + 9;
	item->speed = 2;
	item->gravity_status = 1;
	item->fallspeed = 1;
	item->pos.y_pos += 256;
	lara.gun_status = LG_NO_ARMS;
}

void MonkeySwingSnap(ITEM_INFO* item, COLL_INFO* coll)
{
	short room_num;

	room_num = item->room_number;
	item->pos.y_pos = GetCeiling(GetFloor(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, &room_num),
		item->pos.x_pos, item->pos.y_pos, item->pos.z_pos) + 704;
}

void lara_as_hang2(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	lara.torso_x_rot = 0;
	lara.torso_y_rot = 0;

	if (lara.CanMonkeySwing)
	{
		if (!(input & IN_ACTION) || item->hit_points <= 0)
		{
			MonkeySwingFall(item);
			return;
		}

		camera.target_angle = 0;
		camera.target_elevation = -10920;
	}

	if (input & IN_LOOK)
		LookUpDown();
}

void lara_col_hang2(ITEM_INFO* item, COLL_INFO* coll)
{
	item->fallspeed = 0;
	item->gravity_status = 0;

	if (lara.CanMonkeySwing)
	{
		coll->bad_pos = -NO_HEIGHT;
		coll->bad_neg = NO_HEIGHT;
		coll->bad_ceiling = 0;
		coll->slopes_are_walls = 0;
		lara.move_angle = item->pos.y_rot;
		coll->facing = lara.move_angle;
		coll->radius = 100;
		GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 600);

		if (input & IN_FORWARD && coll->coll_type != CT_FRONT && ABS(coll->mid_ceiling - coll->front_ceiling) < 50)
			item->goal_anim_state = AS_MONKEYSWING;
		else if (input & IN_LSTEP && TestMonkeyLeft(item, coll))
			item->goal_anim_state = AS_MONKEYL;
		else if (input & IN_RSTEP && TestMonkeyRight(item, coll))
			item->goal_anim_state = AS_MONKEYR;
		else if (input & IN_LEFT)
			item->goal_anim_state = AS_HANGTURNL;
		else if (input & IN_RIGHT)
			item->goal_anim_state = AS_HANGTURNR;

		MonkeySwingSnap(item, coll);
	}
	else
	{
		LaraHangTest(item, coll);

		if (item->goal_anim_state == AS_HANG2)
		{
			if (input & IN_FORWARD && coll->front_floor > -850 && coll->front_floor < -650 && coll->front_floor - coll->front_ceiling >= 0 &&
				coll->left_floor2 - coll->left_ceiling2 >= 0 && coll->right_floor2 - coll->right_ceiling2 >= 0 && !coll->hit_static)
			{
				if (input & IN_WALK)
					item->goal_anim_state = AS_GYMNAST;
				else
					item->goal_anim_state = AS_NULL;
			}
#ifdef GENERAL_FIXES
			else if (input & (IN_FORWARD) &&	//fuck off
#else
			else if (input & (IN_FORWARD | IN_DUCK) &&
#endif
				coll->front_floor > -850 && coll->front_floor < -650 &&
				coll->front_floor - coll->front_ceiling >= 0 && coll->left_floor2 - coll->left_ceiling2 >= 0 &&
				coll->right_floor2 - coll->right_ceiling2 >= 0 && !coll->hit_static)
			{
				item->goal_anim_state = AS_HANG2DUCK;
				item->required_anim_state = AS_DUCK;
			}
			else if (input & IN_LEFT || input & IN_LSTEP)
				item->goal_anim_state = AS_HANGLEFT;
			else if (input & IN_RIGHT || input & IN_RSTEP)
				item->goal_anim_state = AS_HANGRIGHT;
		}
	}
}

void lara_as_monkeyswing(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_HANG2;
		return;
	}

	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;
	lara.torso_x_rot = 0;
	lara.torso_y_rot = 0;

	if (input & IN_LOOK)
		LookUpDown();

	if (input & IN_FORWARD)
		item->goal_anim_state = AS_MONKEYSWING;
	else
		item->goal_anim_state = AS_HANG2;

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -546)
			lara.turn_rate = -546;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate > 546)
			lara.turn_rate = 546;
	}
}

void lara_col_monkeyswing(ITEM_INFO* item, COLL_INFO* coll)
{
	if (!(input & IN_ACTION) || !lara.CanMonkeySwing)
	{
		MonkeySwingFall(item);
		return;
	}
	
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = NO_HEIGHT;
	coll->bad_ceiling = 0;
	lara.move_angle = item->pos.y_rot;
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;
	coll->facing = lara.move_angle;
	coll->radius = 100;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 600);

	if (coll->coll_type == CT_FRONT || ABS(coll->mid_ceiling - coll->front_ceiling) > 50)
	{
		item->anim_number = ANIM_MONKEYHANG;
		item->frame_number = anims[ANIM_MONKEYHANG].frame_base;
		item->current_anim_state = AS_HANG2;
		item->goal_anim_state = AS_HANG2;
	}
	else
	{
		if (ABS(coll->mid_ceiling - coll->left_ceiling2) > 50)
		{
			ShiftItem(item, coll);
			item->pos.y_rot += 910;
		}
		else if (ABS(coll->mid_ceiling - coll->right_ceiling2) > 50)
		{
			ShiftItem(item, coll);
			item->pos.y_rot -= 910;
		}

		camera.target_elevation = 1820;
		MonkeySwingSnap(item, coll);
	}
}

void lara_as_monkeyl(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_HANG2;
		return;
	}

	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	lara.torso_y_rot = 0;
	lara.torso_x_rot = 0;

	if (input & IN_LSTEP)
		item->goal_anim_state = AS_MONKEYL;
	else
		item->goal_anim_state = AS_HANG2;

	camera.target_elevation = 1820;
}

void lara_col_monkeyl(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION && lara.CanMonkeySwing)
	{
		if (TestMonkeyLeft(item, coll))
			MonkeySwingSnap(item, coll);
		else
		{
			item->anim_number = ANIM_MONKEYHANG;
			item->current_anim_state = AS_HANG2;
			item->goal_anim_state = AS_HANG2;
			item->frame_number = anims[ANIM_MONKEYHANG].frame_base;
		}
	}
	else
		MonkeySwingFall(item);
}

void lara_as_monkeyr(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_HANG2;
		return;
	}

	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	lara.torso_y_rot = 0;
	lara.torso_x_rot = 0;

	if (input & IN_RSTEP)
		item->goal_anim_state = AS_MONKEYR;
	else
		item->goal_anim_state = AS_HANG2;

	camera.target_elevation = 1820;
}

void lara_col_monkeyr(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION && lara.CanMonkeySwing)
	{
		if (TestMonkeyRight(item, coll))
			MonkeySwingSnap(item, coll);
		else
		{
			item->anim_number = ANIM_MONKEYHANG;
			item->current_anim_state = AS_HANG2;
			item->goal_anim_state = AS_HANG2;
			item->frame_number = anims[ANIM_MONKEYHANG].frame_base;
		}
	}
	else
		MonkeySwingFall(item);
}

void lara_as_hangturnl(ITEM_INFO* item, COLL_INFO* coll)
{
	camera.target_elevation = 1820;
	lara.torso_y_rot = 0;
	lara.torso_x_rot = 0;
	item->pos.y_rot -= 273;

	if (item->hit_points <= 0 || !(input & IN_LEFT))
		item->goal_anim_state = AS_HANG2;
}

void lara_as_hangturnr(ITEM_INFO* item, COLL_INFO* coll)
{
	camera.target_elevation = 1820;
	lara.torso_y_rot = 0;
	lara.torso_x_rot = 0;
	item->pos.y_rot += 273;

	if (item->hit_points <= 0 || !(input & IN_RIGHT))
		item->goal_anim_state = AS_HANG2;
}

void lara_col_hangturnlr(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION && lara.CanMonkeySwing)
	{
		coll->bad_pos = -NO_HEIGHT;
		coll->bad_neg = -384;
		coll->bad_ceiling = 0;
		lara.move_angle = item->pos.y_rot;
		coll->facing = item->pos.y_rot;
		coll->radius = 100;
		coll->slopes_are_walls = 1;
		GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 600);
		MonkeySwingSnap(item, coll);
	}
	else
		MonkeySwingFall(item);
}

void lara_as_monkey180(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	item->goal_anim_state = AS_HANG2;
}

void lara_col_monkey180(ITEM_INFO* item, COLL_INFO* coll)
{
	lara_col_monkeyswing(item, coll);
}

void lara_as_hangleft(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = 0;
	camera.target_elevation = -10920;

	if (!(input & (IN_LEFT | IN_LSTEP)))
		item->goal_anim_state = AS_HANG;
}

void lara_col_hangleft(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot - 16384;
	coll->radius = 102;
	LaraHangTest(item, coll);
	lara.move_angle = item->pos.y_rot - 16384;
}

void lara_as_hangright(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = 0;
	camera.target_elevation = -10920;

	if (!(input & (IN_RIGHT | IN_RSTEP)))
		item->goal_anim_state = AS_HANG;
}

void lara_col_hangright(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot + 16384;
	coll->radius = 102;
	LaraHangTest(item, coll);
	lara.move_angle = item->pos.y_rot + 16384;
}

void lara_as_extcornerl(ITEM_INFO* item, COLL_INFO* coll)
{
	short flip;

	camera.target_angle = 0x4000;
	camera.target_elevation = -6144;
	flip = item->anim_number == ANIM_EXTCORNERL_END || item->anim_number == ANIM_EXTCLIMBL_END;
	SetCornerAnim(item, coll, 0x4000, flip);
}

void lara_as_extcornerr(ITEM_INFO* item, COLL_INFO* coll)
{
	short flip;

	camera.target_angle = -0x4000;
	camera.target_elevation = -6144;
	flip = item->anim_number == ANIM_EXTCORNERR_END || item->anim_number == ANIM_EXTCLIMBR_END;
	SetCornerAnim(item, coll, -0x4000, flip);
}

void lara_as_intcornerl(ITEM_INFO* item, COLL_INFO* coll)
{
	short flip;

	camera.target_angle = -0x4000;
	camera.target_elevation = -6144;
	flip = item->anim_number == ANIM_INTCORNERL_END || item->anim_number == ANIM_INTCLIMBL_END;
	SetCornerAnim(item, coll, -0x4000, flip);
}

void lara_as_intcornerr(ITEM_INFO* item, COLL_INFO* coll)
{
	short flip;

	camera.target_angle = 0x4000;
	camera.target_elevation = -6144;
	flip = item->anim_number == ANIM_INTCORNERR_END || item->anim_number == ANIM_INTCLIMBR_END;
	SetCornerAnim(item, coll, 0x4000, flip);
}

void lara_as_splat(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
}

void lara_col_splat(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->slopes_are_walls = 1;
	coll->slopes_are_pits = 1;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	GetLaraCollisionInfo(item, coll);
	ShiftItem(item, coll);

	if (coll->mid_floor >= -256 && coll->mid_floor <= 256)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_compress(ITEM_INFO* item, COLL_INFO* coll)
{
	if (lara.water_status != LW_WADE)
	{
		if (input & IN_FORWARD && LaraFloorFront(item, item->pos.y_rot, 256) >= -384)
		{
			item->goal_anim_state = AS_FORWARDJUMP;
			lara.move_angle = item->pos.y_rot;
		}
		else if (input & IN_LEFT && LaraFloorFront(item, item->pos.y_rot - 16384, 256) >= -384)
		{
			item->goal_anim_state = AS_LEFTJUMP;
			lara.move_angle = item->pos.y_rot - 16384;
		}
		else if (input & IN_RIGHT && LaraFloorFront(item, item->pos.y_rot + 16384, 256) >= -384)
		{
			item->goal_anim_state = AS_RIGHTJUMP;
			lara.move_angle = item->pos.y_rot + 16384;
		}
		else if (input & IN_BACK && LaraFloorFront(item, item->pos.y_rot - 32768, 256) >= -384)
		{
			item->goal_anim_state = AS_BACKJUMP;
			lara.move_angle = item->pos.y_rot + 32768;
		}
	}

	if (item->fallspeed > 131)
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_compress(ITEM_INFO* item, COLL_INFO* coll)
{
	item->fallspeed = 0;
	item->gravity_status = 0;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = NO_HEIGHT;
	coll->bad_ceiling = 0;
	GetLaraCollisionInfo(item, coll);

	if (LaraFallen(item, coll))
		return;

	if (coll->mid_ceiling > -100)
	{
		item->anim_number = ANIM_STOP;
		item->frame_number = anims[ANIM_STOP].frame_base;
		item->goal_anim_state = AS_STOP;
		item->current_anim_state = AS_STOP;
		item->speed = 0;
		item->fallspeed = 0;
		item->gravity_status = 0;
		item->pos.x_pos = coll->old.x;
		item->pos.y_pos = coll->old.y;
		item->pos.z_pos = coll->old.z;
	}

	if (coll->mid_floor > -256 && coll->mid_floor < 256)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_back(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (!lara.IsMoving)
	{
		if (input & IN_BACK && (input & IN_WALK || lara.water_status == LW_WADE))
			item->goal_anim_state = AS_BACK;
		else
			item->goal_anim_state = AS_STOP;

		if (input & IN_LEFT)
		{
			lara.turn_rate -= 409;

			if (lara.turn_rate < -728)
				lara.turn_rate = -728;
		}
		else if (input & IN_RIGHT)
		{
			lara.turn_rate += 409;

			if (lara.turn_rate > 728)
				lara.turn_rate = 728;
		}
	}
}

void lara_col_back(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 0;
	item->fallspeed = 0;
	lara.move_angle = item->pos.y_rot + 32768;
	coll->bad_pos = lara.water_status == LW_WADE ? -NO_HEIGHT : 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_pits = 1;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
		LaraCollideStop(item, coll);

	if (LaraFallen(item, coll))
		return;

	if (coll->mid_floor > 128 && coll->mid_floor < 384)
	{
		if (item->frame_number >= 964 && item->frame_number <= 993)
		{
			item->anim_number = ANIM_BACKSTEPD_RIGHT;
			item->frame_number = anims[ANIM_BACKSTEPD_RIGHT].frame_base;
		}
		else
		{
			item->anim_number = ANIM_BACKSTEPD_LEFT;
			item->frame_number = anims[ANIM_BACKSTEPD_LEFT].frame_base;
		}
	}

	if (TestLaraSlide(item, coll))
		return;

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_null(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;
}

void lara_as_fastturn(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (lara.turn_rate >= 0)
	{
		lara.turn_rate = 1456;

		if (!(input & IN_RIGHT))
			item->goal_anim_state = AS_STOP;
	}
	else
	{
		lara.turn_rate = -1456;

		if (!(input & IN_LEFT))
			item->goal_anim_state = AS_STOP;
	}
}

void lara_as_stepright(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (!lara.IsMoving)
	{
		if (!(input & IN_RSTEP))
			item->goal_anim_state = AS_STOP;

		if (input & IN_LEFT)
		{
			lara.turn_rate -= 409;

			if (lara.turn_rate < -728)
				lara.turn_rate = -728;
		}
		else if (input & IN_RIGHT)
		{
			lara.turn_rate += 409;

			if (lara.turn_rate > 728)
				lara.turn_rate = 728;
		}
	}
}

void lara_col_stepright(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->current_anim_state == AS_STEPRIGHT)
		lara.move_angle = item->pos.y_rot + 16384;
	else
		lara.move_angle = item->pos.y_rot - 16384;

	item->gravity_status = 0;
	item->fallspeed = 0;
	coll->bad_pos = lara.water_status == LW_WADE ? -NO_HEIGHT : 128;
	coll->slopes_are_walls = 1;
	coll->slopes_are_pits = 1;
	coll->bad_neg = -128;
	coll->bad_ceiling = 0;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
		LaraCollideStop(item, coll);

	if (LaraFallen(item, coll))
		return;

	if (TestLaraSlide(item, coll))
		return;

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_stepleft(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (!lara.IsMoving)
	{
		if (!(input & IN_LSTEP))
			item->goal_anim_state = AS_STOP;

		if (input & IN_LEFT)
		{
			lara.turn_rate -= 409;

			if (lara.turn_rate < -728)
				lara.turn_rate = -728;
		}
		else if (input & IN_RIGHT)
		{
			lara.turn_rate += 409;

			if (lara.turn_rate > 728)
				lara.turn_rate = 728;
		}
	}
}

void lara_col_stepleft(ITEM_INFO* item, COLL_INFO* coll)
{
	lara_col_stepright(item, coll);
}

void lara_as_slide(ITEM_INFO* item, COLL_INFO* coll)
{
	camera.target_elevation = -8190;

	if (input & IN_JUMP && !(input & IN_BACK))
		item->goal_anim_state = AS_FORWARDJUMP;
}

void lara_col_slide(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	lara_slide_slope(item, coll);
}

void lara_as_backjump(ITEM_INFO* item, COLL_INFO* coll)
{
	camera.target_angle = 24570;

	if (item->fallspeed <= 131)
	{
		if (item->goal_anim_state == AS_RUN)
			item->goal_anim_state = AS_STOP;
		else if (input & (IN_FORWARD | IN_ROLL) && item->goal_anim_state != AS_STOP)
			item->goal_anim_state = AS_TWIST;
	}
	else
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_backjump(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot + 32768;
	lara_col_jumper(item, coll);
}

void lara_as_rightjump(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;

	if (item->fallspeed <= 131)
	{
		if (input & IN_LEFT && item->goal_anim_state != AS_STOP)
			item->goal_anim_state = AS_TWIST;
	}
	else
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_rightjump(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot + 16384;
	lara_col_jumper(item, coll);
}

void lara_as_leftjump(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;

	if (item->fallspeed <= 131)
	{
		if (input & IN_RIGHT && item->goal_anim_state != AS_STOP)
			item->goal_anim_state = AS_TWIST;
	}
	else
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_leftjump(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot - 16384;
	lara_col_jumper(item, coll);
}

void lara_as_fallback(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->fallspeed > 131)
		item->goal_anim_state = AS_FASTFALL;

	if (input & IN_ACTION && lara.gun_status == LG_NO_ARMS)
		item->goal_anim_state = AS_REACH;
}

void lara_col_fallback(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot - 32768;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);
	LaraDeflectEdgeJump(item, coll);

	if (coll->mid_floor <= 0 && item->fallspeed > 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else
			item->goal_anim_state = AS_STOP;

		item->fallspeed = 0;
		item->gravity_status = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_slideback(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_JUMP && !(input & IN_FORWARD))
		item->goal_anim_state = AS_BACKJUMP;
}

void lara_col_slideback(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot + 32768;
	lara_slide_slope(item, coll);
}

void lara_col_roll(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	item->gravity_status = 0;
	item->fallspeed = 0;
	coll->slopes_are_pits = 0;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (TestLaraSlide(item, coll))
		return;

	if (LaraFallen(item, coll))
		return;

	ShiftItem(item, coll);

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_col_roll2(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 0;
	item->fallspeed = 0;
	lara.move_angle = item->pos.y_rot + 32768;
	coll->slopes_are_walls = 1;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (TestLaraSlide(item, coll))
		return;
	
	if (coll->mid_floor > 200)
	{
		item->fallspeed = 0;
		item->anim_number = ANIM_FALLBACK;
		item->frame_number = anims[ANIM_FALLBACK].frame_base;
		item->current_anim_state = AS_FALLBACK;
		item->goal_anim_state = AS_FALLBACK;
		item->gravity_status = 1;
	}
	else
	{
		ShiftItem(item, coll);

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_pushblock(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.flags = 1;
	camera.target_angle = 16380;
	camera.target_elevation = -4550;
}

void lara_as_pullblock(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.flags = 1;
	camera.target_angle = 6370;
	camera.target_elevation = -4550;
}

void lara_as_ppready(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = 13650;

	if (!(input & IN_ACTION))
		item->goal_anim_state = AS_STOP;
}

void lara_as_pickup(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = -23660;
	camera.target_elevation = -2730;
	camera.target_distance = 1024;
}

void lara_as_pickupflare(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = 23660;
	camera.target_elevation = -2730;
	camera.target_distance = 1024;

	if (item->frame_number == anims[item->anim_number].frame_end - 1)
		lara.gun_status = LG_NO_ARMS;
}

void lara_as_switchon(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = 14560;
	camera.target_elevation = -4550;
	camera.target_distance = 1024;
	camera.speed = 6;
}

void lara_as_usekey(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = -14560;
	camera.target_elevation = -4550;
	camera.target_distance = 1024;
}

void lara_as_usepuzzle(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.target_angle = -14560;
	camera.target_elevation = -4550;
	camera.target_distance = 1024;

	if (item->frame_number == anims[item->anim_number].frame_end && item->item_flags[0])
	{
		item->anim_number = item->item_flags[0];
		item->frame_number = anims[item->anim_number].frame_base;
		item->current_anim_state = AS_CONTROLLED;
	}
}

void lara_as_special(ITEM_INFO* item, COLL_INFO* coll)
{
	camera.flags = 1;
	camera.target_angle = 30940;
	camera.target_elevation = -4550;
}

void lara_as_swandive(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 1;
	coll->enable_spaz = 0;

	if (item->fallspeed > 131 && item->goal_anim_state != AS_DIVE)
		item->goal_anim_state = AS_FASTDIVE;
}

void lara_col_swandive(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);
	LaraDeflectEdgeJump(item, coll);

	if (coll->mid_floor <= 0 && item->fallspeed > 0)
	{
		item->goal_anim_state = AS_STOP;
		item->fallspeed = 0;
		item->gravity_status = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_fastdive(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ROLL && item->goal_anim_state == AS_FASTDIVE)
		item->goal_anim_state = AS_TWIST;

	coll->enable_baddie_push = 1;
	coll->enable_spaz = 0;
	item->speed = 95 * item->speed / 100;
}

void lara_col_fastdive(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);
	LaraDeflectEdgeJump(item, coll);

	if (coll->mid_floor <= 0 && item->fallspeed > 0)
	{
		if (item->fallspeed <= 133)
			item->goal_anim_state = AS_STOP;
		else
			item->goal_anim_state = AS_DEATH;

		item->fallspeed = 0;
		item->gravity_status = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_waterout(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
	camera.flags = 1;
}

void lara_as_wade(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	camera.target_elevation = -4004;

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -1456)
			lara.turn_rate = -1456;

		item->pos.z_rot -= 273;

		if (item->pos.z_rot < -2002)
			item->pos.z_rot = -2002;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate > 1456)
			lara.turn_rate = 1456;

		item->pos.z_rot += 273;

		if (item->pos.z_rot > 2002)
			item->pos.z_rot = 2002;
	}

	if (input & IN_FORWARD)
	{
		if (lara.water_status == LW_ABOVE_WATER)
			item->goal_anim_state = AS_RUN;
		else
			item->goal_anim_state = AS_WADE;
	}
	else
		item->goal_anim_state = AS_STOP;
}

void lara_col_wade(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (TestLaraVault(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
	{
		item->pos.z_rot = 0;

		if ((coll->front_type == WALL || coll->front_type == SPLIT_TRI) && coll->front_floor < -640)
		{
			item->current_anim_state = AS_SPLAT;

			if (item->frame_number >= 0 && item->frame_number <= 9)
			{
				item->anim_number = ANIM_HITWALLLEFT;
				item->frame_number = anims[ANIM_HITWALLLEFT].frame_base;
				return;
			}

			if (item->frame_number >= 10 && item->frame_number <= 21)
			{
				item->anim_number = ANIM_HITWALLRIGHT;
				item->frame_number = anims[ANIM_HITWALLRIGHT].frame_base;
				return;
			}
		}

		LaraCollideStop(item, coll);
	}

	if (coll->mid_floor >= -384 && coll->mid_floor < -128)
	{
		if (item->frame_number >= 3 && item->frame_number <= 14)
		{
			item->anim_number = ANIM_RUNSTEPUP_LEFT;
			item->frame_number = anims[ANIM_RUNSTEPUP_LEFT].frame_base;
		}
		else
		{
			item->anim_number = ANIM_RUNSTEPUP_RIGHT;
			item->frame_number = anims[ANIM_RUNSTEPUP_RIGHT].frame_base;
		}
	}

	if (coll->mid_floor >= 50)
		item->pos.y_pos += 50;
	else if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_default_col(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_pits = 1;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);
}

void lara_as_deathslide(ITEM_INFO* item, COLL_INFO* coll)
{
	short room_number;

	room_number = item->room_number;
	camera.target_angle = 12740;
	GetHeight(GetFloor(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, &room_number), item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	coll->trigger = trigger_index;

	if (!(input & IN_ACTION))
	{
		item->goal_anim_state = AS_FORWARDJUMP;
		AnimateLara(item);
		lara_item->gravity_status = 1;
		lara_item->speed = 100;
		lara_item->fallspeed = 40;
		lara.move_angle = item->pos.y_rot;
	}
}

void lara_as_controlled(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;

	if (item->anim_number == ANIM_PLAYHARP && item->frame_number == anims[ANIM_PLAYHARP].frame_base + 120)
		S_CDPlay(6, 0);

	if (item->anim_number == ANIM_MINEDETECT)
	{
		if (item->frame_number == anims[ANIM_MINEDETECT].frame_base + 16)
			lara.mesh_ptrs[LM_RHAND] = meshes[objects[MESHSWAP3].mesh_index + 2 * LM_RHAND];
		else if (item->frame_number == anims[ANIM_MINEDETECT].frame_base + 118)
			lara.mesh_ptrs[LM_RHAND] = meshes[objects[LARA_SKIN].mesh_index + 2 * LM_RHAND];
	}

	if (item->frame_number == anims[item->anim_number].frame_end - 1)
	{
		if (item->anim_number == ANIM_PLAYHARP)
			S_CDPlay(19, 0);

		lara.gun_status = LG_NO_ARMS;

		if (UseForcedFixedCamera)
			UseForcedFixedCamera = 0;
	}
}

void lara_as_controlledl(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;
}

void lara_col_turnswitch(ITEM_INFO* item, COLL_INFO* coll)
{
	if (coll->old.x != item->pos.x_pos || coll->old.z != item->pos.z_pos)
	{
		if (item->anim_number == ANIM_TURNSWITCHAC)
		{
			item->pos.y_rot -= 0x4000;
			item->anim_number = ANIM_TURNSWITCHAD;
			item->frame_number = anims[ANIM_TURNSWITCHAD].frame_base;
		}

		if (item->anim_number == ANIM_TURNSWITCHCC)
		{
			item->pos.y_rot += 0x4000;
			item->anim_number = ANIM_TURNSWITCHCD;
			item->frame_number = anims[ANIM_TURNSWITCHCD].frame_base;
		}
	}
}

void lara_as_pulley(ITEM_INFO* item, COLL_INFO* coll)
{
	ITEM_INFO* p;

	p = (ITEM_INFO*)lara.GeneralPtr;
	lara.look = 0;
	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;

	if (input & IN_ACTION && p->trigger_flags)
		item->goal_anim_state = AS_PULLEY;
	else
		item->goal_anim_state = AS_STOP;

	if (item->anim_number == ANIM_PULLEY && item->frame_number == anims[ANIM_PULLEY].frame_base + 44)
	{
		if (p->trigger_flags)
		{
			if (!p->item_flags[1])
			{
				p->trigger_flags--;

				if (!p->trigger_flags)
				{
					p->status = ITEM_DEACTIVATED;
					p->item_flags[2] = 1;

					if (p->item_flags[3] >= 0)
						p->trigger_flags = ABS(p->item_flags[3]);
					else
						p->item_flags[0] = 1;
				}
				else
				{
					if (p->item_flags[2])
					{
						p->item_flags[2] = 0;
						p->status = ITEM_DEACTIVATED;
					}
				}
			}
		}
	}

	if (item->anim_number == ANIM_PULLEY2STAT && item->frame_number == anims[ANIM_PULLEY2STAT].frame_end - 1)
		lara.gun_status = LG_NO_ARMS;
}

void lara_as_walk(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (lara.IsMoving)
		return;

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -728)
			lara.turn_rate = -728;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate > 728)
			lara.turn_rate = 728;
	}

	if (input & IN_FORWARD)
	{
		if (lara.water_status == LW_WADE)
			item->goal_anim_state = AS_WADE;
		else if (input & IN_WALK)
			item->goal_anim_state = AS_WALK;
		else
			item->goal_anim_state = AS_RUN;
	}
	else
		item->goal_anim_state = AS_STOP;
}

void lara_col_walk(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 0;
	item->fallspeed = 0;
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_walls = 1;
	coll->slopes_are_pits = 1;
	coll->lava_is_pit = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (TestLaraVault(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
	{
		if (item->frame_number >= 29 && item->frame_number <= 47)
		{
			item->anim_number = ANIM_WALK_STOP_RIGHT;
			item->frame_number = anims[ANIM_WALK_STOP_RIGHT].frame_base;
		}
		else
		{
			if (item->frame_number >= 22 && item->frame_number <= 28 ||
				item->frame_number >= 48 && item->frame_number <= 57)
			{
				item->anim_number = ANIM_WALK_STOP_LEFT;
				item->frame_number = anims[ANIM_WALK_STOP_LEFT].frame_base;
			}
			else
				LaraCollideStop(item, coll);
		}
	}

	if (LaraFallen(item, coll))
		return;

	if (coll->mid_floor > 128)
	{
		if (coll->front_floor == NO_HEIGHT || coll->front_floor <= 128)
			coll->mid_floor = 0;
		else
		{
			if (item->frame_number >= 28 && item->frame_number <= 45)
			{
				item->anim_number = ANIM_WALKSTEPD_RIGHT;
				item->frame_number = anims[ANIM_WALKSTEPD_RIGHT].frame_base;
			}
			else
			{
				item->anim_number = ANIM_WALKSTEPD_LEFT;
				item->frame_number = anims[ANIM_WALKSTEPD_LEFT].frame_base;
			}
		}
	}

	if (coll->mid_floor >= -384 && coll->mid_floor < -128)
	{
		if (coll->front_floor == NO_HEIGHT || coll->front_floor < -384 || coll->front_floor >= -128)
			coll->mid_floor = 0;
		else
		{
			if (item->frame_number >= 27 && item->frame_number <= 44)
			{
				item->anim_number = ANIM_WALKSTEPUP_RIGHT;
				item->frame_number = anims[ANIM_WALKSTEPUP_RIGHT].frame_base;
			}
			else
			{
				item->anim_number = ANIM_WALKSTEPUP_LEFT;
				item->frame_number = anims[ANIM_WALKSTEPUP_LEFT].frame_base;
			}
		}
	}

	if (TestLaraSlide(item, coll))
		return;

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_run(ITEM_INFO* item, COLL_INFO* coll)
{
	static long jump_ok = 1;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_DEATH;
		return;
	}

	if (input & IN_ROLL)
	{
		item->anim_number = ANIM_ROLL;
		item->frame_number = anims[ANIM_ROLL].frame_base + 2;
		item->current_anim_state = AS_ROLL;
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (input & IN_SPRINT && DashTimer)
	{
		item->goal_anim_state = AS_DASH;
		return;
	}

	if (input & IN_DUCK && lara.water_status != LW_WADE &&
		(lara.gun_status == LG_NO_ARMS || lara.gun_type == WEAPON_NONE || lara.gun_type == WEAPON_PISTOLS ||
			lara.gun_type == WEAPON_REVOLVER || lara.gun_type == WEAPON_UZI || lara.gun_type == WEAPON_FLARE))
	{
		item->goal_anim_state = AS_DUCK;
		return;
	}

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -1456)
			lara.turn_rate = -1456;

		item->pos.z_rot -= 273;

		if (item->pos.z_rot < -2002)
			item->pos.z_rot = -2002;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate > 1456)
			lara.turn_rate = 1456;

		item->pos.z_rot += 273;

		if (item->pos.z_rot > 2002)
			item->pos.z_rot = 2002;
	}

	if (item->anim_number == ANIM_STARTRUN)
		jump_ok = 0;
	else if (item->anim_number != ANIM_RUN || item->frame_number == 4)
		jump_ok = 1;

	if (input & IN_JUMP && jump_ok && !item->gravity_status)
		item->goal_anim_state = AS_FORWARDJUMP;
	else if (input & IN_FORWARD)
	{
		if (lara.water_status == LW_WADE)
			item->goal_anim_state = AS_WADE;
		else if (input & IN_WALK)
			item->goal_anim_state = AS_WALK;
		else
			item->goal_anim_state = AS_RUN;
	}
	else
		item->goal_anim_state = AS_STOP;
}

void lara_col_run(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (TestLaraVault(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
	{
		item->pos.z_rot = 0;

		if (item->anim_number != ANIM_STARTRUN && TestWall(item, 256, 0, -640))
		{
			item->current_anim_state = AS_SPLAT;

			if (item->frame_number >= 0 && item->frame_number <= 9)
			{
				item->anim_number = ANIM_HITWALLLEFT;
				item->frame_number = anims[ANIM_HITWALLLEFT].frame_base;
				return;
			}

			if (item->frame_number >= 10 && item->frame_number <= 21)
			{
				item->anim_number = ANIM_HITWALLRIGHT;
				item->frame_number = anims[ANIM_HITWALLRIGHT].frame_base;
				return;
			}
		}

		LaraCollideStop(item, coll);
	}

	if (LaraFallen(item, coll))
		return;

	if (coll->mid_floor >= -384 && coll->mid_floor < -128)
	{
		if (coll->front_floor == NO_HEIGHT || coll->front_floor < -384 || coll->front_floor >= -128)
			coll->mid_floor = 0;
		else
		{
			if (item->frame_number >= 3 && item->frame_number <= 14)
			{
				item->anim_number = ANIM_RUNSTEPUP_LEFT;
				item->frame_number = anims[ANIM_RUNSTEPUP_LEFT].frame_base;
			}
			else
			{
				item->anim_number = ANIM_RUNSTEPUP_RIGHT;
				item->frame_number = anims[ANIM_RUNSTEPUP_RIGHT].frame_base;
			}
		}
	}

	if (TestLaraSlide(item, coll))
		return;

	if (coll->mid_floor < 50)
	{
		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
	else
		item->pos.y_pos += 50;
}

void lara_as_fastback(ITEM_INFO* item, COLL_INFO* coll)
{
	item->goal_anim_state = AS_STOP;

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -1092)
			lara.turn_rate = -1092;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate > 1092)
			lara.turn_rate = 1092;
	}
}

void lara_col_fastback(ITEM_INFO* item, COLL_INFO* coll)
{
	item->fallspeed = 0;
	item->gravity_status = 0;
	lara.move_angle = item->pos.y_rot + 32768;
	coll->slopes_are_walls = 0;
	coll->slopes_are_pits = 1;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (coll->mid_floor > 200)
	{
		item->current_anim_state = AS_FALLBACK;
		item->goal_anim_state = AS_FALLBACK;
		item->anim_number = ANIM_FALLBACK;
		item->frame_number = anims[ANIM_FALLBACK].frame_base;
		item->fallspeed = 0;
		item->gravity_status = 1;
		return;
	}

	if (TestLaraSlide(item, coll))
		return;

	if (LaraDeflectEdge(item, coll))
		LaraCollideStop(item, coll);

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_turn_r(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	lara.turn_rate += 409;

	if (lara.gun_status != LG_READY || lara.water_status == LW_WADE)
	{
		if (lara.turn_rate > 728)
		{
			if (input & IN_WALK || lara.water_status == LW_WADE)
				lara.turn_rate = 728;
			else
				item->goal_anim_state = AS_FASTTURN;
		}
	}
	else
		item->goal_anim_state = AS_FASTTURN;

	if (!(input & IN_FORWARD))
	{
		if (!(input & IN_RIGHT))
			item->goal_anim_state = AS_STOP;

		return;
	}

	if (lara.water_status == LW_WADE)
		item->goal_anim_state = AS_WADE;
	else if (input & IN_WALK)
		item->goal_anim_state = AS_WALK;
	else
		item->goal_anim_state = AS_RUN;
}

void lara_col_turn_r(ITEM_INFO* item, COLL_INFO* coll)
{
	item->fallspeed = 0;
	item->gravity_status = 0;
	lara.move_angle = item->pos.y_rot;
	coll->slopes_are_walls = 1;
	coll->slopes_are_pits = 1;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	GetLaraCollisionInfo(item, coll);

	if (coll->mid_floor <= 100)
	{
		if (TestLaraSlide(item, coll))
			return;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
	else
	{
		item->fallspeed = 0;
		item->anim_number = ANIM_FALLDOWN;
		item->frame_number = anims[ANIM_FALLDOWN].frame_base;
		item->current_anim_state = AS_FORWARDJUMP;
		item->goal_anim_state = AS_FORWARDJUMP;
		item->gravity_status = 1;
	}
}

void lara_as_turn_l(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	lara.turn_rate -= 409;

	if (lara.gun_status != LG_READY || lara.water_status == LW_WADE)
	{
		if (lara.turn_rate < -728)
		{
			if (input & IN_WALK || lara.water_status == LW_WADE)
				lara.turn_rate = -728;
			else
				item->goal_anim_state = AS_FASTTURN;
		}
	}
	else
		item->goal_anim_state = AS_FASTTURN;

	if (!(input & IN_FORWARD))
	{
		if (!(input & IN_LEFT))
			item->goal_anim_state = AS_STOP;

		return;
	}

	if (lara.water_status == LW_WADE)
		item->goal_anim_state = AS_WADE;
	else if (input & IN_WALK)
		item->goal_anim_state = AS_WALK;
	else
		item->goal_anim_state = AS_RUN;
}

void lara_col_turn_l(ITEM_INFO* item, COLL_INFO* coll)
{
	lara_col_turn_r(item, coll);
}

void lara_as_death(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.look = 0;
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;

	if (BinocularRange)
	{
		BinocularRange = 0;
		LaserSight = 0;
		AlterFOV(14560);
		lara_item->mesh_bits = -1;
		lara.Busy = 0;
	}
}

void lara_col_death(ITEM_INFO* item, COLL_INFO* coll)
{
	StopSoundEffect(30);
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	coll->radius = 400;
	GetLaraCollisionInfo(item, coll);
	ShiftItem(item, coll);
	item->hit_points = -1;
	lara.air = -1;

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_as_fastfall(ITEM_INFO* item, COLL_INFO* coll)
{
	item->speed = 95 * item->speed / 100;

	if (item->fallspeed == 154)
		SoundEffect(30, &item->pos, 0);
}

void lara_col_fastfall(ITEM_INFO* item, COLL_INFO* coll)
{
	item->gravity_status = 1;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);
	LaraSlideEdgeJump(item, coll);

	if (coll->mid_floor <= 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else
		{
			item->goal_anim_state = AS_STOP;
			item->current_anim_state = AS_STOP;
			item->anim_number = ANIM_LANDFAR;
			item->frame_number = anims[ANIM_LANDFAR].frame_base;
		}

		StopSoundEffect(30);
		item->fallspeed = 0;
		item->gravity_status = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_stop(ITEM_INFO* item, COLL_INFO* coll)
{
	short height, ceiling, fheight, rheight;

	fheight = NO_HEIGHT;
	rheight = NO_HEIGHT;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_DEATH;
		return;
	}

	if (item->anim_number != 226 && item->anim_number != 228)
		StopSoundEffect(3);

	if (UseInventoryItems(item))
		return;

	if (input & IN_ROLL && lara.water_status != LW_WADE)
	{
		item->anim_number = ANIM_ROLL;
		item->frame_number = anims[ANIM_ROLL].frame_base + 2;
		item->current_anim_state = AS_ROLL;
		item->goal_anim_state = AS_STOP;
		return;
	}

	if (input & IN_DUCK && lara.water_status != LW_WADE && item->current_anim_state == AS_STOP &&
		(lara.gun_status == LG_NO_ARMS || lara.gun_type == WEAPON_NONE || lara.gun_type == WEAPON_PISTOLS ||
			lara.gun_type == WEAPON_REVOLVER || lara.gun_type == WEAPON_UZI || lara.gun_type == WEAPON_FLARE))
	{
		item->goal_anim_state = AS_DUCK;
		return;
	}

	item->goal_anim_state = AS_STOP;

	if (input & IN_LOOK)
		LookUpDown();

	if (input & IN_FORWARD)
		fheight = LaraFloorFront(item, item->pos.y_rot, 104);
	else if (input & IN_BACK)
		rheight = LaraFloorFront(item, item->pos.y_rot + 32768, 104);

	if (input & IN_LSTEP)
	{
		height = LaraFloorFront(item, item->pos.y_rot - 16384, 116);
		ceiling = LaraCeilingFront(item, item->pos.y_rot - 16384, 116, 762);

		if (height < 128 && height > -128 && height_type != BIG_SLOPE && ceiling <= 0)
			item->goal_anim_state = AS_STEPLEFT;
	}
	else if (input & IN_RSTEP)
	{
		height = LaraFloorFront(item, item->pos.y_rot + 16384, 116);
		ceiling = LaraCeilingFront(item, item->pos.y_rot + 16384, 116, 762);

		if (height < 128 && height > -128 && height_type != BIG_SLOPE && ceiling <= 0)
			item->goal_anim_state = AS_STEPRIGHT;
	}
	else if (input & IN_LEFT)
		item->goal_anim_state = AS_TURN_L;
	else if (input & IN_RIGHT)
		item->goal_anim_state = AS_TURN_R;

	if (lara.water_status == LW_WADE)
	{
		if (input & IN_JUMP)
			item->goal_anim_state = AS_COMPRESS;

		if (input & IN_FORWARD)
		{
			if (fheight >= 383 || fheight <= -383)
			{
				lara.move_angle = item->pos.y_rot;
				coll->bad_pos = -NO_HEIGHT;
				coll->bad_neg = -384;
				coll->bad_ceiling = 0;
				coll->radius = 102;
				coll->slopes_are_walls = 1;
				GetLaraCollisionInfo(item, coll);

				if (!TestLaraVault(item, coll))
					coll->radius = 100;
			}
			else
				lara_as_wade(item, coll);
		}
		else if (input & IN_BACK && rheight < 383 && rheight > -383)
			lara_as_back(item, coll);
	}
	else if (input & IN_JUMP)
		item->goal_anim_state = AS_COMPRESS;
	else if (input & IN_FORWARD)
	{
		ceiling = LaraCeilingFront(item, item->pos.y_rot, 104, 762);
		height = LaraFloorFront(item, item->pos.y_rot, 104);

		if ((height_type == BIG_SLOPE || height_type == DIAGONAL) && height < 0 || ceiling > 0)
		{
			item->goal_anim_state = AS_STOP;
			return;
		}

		if (height >= -256 || fheight >= -256)
		{
			if (input & IN_WALK)
				lara_as_walk(item, coll);
			else
				lara_as_run(item, coll);
		}
		else
		{
			lara.move_angle = item->pos.y_rot;
			coll->bad_pos = -NO_HEIGHT;
			coll->bad_neg = -384;
			coll->bad_ceiling = 0;
			coll->radius = 102;
			coll->slopes_are_walls = 1;
			GetLaraCollisionInfo(item, coll);

			if (!TestLaraVault(item, coll))
			{
				coll->radius = 100;
				item->goal_anim_state = AS_STOP;
			}
		}
	}
	else if (input & IN_BACK)
	{
		if (input & IN_WALK)
		{
			if (rheight < 383 && rheight > -383 && height_type != BIG_SLOPE)
				lara_as_back(item, coll);
		}
		else if (rheight > -383)
			item->goal_anim_state = AS_FASTBACK;
	}
}

void lara_col_stop(ITEM_INFO* item, COLL_INFO* coll)
{
	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = 384;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	item->gravity_status = 0;
	item->fallspeed = 0;
	coll->slopes_are_pits = 1;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (LaraHitCeiling(item, coll))
		return;

	if (LaraFallen(item, coll))
		return;

	if (TestLaraSlide(item, coll))
		return;

	ShiftItem(item, coll);

	if (coll->mid_floor != NO_HEIGHT)
		item->pos.y_pos += coll->mid_floor;
}

void lara_col_pose(ITEM_INFO* item, COLL_INFO* coll)
{
	lara_col_stop(item, coll);
}

void lara_as_upjump(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->fallspeed > 131)
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_upjump(ITEM_INFO* item, COLL_INFO* coll)
{
	short* bounds;
	long edge, edge_catch;
	short angle;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_STOP;
		return;
	}

	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	coll->hit_ceiling = 0;
	coll->facing = item->speed >= 0 ? lara.move_angle : lara.move_angle - 32768;
	GetCollisionInfo(coll, item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, item->room_number, 870);

	if (input & IN_ACTION && lara.gun_status == LG_NO_ARMS && !coll->hit_static)
	{
		if (lara.CanMonkeySwing && coll->coll_type == CT_TOP)
		{
			item->goal_anim_state = AS_HANG2;
			item->current_anim_state = AS_HANG2;
			item->anim_number = ANIM_UPJUMPGRAB;
			item->frame_number = anims[ANIM_UPJUMPGRAB].frame_base;
			item->gravity_status = 0;
			item->speed = 0;
			item->fallspeed = 0;
			lara.gun_status = LG_HANDS_BUSY;
			MonkeySwingSnap(item, coll);
			return;
		}

		if (coll->coll_type == CT_FRONT && coll->mid_ceiling <= -384)
		{
			edge_catch = LaraTestEdgeCatch(item, coll, &edge);

			if (edge_catch && (edge_catch >= 0 || LaraTestHangOnClimbWall(item, coll)))
			{
				angle = item->pos.y_rot;

				if (angle >= -6370 && angle <= 6370)
					angle = 0;
				else if (angle >= 10014 && angle <= 22754)
					angle = 16384;
				else if (angle >= 26397 || angle <= -26397)
					angle = -32768;
				else if (angle >= -22754 && angle <= -10014)
					angle = -16384;

				if (!(angle & 0x3FFF))
				{
					if (TestHangSwingIn(item, angle))
					{
						item->anim_number = ANIM_UPJUMPGRAB;
						item->frame_number = anims[ANIM_UPJUMPGRAB].frame_base;
						item->goal_anim_state = AS_HANG2;
						item->current_anim_state = AS_HANG2;
					}
					else
					{
						item->anim_number = ANIM_GRABLEDGE;
						item->frame_number = anims[ANIM_GRABLEDGE].frame_base + 12;
						item->goal_anim_state = AS_HANG;
						item->current_anim_state = AS_HANG;
					}

					bounds = GetBoundsAccurate(item);

					if (edge_catch <= 0)
						item->pos.y_pos = edge - bounds[2] + 4;
					else
						item->pos.y_pos += coll->front_floor - bounds[2];

					item->pos.x_pos += coll->shift.x;
					item->pos.z_pos += coll->shift.z;
					item->pos.y_rot = angle;
					item->gravity_status = 0;
					item->speed = 0;
					item->fallspeed = 0;
					lara.gun_status = LG_HANDS_BUSY;
					lara.torso_y_rot = 0;
					lara.torso_x_rot = 0;
					return;
				}
			}
		}
	}

	ShiftItem(item, coll);

	if (coll->coll_type == CT_CLAMP || coll->coll_type == CT_TOP || coll->coll_type == CT_TOP_FRONT || coll->hit_ceiling)
		item->fallspeed = 1;

	if (coll->coll_type == CT_NONE)
	{
		if (item->fallspeed < -70)
		{
			if (input & IN_FORWARD && item->speed < 5)
				item->speed++;
			else if (input & IN_BACK && item->speed > -5)
				item->speed -= 2;
		}
	}
	else
		item->speed = item->speed <= 0 ? -2 : 2;

	if (item->fallspeed > 0 && coll->mid_floor <= 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else
			item->goal_anim_state = AS_STOP;

		item->gravity_status = 0;
		item->fallspeed = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void lara_as_forwardjump(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->goal_anim_state == AS_SWANDIVE || item->goal_anim_state == AS_REACH)
		item->goal_anim_state = AS_FORWARDJUMP;

	if (item->goal_anim_state != AS_DEATH && item->goal_anim_state != AS_STOP && item->goal_anim_state != AS_RUN)
	{
		if (input & IN_ACTION && lara.gun_status == LG_NO_ARMS)
			item->goal_anim_state = AS_REACH;

		if (input & (IN_ROLL | IN_BACK))
			item->goal_anim_state = AS_TWIST;

		if (input & IN_WALK && lara.gun_status == LG_NO_ARMS)
			item->goal_anim_state = AS_SWANDIVE;

		if (item->fallspeed > 131)
			item->goal_anim_state = AS_FASTFALL;
	}

	if (input & IN_LEFT)
	{
		lara.turn_rate -= 409;

		if (lara.turn_rate < -546)
			lara.turn_rate = -546;
	}
	else if (input & IN_RIGHT)
	{
		lara.turn_rate += 409;

		if (lara.turn_rate >= 546)
			lara.turn_rate = 546;
	}
}

void lara_col_forwardjump(ITEM_INFO* item, COLL_INFO* coll)
{
	if (item->speed < 0)
		lara.move_angle = item->pos.y_rot + 32768;
	else
		lara.move_angle = item->pos.y_rot;

	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);
	LaraDeflectEdgeJump(item, coll);

	if (item->speed < 0)
		lara.move_angle = item->pos.y_rot;

	if (coll->mid_floor <= 0 && item->fallspeed > 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else
		{
			if (lara.water_status == LW_WADE)
				item->goal_anim_state = AS_STOP;
			else
			{
				if (input & IN_FORWARD && !(input & IN_WALK))
					item->goal_anim_state = AS_RUN;
				else
					item->goal_anim_state = AS_STOP;
			}
		}

		item->gravity_status = 0;
		item->fallspeed = 0;
		item->speed = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;

		AnimateLara(item);
	}
}

void lara_as_reach(ITEM_INFO* item, COLL_INFO* coll)
{
	camera.target_angle = 15470;

	if (item->fallspeed > 131)
		item->goal_anim_state = AS_FASTFALL;
}

void lara_col_reach(ITEM_INFO* item, COLL_INFO* coll)
{
	short* bounds;
	long edge, edge_catch;
	short angle;

	if (lara.RopePtr == -1)
		item->gravity_status = 1;

	lara.move_angle = item->pos.y_rot;
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = 0;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);

	if (input & IN_ACTION && lara.gun_status == LG_NO_ARMS && !coll->hit_static)
	{
		if (lara.CanMonkeySwing && coll->coll_type == CT_TOP)
		{
			lara.head_y_rot = 0;
			lara.head_x_rot = 0;
			lara.torso_y_rot = 0;
			lara.torso_x_rot = 0;
			item->anim_number = ANIM_HANG2STOP;
			item->frame_number = anims[ANIM_HANG2STOP].frame_base;
			item->goal_anim_state = AS_HANG2;
			item->current_anim_state = AS_HANG2;
			item->gravity_status = 0;
			item->speed = 0;
			item->fallspeed = 0;
			lara.gun_status = LG_HANDS_BUSY;
			return;
		}

		if (coll->mid_ceiling <= -384 && coll->mid_floor >= 200 && coll->coll_type == CT_FRONT)
		{
			edge_catch = LaraTestEdgeCatch(item, coll, &edge);

			if (edge_catch && (edge_catch >= 0 || LaraTestHangOnClimbWall(item, coll)))
			{
				angle = item->pos.y_rot;

				if (angle >= -6370 && angle <= 6370)
					angle = 0;
				else if (angle >= 10014 && angle <= 22754)
					angle = 16384;
				else if (angle >= 26397 || angle <= -26397)
					angle = -32768;
				else if (angle >= -22754 && angle <= -10014)
					angle = -16384;

				if (!(angle & 0x3FFF))
				{
					if (TestHangSwingIn(item, angle))
					{
						lara.head_y_rot = 0;
						lara.head_x_rot = 0;
						lara.torso_y_rot = 0;
						lara.torso_x_rot = 0;
						item->anim_number = ANIM_HANG2STOP;
						item->frame_number = anims[ANIM_HANG2STOP].frame_base;
						item->goal_anim_state = AS_HANG2;
						item->current_anim_state = AS_HANG2;
					}
					else
					{
						item->anim_number = ANIM_GRABLEDGE;
						item->frame_number = anims[ANIM_GRABLEDGE].frame_base;
						item->goal_anim_state = AS_HANG;
						item->current_anim_state = AS_HANG;
					}

					bounds = GetBoundsAccurate(item);

					if (edge_catch <= 0)
						item->pos.y_pos = edge - bounds[2] - 22;
					else
					{
						item->pos.y_pos += coll->front_floor - bounds[2];

						switch ((ushort)(item->pos.y_rot + 0x2000) >> 14)
						{
						case NORTH:
							item->pos.z_pos = (item->pos.z_pos | 0x3FF) - 100;
							item->pos.x_pos += coll->shift.x;
							break;

						case EAST:
							item->pos.x_pos = (item->pos.x_pos | 0x3FF) - 100;
							item->pos.z_pos += coll->shift.z;
							break;

						case SOUTH:
							item->pos.z_pos = (item->pos.z_pos & ~0x3FF) + 100;
							item->pos.x_pos += coll->shift.x;
							break;

						case WEST:
							item->pos.x_pos = (item->pos.x_pos & ~0x3FF) + 100;
							item->pos.z_pos += coll->shift.z;
							break;
						}
					}
						
					item->pos.y_rot = angle;
					item->gravity_status = 1;
					item->speed = 2;
					item->fallspeed = 1;
					lara.gun_status = LG_HANDS_BUSY;
					return;
				}
			}
		}
	}

	LaraSlideEdgeJump(item, coll);
	GetLaraCollisionInfo(item, coll);
	ShiftItem(item, coll);

	if (item->fallspeed > 0 && coll->mid_floor <= 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else
		{
			item->gravity_status = 0;
			item->goal_anim_state = AS_STOP;
			item->fallspeed = 0;

			if (coll->mid_floor != NO_HEIGHT)
				item->pos.y_pos += coll->mid_floor;
		}
	}
}

void lara_col_polestat(ITEM_INFO* item, COLL_INFO* coll)
{
	short room_num;

	if (item->hit_points <= 0)
	{
		item->goal_anim_state = AS_FASTFALL;
		return;
	}

	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;

	if (item->anim_number != ANIM_POLESTAT)
		return;

	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	lara.move_angle = item->pos.y_rot;
	coll->facing = lara.move_angle;
	coll->radius = 100;
	coll->slopes_are_walls = 1;
	GetLaraCollisionInfo(item, coll);

	if (input & IN_ACTION)
	{
		item->goal_anim_state = AS_POLESTAT;

		if (input & IN_LEFT)
			item->goal_anim_state = AS_POLELEFT;
		else if (input & IN_RIGHT)
			item->goal_anim_state = AS_POLERIGHT;

		if (input & IN_LOOK)
			LookUpDown();

		if (input & IN_FORWARD)
		{
			room_num = item->room_number;

			if (item->pos.y_pos - GetCeiling(GetFloor(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, &room_num),
				item->pos.x_pos, item->pos.y_pos, item->pos.z_pos) > 1024)
				item->goal_anim_state = AS_POLEUP;
		}
		else if (input & IN_BACK && coll->mid_floor > 0)
		{
			item->goal_anim_state = AS_POLEDOWN;
			item->item_flags[2] = 0;
		}

		if (input & IN_JUMP)
			item->goal_anim_state = AS_BACKJUMP;
	}
	else if (coll->mid_floor <= 0)
		item->goal_anim_state = AS_STOP;
	else
	{
		item->goal_anim_state = AS_FASTFALL;
		item->pos.x_pos -= (64 * phd_sin(item->pos.y_rot)) >> 14;
		item->pos.z_pos -= (64 * phd_cos(item->pos.y_rot)) >> 14;
	}
}

void lara_as_poleleft(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;

	if (((input & (IN_LEFT | IN_ACTION)) != (IN_LEFT | IN_ACTION)) || input & (IN_FORWARD | IN_BACK) || item->hit_points <= 0)
		item->goal_anim_state = AS_POLESTAT;
	else
		item->pos.y_rot += 256;
}

void lara_as_poleright(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;

	if (((input & (IN_RIGHT | IN_ACTION)) != (IN_RIGHT | IN_ACTION)) || input & (IN_FORWARD | IN_BACK) || item->hit_points <= 0)
		item->goal_anim_state = AS_POLESTAT;
	else
		item->pos.y_rot -= 256;
}

void lara_col_poleup(ITEM_INFO* item, COLL_INFO* coll)
{
	short room_num;

	coll->enable_baddie_push = 0;
	coll->enable_spaz = 0;

	if (input & IN_LOOK)
		LookUpDown();

	if (!(input & (IN_ACTION | IN_FORWARD)) || item->hit_points <= 0)
		item->goal_anim_state = AS_POLESTAT;

	room_num = item->room_number;

	if (item->pos.y_pos - GetCeiling(GetFloor(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, &room_num),
		item->pos.x_pos, item->pos.y_pos, item->pos.z_pos) < 1024)
		item->goal_anim_state = AS_POLESTAT;
}

void lara_col_poledown(ITEM_INFO* item, COLL_INFO* coll)
{
	short room_number;

	coll->enable_spaz = 0;
	coll->enable_baddie_push = 0;

	if (input & IN_LOOK)
		LookUpDown();

	if (((input & (IN_BACK | IN_ACTION)) != (IN_BACK | IN_ACTION)) || item->hit_points <= 0)
		item->goal_anim_state = AS_POLESTAT;

	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 0;
	lara.move_angle = item->pos.y_rot;
	coll->slopes_are_walls = 1;
	coll->facing = lara.move_angle;
	coll->radius = 100;
	GetLaraCollisionInfo(item, coll);

	if (coll->mid_floor < 0)
	{
		room_number = item->room_number;
		item->floor = GetHeight(GetFloor(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos, &room_number),
			item->pos.x_pos, item->pos.y_pos - 762, item->pos.z_pos);
		item->goal_anim_state = AS_POLESTAT;
		item->item_flags[2] = 0;
	}

	if (input & IN_LEFT)
		item->pos.y_rot += 256;
	else if (input & IN_RIGHT)
		item->pos.y_rot -= 256;

	if (item->anim_number == ANIM_POLESLIDEE)
		item->item_flags[2] -= 1024;
	else
		item->item_flags[2] += 256;

	SoundEffect(340, &item->pos, 0);

	if (item->item_flags[2] <= 16384)
	{
		if (item->item_flags[2] < 0)
			item->item_flags[2] = 0;
	}
	else
		item->item_flags[2] = 16384;

	item->pos.y_pos += item->item_flags[2] >> 8;
}

void lara_as_rope(ITEM_INFO* item, COLL_INFO* coll)
{
	if (!(input & IN_ACTION))
		FallFromRope(item);

	if (input & IN_LOOK)
		LookUpDown();
}

void lara_col_rope(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION)
	{
		UpdateRopeSwing(item);

		if (input & IN_SPRINT)
		{
			lara.RopeDFrame = (anims[ANIM_SWINGFWD].frame_base + 32) << 8;
			lara.RopeFrame = lara.RopeDFrame;
			item->goal_anim_state = AS_ROPEFWD;
		}
		else if (input & IN_FORWARD && lara.RopeSegment > 4)
			item->goal_anim_state = AS_CLIMBROPE;
		else if (input & IN_BACK && lara.RopeSegment < 21)
		{
			item->goal_anim_state = AS_SLIDEROPE;
			lara.RopeFlag = 0;
			lara.RopeCount = 0;
		}
		else if (input & IN_LEFT)
			item->goal_anim_state = AS_ROPELEFT;
		else if (input & IN_RIGHT)
			item->goal_anim_state = AS_ROPERIGHT;
	}
	else
		FallFromRope(item);
}

void lara_as_ropel(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION)
	{
		if (input & IN_LEFT)
			lara.RopeY += 256;
		else
			item->goal_anim_state = AS_ROPE;
	}
	else
		FallFromRope(item);
}

void lara_as_roper(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION)
	{
		if (input & IN_RIGHT)
			lara.RopeY -= 256;
		else
			item->goal_anim_state = AS_ROPE;
	}
	else
		FallFromRope(item);
}

void lara_col_ropefwd(ITEM_INFO* item, COLL_INFO* coll)
{
	long Vel;

	camera.target_distance = 2048;
	UpdateRopeSwing(item);

	if (item->anim_number == ANIM_SWINGFWD)
	{
		if (input & IN_SPRINT)
		{
			if (ABS(lara.RopeLastX) < 9000)
				Vel = 192 * (9000 - ABS(lara.RopeLastX)) / 9000;
			else
				Vel = 0;

			ApplyVelocityToRope(lara.RopeSegment - 2, item->pos.y_rot + (!lara.RopeDirection ? 32760 : 0), (ushort)(Vel >> 5));
		}

		if (lara.RopeFrame < lara.RopeDFrame)
		{
			lara.RopeFrame += lara.RopeFrameRate;

			if (lara.RopeFrame > lara.RopeDFrame)
				lara.RopeFrame = lara.RopeDFrame;
		}
		else if (lara.RopeFrame > lara.RopeDFrame)
		{
			lara.RopeFrame -= lara.RopeFrameRate;

			if (lara.RopeFrame < lara.RopeDFrame)
				lara.RopeFrame = lara.RopeDFrame;
		}

		item->frame_number = (short)(lara.RopeFrame >> 8);

		if (!(input & IN_SPRINT) && lara.RopeFrame >> 8 == anims[ANIM_SWINGFWD].frame_base + 32 &&
			lara.RopeMaxXBackward < 6750 && lara.RopeMaxXForward < 6750)
		{
			item->anim_number = ANIM_SWING2HANG;
			item->frame_number = anims[ANIM_SWING2HANG].frame_base;
			item->current_anim_state = AS_ROPE;
			item->goal_anim_state = AS_ROPE;
		}

		if (input & IN_JUMP)
			JumpOffRope(item);
	}
	else if (item->frame_number == anims[ANIM_ROPEKICK].frame_base + 15)
		ApplyVelocityToRope(lara.RopeSegment, item->pos.y_rot, 128);
}

void lara_as_climbrope(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ROLL)
		FallFromRope(item);
	else
	{
		camera.target_angle = 5460;

		if (anims[item->anim_number].frame_end == item->frame_number)
		{
			item->frame_number = anims[item->anim_number].frame_base;
			lara.RopeSegment -= 2;
		}

		if (!(input & IN_FORWARD) || lara.RopeSegment <= 4)
			item->goal_anim_state = AS_ROPE;
	}
}

void lara_as_climbroped(ITEM_INFO* item, COLL_INFO* coll)
{
	if (input & IN_ACTION)
	{
		camera.target_angle = 5460;

		if (lara.RopeCount)
		{
			if (!lara.RopeFlag)
			{
				lara.RopeCount++;
				lara.RopeOffset += lara.RopeDownVel;

				if (!lara.RopeCount)
					lara.RopeFlag = 1;

				return;
			}
		}
		else if (!lara.RopeFlag)
		{
			lara.RopeOffset = 0;
			lara.RopeDownVel = (ulong)(RopeList[lara.RopePtr].MeshSegment[lara.RopeSegment + 1].y - RopeList[lara.RopePtr].MeshSegment[lara.RopeSegment].y) >> 17;
			lara.RopeCount = 0;
			lara.RopeOffset += lara.RopeDownVel;
			lara.RopeFlag = 1;
			return;
		}

		if (item->anim_number == ANIM_ROPESLIDEL && item->frame_number == anims[item->anim_number].frame_end)
		{
			item->frame_number = anims[item->anim_number].frame_base;
			lara.RopeFlag = 0;
			lara.RopeSegment++;
			lara.RopeOffset = 0;
		}

		if (!(input & IN_BACK) || lara.RopeSegment >= 21)
			item->goal_anim_state = AS_ROPE;
	}
	else
		FallFromRope(item);
}

void lara_col_jumper(ITEM_INFO* item, COLL_INFO* coll)
{
	coll->bad_pos = -NO_HEIGHT;
	coll->bad_neg = -384;
	coll->bad_ceiling = 192;
	GetLaraCollisionInfo(item, coll);
	LaraDeflectEdgeJump(item, coll);

	if (item->fallspeed > 0 && coll->mid_floor <= 0)
	{
		if (LaraLandedBad(item, coll))
			item->goal_anim_state = AS_DEATH;
		else
			item->goal_anim_state = AS_STOP;

		item->fallspeed = 0;
		item->gravity_status = 0;

		if (coll->mid_floor != NO_HEIGHT)
			item->pos.y_pos += coll->mid_floor;
	}
}

void inject_lara(bool replace)
{
	INJECT(0x00420B10, LaraAboveWater, replace);
	INJECT(0x00426C50, SetCornerAnim, replace);
	INJECT(0x00426AB0, CanLaraHangSideways, replace);
	INJECT(0x00425F50, lara_as_hang, replace);
	INJECT(0x00425FA0, lara_col_hang, replace);
	INJECT(0x00420A80, LaraCeilingFront, replace);
	INJECT(0x00421620, LaraFloorFront, replace);
	INJECT(0x00420FE0, LaraFallen, replace);
	INJECT(0x00420EA0, TestLaraSlide, replace);
	INJECT(0x00422390, LaraHitCeiling, replace);
	INJECT(0x00420D50, lara_as_duckl, replace);
	INJECT(0x00420D80, lara_as_duckr, replace);
	INJECT(0x00420DB0, lara_col_ducklr, replace);
	INJECT(0x00421040, lara_as_duck, replace);
	INJECT(0x004210F0, lara_col_duck, replace);
	INJECT(0x00421210, lara_as_all4s, replace);
	INJECT(0x00421280, lara_col_all4s, replace);
	INJECT(0x004216A0, lara_as_crawl, replace);
	INJECT(0x00421770, lara_col_crawl, replace);
	INJECT(0x00421880, LaraDeflectEdgeDuck, replace);
	INJECT(0x00421900, lara_as_all4turnl, replace);
	INJECT(0x004219B0, lara_as_all4turnr, replace);
	INJECT(0x00421960, lara_col_all4turnlr, replace);
	INJECT(0x00421A10, lara_as_crawlb, replace);
	INJECT(0x00421AD0, lara_col_crawlb, replace);
	INJECT(0x00421BF0, lara_col_crawl2hang, replace);
	INJECT(0x004220B0, lara_as_dash, replace);
	INJECT(0x00422200, lara_col_dash, replace);
	INJECT(0x00422350, GetLaraCollisionInfo, replace);
	INJECT(0x00422A90, lara_as_dashdive, replace);
	INJECT(0x00422AC0, lara_col_dashdive, replace);
	INJECT(0x00422BE0, LaraLandedBad, replace);
	INJECT(0x00422E50, MonkeySwingFall, replace);
	INJECT(0x00423660, MonkeySwingSnap, replace);
	INJECT(0x00422DE0, lara_as_hang2, replace);
	INJECT(0x00422EB0, lara_col_hang2, replace);
	INJECT(0x00423880, lara_as_monkeyswing, replace);
	INJECT(0x00423930, lara_col_monkeyswing, replace);
	INJECT(0x00423A60, lara_as_monkeyl, replace);
	INJECT(0x00423AC0, lara_col_monkeyl, replace);
	INJECT(0x00423B30, lara_as_monkeyr, replace);
	INJECT(0x00423B90, lara_col_monkeyr, replace);
	INJECT(0x00423C00, lara_as_hangturnl, replace);
	INJECT(0x00423CE0, lara_as_hangturnr, replace);
	INJECT(0x00423C40, lara_col_hangturnlr, replace);
	INJECT(0x00423D20, lara_as_monkey180, replace);
	INJECT(0x00423D40, lara_col_monkey180, replace);
	INJECT(0x00426B50, lara_as_hangleft, replace);
	INJECT(0x00426B90, lara_col_hangleft, replace);
	INJECT(0x00426BD0, lara_as_hangright, replace);
	INJECT(0x00426C10, lara_col_hangright, replace);
	INJECT(0x00426D70, lara_as_extcornerl, replace);
	INJECT(0x00426DC0, lara_as_extcornerr, replace);
	INJECT(0x00426E10, lara_as_intcornerl, replace);
	INJECT(0x00426E60, lara_as_intcornerr, replace);
	INJECT(0x00426EB0, lara_as_splat, replace);
	INJECT(0x004280F0, lara_col_splat, replace);
	INJECT(0x00426EC0, lara_as_compress, replace);
	INJECT(0x00428160, lara_col_compress, replace);
	INJECT(0x00426FD0, lara_as_back, replace);
	INJECT(0x00428210, lara_col_back, replace);
	INJECT(0x00427060, lara_as_null, replace);
	INJECT(0x00427070, lara_as_fastturn, replace);
	INJECT(0x004270C0, lara_as_stepright, replace);
	INJECT(0x00428350, lara_col_stepright, replace);
	INJECT(0x00427150, lara_as_stepleft, replace);
	INJECT(0x00428430, lara_col_stepleft, replace);
	INJECT(0x004271E0, lara_as_slide, replace);
	INJECT(0x00428450, lara_col_slide, replace);
	INJECT(0x00427210, lara_as_backjump, replace);
	INJECT(0x00428580, lara_col_backjump, replace);
	INJECT(0x00427260, lara_as_rightjump, replace);
	INJECT(0x004285B0, lara_col_rightjump, replace);
	INJECT(0x004272A0, lara_as_leftjump, replace);
	INJECT(0x004285E0, lara_col_leftjump, replace);
	INJECT(0x004272E0, lara_as_fallback, replace);
	INJECT(0x00428610, lara_col_fallback, replace);
	INJECT(0x00427310, lara_as_slideback, replace);
	INJECT(0x004286A0, lara_col_slideback, replace);
	INJECT(0x004286D0, lara_col_roll, replace);
	INJECT(0x00428770, lara_col_roll2, replace);
	INJECT(0x00427330, lara_as_pushblock, replace);
	INJECT(0x00427370, lara_as_pullblock, replace);
	INJECT(0x004273B0, lara_as_ppready, replace);
	INJECT(0x004273E0, lara_as_pickup, replace);
	INJECT(0x00427420, lara_as_pickupflare, replace);
	INJECT(0x00427480, lara_as_switchon, replace);
	INJECT(0x004274C0, lara_as_usekey, replace);
	INJECT(0x00427500, lara_as_usepuzzle, replace);
	INJECT(0x00427580, lara_as_special, replace);
	INJECT(0x004275A0, lara_as_swandive, replace);
	INJECT(0x00428850, lara_col_swandive, replace);
	INJECT(0x004275E0, lara_as_fastdive, replace);
	INJECT(0x004288D0, lara_col_fastdive, replace);
	INJECT(0x00427640, lara_as_waterout, replace);
	INJECT(0x00427660, lara_as_wade, replace);
	INJECT(0x00428960, lara_col_wade, replace);
	INJECT(0x00428AD0, lara_default_col, replace);
	INJECT(0x00427730, lara_as_deathslide, replace);
	INJECT(0x004277D0, lara_as_controlled, replace);
	INJECT(0x004278B0, lara_as_controlledl, replace);
	INJECT(0x004278D0, lara_col_turnswitch, replace);
	INJECT(0x00427940, lara_as_pulley, replace);
	INJECT(0x00425AC0, lara_as_walk, replace);
	INJECT(0x00427A40, lara_col_walk, replace);
	INJECT(0x00425B60, lara_as_run, replace);
	INJECT(0x00427C30, lara_col_run, replace);
	INJECT(0x00425D10, lara_as_fastback, replace);
	INJECT(0x00427E00, lara_col_fastback, replace);
	INJECT(0x00425D70, lara_as_turn_r, replace);
	INJECT(0x00427EF0, lara_col_turn_r, replace);
	INJECT(0x00425E20, lara_as_turn_l, replace);
	INJECT(0x00427FB0, lara_col_turn_l, replace);
	INJECT(0x00425EC0, lara_as_death, replace);
	INJECT(0x00427FD0, lara_col_death, replace);
	INJECT(0x00425F10, lara_as_fastfall, replace);
	INJECT(0x00428040, lara_col_fastfall, replace);
	INJECT(0x00424A60, lara_as_stop, replace);
	INJECT(0x00425070, lara_col_stop, replace);
	INJECT(0x00427DE0, lara_col_pose, replace);
	INJECT(0x00425110, lara_as_upjump, replace);
	INJECT(0x00425130, lara_col_upjump, replace);
	INJECT(0x00425480, lara_as_forwardjump, replace);
	INJECT(0x00425550, lara_col_forwardjump, replace);
	INJECT(0x00425630, lara_as_reach, replace);
	INJECT(0x00425650, lara_col_reach, replace);
	INJECT(0x00423D60, lara_col_polestat, replace);
	INJECT(0x00423EE0, lara_as_poleleft, replace);
	INJECT(0x00423F20, lara_as_poleright, replace);
	INJECT(0x00423F60, lara_col_poleup, replace);
	INJECT(0x00423FF0, lara_col_poledown, replace);
	INJECT(0x004241A0, lara_as_rope, replace);
	INJECT(0x00424270, lara_col_rope, replace);
	INJECT(0x00424620, lara_as_ropel, replace);
	INJECT(0x00424650, lara_as_roper, replace);
	INJECT(0x00424680, lara_col_ropefwd, replace);
	INJECT(0x004248E0, lara_as_climbrope, replace);
	INJECT(0x00424950, lara_as_climbroped, replace);
	INJECT(0x00428B20, lara_col_jumper, replace);
}