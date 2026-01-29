//Don't modify this file

#define CSQC	//so code sections can be compiled for either progs
/*
==============================================================================

			SOURCE FOR GLOBALVARS_T C STRUCTURE

==============================================================================
*/

//
// system globals
//
entity		self;
entity		other;
entity		world;
float		time;
float		frametime;

float 		player_localentnum;	//the entnum
float 		player_localnum;	//the playernum
float		maxclients;	//a constant filled in by the engine. gah, portability eh?

float		clientcommandframe;	//player movement
float		servercommandframe;	//clientframe echoed off the server

string		mapname;	//current brief map name
float		intermission;	//non-zero if the server sent an svc_intermission.

//
// global variables set by built in functions
//
vector		v_forward, v_up, v_right;	// set by makevectors()

// set by traceline / tracebox
float		trace_allsolid;
float		trace_startsolid;
float		trace_fraction;
vector		trace_endpos;
vector		trace_plane_normal;
float		trace_plane_dist;
entity		trace_ent;
float		trace_inopen;
float		trace_inwater;

//these fields are read and set by the default player physics
vector		pmove_org;
vector		pmove_vel;
vector		pmove_mins;
vector		pmove_maxs;
//retrieved from the current movement commands (read by player physics)
float		input_timelength;
vector		input_angles;
vector		input_movevalues;	//forwards, right, up.
float		input_buttons;		//attack, use, jump (default physics only uses jump)

float		movevar_gravity;
float		movevar_stopspeed;
float		movevar_maxspeed;
float		movevar_spectatormaxspeed;	//used by NOCLIP movetypes.
float		movevar_accelerate;
float		movevar_airaccelerate;
float		movevar_wateraccelerate;
float		movevar_friction;
float		movevar_waterfriction;
float		movevar_entgravity;	//the local player's gravity field. Is a multiple (1 is the normal value)

//================================================
void		end_sys_globals;		// flag for structure dumping
//================================================

/*
==============================================================================

			SOURCE FOR ENTVARS_T C STRUCTURE

==============================================================================
*/

//
// system fields (*** = do not set in prog code, maintained by C code)
//
.float		modelindex;		// *** model index in the precached list
.vector		absmin, absmax;	// *** origin + mins / maxs

.float		entnum;	// *** the ent number as on the server
.float		drawmask;
.void()		predraw;

.float		movetype;
.float		solid;

.vector		origin;			// ***
.vector		oldorigin;		// ***
.vector		velocity;
.vector		angles;
.vector		avelocity;

.string		classname;		// spawn function

.float		renderflags;
.string		model;
.float		frame;
.float		frame2;
.float		lerpfrac;	//the ammount of frame2 to use.
.float		skin;

.vector		mins, maxs;		// bounding box extents reletive to origin
.vector		size;			// maxs - mins

.void()		touch;
.void()		use;
.void()		think;
.void()		blocked;		// for doors or plats, called when can't push other

.float		nextthink;

.entity		chain;

.entity 	enemy;

.float		flags;

.float		colormap;

.entity		owner;		// who launched a missile

//================================================
void		end_sys_fields;			// flag for structure dumping
//================================================
