//FIXME: most of this should be in common/constants.qc

//
// constants
//

// point content values

float	CONTENT_EMPTY			= -1;
float	CONTENT_SOLID			= -2;
float	CONTENT_WATER			= -3;
float	CONTENT_SLIME			= -4;
float	CONTENT_LAVA			= -5;
float	CONTENT_SKY				= -6;

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
float	CHAN_AUTO		= 0;
float	CHAN_WEAPON		= 1;
float	CHAN_VOICE		= 2;
float	CHAN_ITEM		= 3;
float	CHAN_BODY		= 4;

float	ATTN_NONE		= 0;
float	ATTN_NORM		= 1;
float	ATTN_IDLE		= 2;
float	ATTN_STATIC		= 3;


// entity effects

float	EF_BRIGHTFIELD	= 1;
float	EF_MUZZLEFLASH 	= 2;
float	EF_BRIGHTLIGHT 	= 4;
float	EF_DIMLIGHT 	= 8;




float	MASK_ENGINE		= 1;	//this is special. Any entities known by the engine but not the csqc will be added.
//you can add any other masks below, remember, use bits.
float	MASK_NORMAL		= 2;



float INPUT_KEYDOWN = 0;
float INPUT_KEYUP = 1;
float INPUT_MOUSEMOVE = 2;



//stats 0-31 are filled by the engine.
//they are for the 'fixed function' stuff, that works without requiring csqc.
//these stats need to be standardized amoung engines, so you're not allowed to mod them, so nur.
#define STAT_HEALTH 0
#define STAT_WEAPONMODEL 2	//weapon model index, as evalutated by server code.
#define STAT_AMMO 3
#define STAT_ARMOR	 4
#define STAT_WEAPONFRAME	 5
#define STAT_SHELLS 6
#define STAT_NAILS 7
#define STAT_ROCKETS 8
#define STAT_CELLS 9
#define STAT_ACTIVEWEAPON 10	//the one shown on the hud
#define STAT_ITEMS 15
//stats 32 onwards are filled by the csqc.


#define FL_ONGROUND 1



enum
{
	VF_MIN = 1,
	VF_MIN_X = 2,
	VF_MIN_Y = 3,
	VF_SIZE = 4,
	VF_SIZE_X = 5,
	VF_SIZE_Y = 6,
	VF_VIEWPORT = 7,
	VF_FOV = 8,
	VF_FOVX = 9,
	VF_FOVY = 10,
	VF_ORIGIN = 11,
	VF_ORIGIN_X = 12,
	VF_ORIGIN_Y = 13,
	VF_ORIGIN_Z = 14,
	VF_ANGLES = 15,
	VF_ANGLES_X = 16,
	VF_ANGLES_Y = 17,
	VF_ANGLES_Z = 18,
	VF_DRAWWORLD = 19,
	VF_DRAWENGINESBAR = 20,
	VF_DRAWCROSSHAIR = 21,
	VF_PERSPECTIVE = 200
};

enumflags {
	RF_VIEWMODEL,
	RF_EXTERNALMODEL,
	RF_DEPTHHACK,
	RF_ADDATIVE,
	RF_USEAXIS
};

