// Bot_weapons.h
//

#pragma once

#define BOT_MAX_WEAPONS				32

//projectile flags
#define PFL_WINDOWDAMAGE			1		//projectile damages through window
#define PFL_RETURN					2		//set when projectile returns to owner
//weapon flags
#define WFL_FIRERELEASED			1		//set when projectile is fired with key-up event
//damage types
#define DAMAGETYPE_IMPACT			1		//damage on impact
#define DAMAGETYPE_RADIAL			2		//radial damage
#define DAMAGETYPE_VISIBLE			4		//damage to all entities visible to the projectile

struct projectileinfo_t
{
	projectileinfo_t()
	{
		name = "";
		model = "";
		flags = 0;
		gravity = 0;
		damage = 0;
		radius = 0;
		visdamage = 0;
		damagetype = 0;
		healthinc = 0;
		push = 0;
		detonation = 0;
		bounce = 0;
		bouncefric = 0;
		bouncestop = 0;
	}
	idStr name;
	idStr model;
	int flags;
	float gravity;
	int damage;
	float radius;
	int visdamage;
	int damagetype;
	int healthinc;
	float push;
	float detonation;
	float bounce;
	float bouncefric;
	float bouncestop;
};

struct weaponinfo_t
{
	weaponinfo_t()
	{
		valid = 0;
		number = 0;
		name = "";
		model = "";
		level = 0;
		weaponindex = 0;
		flags = 0;
		projectile = "";
		numprojectiles = 0;
		hspread = 0;
		vspread = 0;
		speed = 0;
		acceleration = 0;
		recoil.Zero();
		offset.Zero();
		angleoffset.Zero();
		extrazvelocity = 0;
		ammoamount = 0;
		ammoindex = 0;
		activate = 0;
		reload = 0;
		spinup = 0;
		spindown = 0;
	}

	int valid;					//true if the weapon info is valid
	int number;									//number of the weapon
	idStr name;
	idStr model;
	int level;
	int weaponindex;
	int flags;
	idStr projectile;
	int numprojectiles;
	float hspread;
	float vspread;
	float speed;
	float acceleration;
	idVec3 recoil;
	idVec3 offset;
	idVec3 angleoffset;
	float extrazvelocity;
	int ammoamount;
	int ammoindex;
	float activate;
	float reload;
	float spinup;
	float spindown;
	projectileinfo_t proj;						//pointer to the used projectile
};

struct bot_weaponstate_t
{
	bot_weaponstate_t()
	{
		Reset();
	}

	void Reset()
	{
		inUse = false;
		weaponweightconfig = NULL;
		for( int i = 0; i < BOT_MAX_WEAPONS; i++ )
		{
			weaponweightindex[i] = 0;
		}
	}

	bool inUse;
	weightconfig_t* weaponweightconfig;
	int weaponweightindex[BOT_MAX_WEAPONS];							//weapon weight index
};

//
// idBotWeaponInfoManager
//
class idBotWeaponInfoManager
{
public:
	void	Init( void );

	int		BotLoadWeaponWeights( int weaponstate, char* filename );
	void	BotGetWeaponInfo( int weaponstate, int weapon, weaponinfo_t* weaponinfo );
	int		BotChooseBestFightWeapon( int weaponstate, int* inventory );
	void	BotResetWeaponState( int weaponstate );
	int		BotAllocWeaponState( void );
	void	BotFreeWeaponState( int ws );
private:
	int		BotValidWeaponNumber( int weaponnum );
	bot_weaponstate_t* BotWeaponStateFromHandle( int handle );
	void	WeaponWeightIndex( weightconfig_t* wwc, bot_weaponstate_t* weaponState );
	void	BotFreeWeaponWeights( int weaponstate );
private:
	void	LoadWeaponConfig( char* filename );

	void	ParseWeaponInfo( idParser& parser, weaponinfo_t& newWeaponInfo );
	void	ParseProjectileInfo( idParser& parser, projectileinfo_t& newProjectileInfo );
private:
	idList<projectileinfo_t> projectileinfo;
	bot_weaponstate_t botweaponstates[MAX_CLIENTS + 1];

	weaponinfo_t weaponinfo[BOT_MAX_WEAPONS];
};

extern idBotWeaponInfoManager botWeaponInfoManager;