#ifndef PMOVE_H
#define PMOVE_H

#include "qtypes.h"
#include "protocol.h"

typedef enum waterlevel_e
{
	WATERLEVEL_NONE,
	WATERLEVEL_WETFEET,
	WATERLEVEL_SWIMMING,
	WATERLEVEL_SUBMERGED
}
waterlevel_t;

typedef struct playermove_s
{
	// entity to be ignored for movement
	struct prvm_edict_s *self;
	// position
	vec3_t origin;
	vec3_t velocity;
	vec3_t angles;
	vec3_t movedir;
	// current bounding box (different if crouched vs standing)
	vec3_t mins;
	vec3_t maxs;
	// currently on the ground
	qbool onground;
	// currently crouching
	qbool crouched;
	// what kind of water (SUPERCONTENTS_LAVA for instance)
	int watertype;
	// how deep
	waterlevel_t waterlevel;
	// weird hacks when jumping out of water
	// (this is in seconds and counts down to 0)
	float waterjumptime;

	int movetype;

	// user command
	usercmd_t cmd;
}
playermove_t;

typedef struct movevars_s
{
	unsigned int moveflags;
	float wallfriction;
	float waterfriction;
	float friction;
	float timescale;
	float gravity;
	float stopspeed;
	float maxspeed;
	float spectatormaxspeed;
	float accelerate;
	float airaccelerate;
	float wateraccelerate;
	float entgravity;
	float jumpvelocity;
	float edgefriction;
	float maxairspeed;
	float stepheight;
	float airaccel_qw;
	float airaccel_qw_stretchfactor;
	float airaccel_sideways_friction;
	float airstopaccelerate;
	float airstrafeaccelerate;
	float maxairstrafespeed;
	float airstrafeaccel_qw;
	float aircontrol;
	float aircontrol_power;
	float aircontrol_penalty;
	float warsowbunny_airforwardaccel;
	float warsowbunny_accel;
	float warsowbunny_topspeed;
	float warsowbunny_turnaccel;
	float warsowbunny_backtosideratio;
	float ticrate;
	float airspeedlimit_nonqw;
} movevars_t;

#endif
