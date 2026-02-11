#include "../plugin.h"

#define DEFAULTHUDNAME "ftehud.hud"

#define MAX_ELEMENTS 128

int testvar;
int testvar2;
int testvar3;
int testvar4;
int K_UPARROW;
int K_DOWNARROW;
int K_LEFTARROW;
int K_RIGHTARROW;
int K_ESCAPE;
int K_MOUSE1;
int K_MOUSE2;
int K_HOME;
int K_SHIFT;
int K_SPACE;
int K_F1;
int K_F2;


#define texture qhandle_t
#define bool qboolean

float realtime;

#define Sound_Start(x)






void vecNorm(float *in, float *out)
{	
	float	new;	

	new = in[0] * in[0] + in[1] * in[1] + in[2]*in[2];
	new = (float)sqrt(new);
	
	if (new == 0)
		out[0] = out[1] = out[2] = 0;
	else
	{
		new = 1/new;
		out[0] = in[0] * new;
		out[1] = in[1] * new;
		out[2] = in[2] * new;
	}
	
}

#if 0
#define MAXCANNONS 4
#define MAXBULLETS 256
#define MAXMONSTERS 256
#define ENEMYTYPES 10
#define SHOTLEVELS 10
#else
enum {
	M_CANNON1,
	M_CANNON2,
	M_CANNON3,
	M_CANNON4,
	M_CANNON5,
	M_CANNON6,
	M_CANNON7,
	M_CANNON8,
	M_CANNON9,
	M_CANNON10,
	M_CANNON11,
	M_CANNON12,

	M_ROCKETL1 = 50,
	M_ROCKETL2,
	M_ROCKETL3,
	M_ROCKETL4,
	M_ROCKETL5,
	M_ROCKETL6,
	M_ROCKETL7,
	M_ROCKETL8,
	M_ROCKETL9,
	M_ROCKETL10,
	M_ROCKETL11,
	M_ROCKETL12,

	M_FORWARD = 100,
	M_DIAG,
	M_DIAG2,
	M_SIDE,

	MAXADDONS
};
#define FIRSTCANNON M_CANNON1
#define LASTCANNON M_ROCKETL1-1

#define FIRSTROCKET M_ROCKETL1
#define LASTROCKET M_FORWARD-1

#define FIRSTCENTRALMOD M_FORWARD
#define LASTCENTRALMOD MAXADDONS-1

int MAXBULLETS = 255;
int MAXMONSTERS = 256;
int MAXSQUADS = 256;
int MAXROUTES = 64;
int MAXROUTEPOINTS=10;
#define ENEMYTYPES monsterpics
#define SHOTLEVELS weaponlevels
#endif
#define CONT_LEFTKEY	1
#define CONT_RIGHTKEY	2
#define CONT_FIREKEY	4
#define CONT_UPKEY		8
#define CONT_DOWNKEY	16
#define CONT_MOUSE		32	//right mouse button pressed

#define PLAYERSHIPSPEEDX	2
#define PLAYERSHIPSPEEDY	2
#define ENEMYSHIPSPEEDX	(random()-0.5)
#define ENEMYSHIPSPEEDY	(random()*2+(float)1)


#define PUP_SHRUNKEN	1	//one round only

void SIRestartGame (void);
float random(void);

int SIlevel;
char SILevelName[128];

int weaponlevels;
int monsterpics;

//Textures in structures are pointers to the texture number. This means we don't have to cycle through each one when we reload the textures.

typedef struct SIEnemyType_s
{
	texture *picture;
	float width;
	float height;
	int health;

	float FireProb;

	int reward;

	int number;

	int AI;
	int healthregen;

	int shotdamage;
	texture *shotpicture;
} SIEnemyType_t;

typedef struct SIRoute_s
{
	int numpoints;
	vec3_t pos[1];
} SIRoute_t;

typedef struct SISquad_s
{
	int route;
	int type;
	int number;
	float interval;
	float time;
	float speed;
} SISquad_t;


typedef struct SIaddon_s
{
	float xoff;
	float yoff;

	int power;
	float refire;
	float reloadtime;
	texture *tex;
} SIaddon_t;

typedef struct SIp_s
{
	float x;
	float y;
	float width;
	float height;
	int cash;
	int health;	

	int contols;
	int powerups;	
		
	SIaddon_t cannon[MAXADDONS];
} SIp_t;

typedef struct SImonster_s
{
	float xpos;
	float ypos;
	float width;
	float height;

	int routenum;
	int destnum;

	float xvel;
	float yvel;

	int health;

	int timetoregen;

	SIEnemyType_t *EnemyType;
} SImonster_t;

typedef struct SIbullet_s
{
	int type;
	int charge;

	float xpos;
	float ypos;
	float width;
	float height;
	float xvel;
	float yvel;
	float yaccel;

	qboolean inuse;
	int damage;
	float alpha;
	float alphachange;

	texture *tex;
} SIbullet_t;

typedef struct ShopRegion_s
{
	float x;
	float y;
	float width;
	float height;

	char *text;

	texture *tex;
	qboolean (*AppearCondition) (int ident);
	void (*CallFunc) (int ident);
	texture *(*Texture) (texture *def, int ident);
	int ident;
} ShopRegion_t;

int notenoughcash;

SIRoute_t	*SIRoute;
SISquad_t	*SISquad;
SIEnemyType_t *SIEnemyType;
SIp_t SIp;
SImonster_t *SImonster;
SIbullet_t *SIbullet;
SIbullet_t *SIweapons;	//prototypes

qboolean SIShop;
qboolean lastlevel;
float deadtime;
float leveltime;

int livingmonsters;

texture SIplayertexture;	//both
texture *SIbaddietexture;	//game only
texture *SIbullettexture;	//game only
texture SIexplosiontexture;	//game only
texture SIhealthtexture;		//shop only
texture SIleaveshoptexture;	//shop only
texture SIshrinktexture;		//shop only
texture SIcannontexture;	//game+shop
texture SIsideshottexture;	//shop only
texture SIrapidtexture;
texture SIsavegametexture;
texture SIloadgametexture;
texture SIquittexture;
/*
sound SoundWin;
sound SoundLoose;
sound SoundFire;
sound SoundExplosion;
sound SoundHit;	//when we don't kill
*/
void SISPHealth (int ident)
{
	if (SIp.health >= 9)
		return;
	if (SIp.cash < 250)
	{
		notenoughcash = 250;
		return;
	}
	SIp.health++;
	SIp.cash -= 250;
}
qboolean SISPHealthThere (int ident)
{
	if (SIp.health < 9)
		return true;
	return false;
}

void SISPUpgrade1 (int ident)
{
	if (SIp.cannon[M_FORWARD].power >= weaponlevels-1)
		return;

	if (SIp.cash < 500)
	{
		notenoughcash = 250;
		return;
	}
	SIp.cannon[M_FORWARD].power++;
	SIp.cash -= 500;
}
texture *SISPUpgrade1Texture (texture *def, int ident)
{
	return &SIbullettexture[SIp.cannon[M_FORWARD].power+1];
}
qboolean SISPUpgrade1There (int ident)
{
	if (SIp.cannon[M_FORWARD].power < weaponlevels-1)
		return true;
	return false;
}

void SISPShrink (int ident)
{
	if (SIp.powerups & PUP_SHRUNKEN)	//can't reshrink
		return;

	if (SIp.cash < 100)
	{
		notenoughcash = 250;
		return;
	}
	SIp.powerups |= PUP_SHRUNKEN;
	SIp.cash -= 100;
}
bool SISPShrinkThere (int ident)
{
	if (SIp.powerups & PUP_SHRUNKEN)
		return false;
	return true;
}

void SISPCannon (int ident)
{
	int a;

	if (SIp.cash < 1500)
	{
		notenoughcash = 250;
		return;
	}

	for (a = FIRSTCANNON; a <= LASTCANNON; a++)
	{
		if (SIp.cannon[a].power == 0)
		{
			SIp.cannon[a].tex = &SIcannontexture;
			SIp.cannon[a].power = 1;

			SIp.cash -= 1500;
			return;
		}
	}
}

bool SISPCannonThere (int ident)
{
	int a;

	for (a = FIRSTCANNON; a <= LASTCANNON; a++)
	{
		if (SIp.cannon[a].power == 0)
		{
			return true;
		}
	}

	return false;
}

texture *SISPCannonUpgradeTexture (texture *def, int ident)
{
	return &SIbullettexture[SIp.cannon[ident].power];
}
void SISPCannonUpgrade(int ident)
{
	if (SIp.cannon[ident].power >= weaponlevels)
		return;

	if (SIp.cash < 600)
	{
		notenoughcash = 250;
		return;
	}
	SIp.cannon[ident].power++;	
	SIp.cash -= 600;
}
bool SISPCannonUpgradeThere(int ident)
{	
	if (SIp.cannon[ident].power >= weaponlevels || SIp.cannon[ident].power <= 0)
		return false;
	return true;
}

void SISPCannonBoost(int ident)
{
	if (SIp.cannon[ident].reloadtime <= 0.1f)
		return;

	if (SIp.cash < 600)
	{
		notenoughcash = 250;
		return;
	}
	SIp.cannon[ident].reloadtime -= 0.1f;	
	SIp.cash -= 600;
}
bool SISPCannonBoostThere(int ident)
{	
	if (SIp.cannon[ident].reloadtime <= 0.1f || SIp.cannon[ident].power <= 0)
		return false;
	return true;
}

void SISPRocket (int ident)
{
	int a;

	if (SIp.cash < 1500)
	{
		notenoughcash = 250;
		return;
	}

	for (a = FIRSTROCKET; a <= LASTROCKET; a++)
	{
		if (SIp.cannon[a].power == 0)
		{
			SIp.cannon[a].tex = &SIcannontexture;
			SIp.cannon[a].power = 1;

			SIp.cash -= 1500;
			return;
		}
	}
}

bool SISPRocketThere (int ident)
{
	int a;

	for (a = FIRSTROCKET; a <= LASTROCKET; a++)
	{
		if (SIp.cannon[a].power == 0)
		{
			return true;
		}
	}

	return false;
}

texture *SISPSideBurstTexture (texture *def, int ident)
{
	return &SIbullettexture[SIp.cannon[M_SIDE].power];
}
void SISPSideBurst (int ident)
{
	if (SIp.cannon[M_SIDE].power >= weaponlevels)
		return;

	if (SIp.cash < 1000)
	{
		notenoughcash = 250;
		return;
	}

	SIp.cash -= 1000;

	SIp.cannon[M_SIDE].power+=1;
}
bool SISPSideBurstThere (int ident)
{
	if (SIp.cannon[M_SIDE].power >= weaponlevels)
		return false;
	return true;
}

void SISPRapidFire (int ident)
{
	if (SIp.cannon[M_FORWARD].reloadtime <= 0.1)
		return;

	if (SIp.cash < 250)
	{
		notenoughcash = 250;
		return;
	}

	SIp.cash -= 250;

	SIp.cannon[M_FORWARD].reloadtime -= 0.1f;
}
bool SISPRapidFireThere (int ident)
{
	if (SIp.cannon[M_FORWARD].reloadtime <= 0.1)
		return false;
	return true;
}

void NextLevel (void);
void LeaveShop (int ident)
{
	NextLevel();
}

bool gamesaved = 2;
void SISaveGame(int ident)
{
	int a;
	texture handle;

	FS_Open("spaceinv/spaceinv.sav", &handle, 2);

	FS_Write(handle, &SIp.cash, sizeof(SIp.cash));
	FS_Write(handle, &SIp.powerups, sizeof(SIp.powerups));
	FS_Write(handle, &SIp.health, sizeof(SIp.health));
	FS_Write(handle, &SIlevel, sizeof(SIlevel));

	for (a = 0; a < MAXADDONS; a++)
	{
		FS_Write(handle, &SIp.cannon[a].power, sizeof(SIp.cannon[a].power));
		FS_Write(handle, &SIp.cannon[a].reloadtime, sizeof(SIp.cannon[a].reloadtime));
	}

	FS_Close(handle);

	gamesaved = true;
}

void SILoadGame(int ident)
{
	int a;
	int len;
	texture handle;

	len = FS_Open("spaceinv/spaceinv.sav", &handle, 1);
	if (len < 0)
		return;

	FS_Read(handle, &SIp.cash, sizeof(SIp.cash));
	FS_Read(handle, &SIp.powerups, sizeof(SIp.powerups));
	FS_Read(handle, &SIp.health, sizeof(SIp.health));
	FS_Read(handle, &SIlevel, sizeof(SIlevel));

	for (a = 0; a < MAXADDONS; a++)
	{
		FS_Read(handle, &SIp.cannon[a].power, sizeof(SIp.cannon[a].power));
		FS_Read(handle, &SIp.cannon[a].reloadtime, sizeof(SIp.cannon[a].reloadtime));

		if (SIp.cannon[a].power > weaponlevels)
			SIp.cannon[a].power = weaponlevels;

	}

	FS_Close(handle);
}

bool SILoadGameThere(int ident)
{
	texture handle;
	if (gamesaved = 2)
	{
		if (FS_Open("spaceinv/spaceinv.sav", &handle, 1) >= 0)
		{
			FS_Close(handle);
			gamesaved = true;
		}
		else
			gamesaved = false;
	}
	return gamesaved;	
}

bool SIFTRUE (int ident) {return true;}
texture *SIShopTexture (texture *def, int ident)
{
	return def;
}

ShopRegion_t ShopRegion[] =
{	
	{40,	40,		40,	40,	"3Leave Shop",				&SIleaveshoptexture,	SIFTRUE,			LeaveShop,		SIShopTexture		},
	{40,	80,		40,	40,	"3       Heal     (250)",	&SIhealthtexture,		SISPHealthThere,	SISPHealth,		SIShopTexture		},
	{40,	120,	40,	40,	"3   Rapid Fire   (250)",	&SIrapidtexture,		SISPRapidFireThere,	SISPRapidFire,	SIShopTexture		},
	{80,	40,		40,	40,	"3Upgrade Weapons (500)",	NULL,					SISPUpgrade1There,	SISPUpgrade1,	SISPUpgrade1Texture	},
	{80,	80,		40,	40,	"3      Shrink    (100)",	&SIshrinktexture,		SISPShrinkThere,	SISPShrink,		SIShopTexture		},
//	{80,	120,	40,	40,	"4Quit Game",				&SIquittexture,			SIFTRUE,			SIQuit,			SIShopTexture		},
	{120,	40,		40,	40,	"3   Side Cannon (1500)",	&SIcannontexture,		SISPCannonThere,	SISPCannon,		SIShopTexture,	1	},
	{120,	80,		40,	40,	"3    Spreader   (1000)",	&SIsideshottexture,		SISPSideBurstThere,	SISPSideBurst,	SIShopTexture		},
	{120,	80,		20,	20,	"3    Spreader   (1000)",	NULL,					SISPSideBurstThere,	SISPSideBurst,	SISPSideBurstTexture},
	{120,	120,	40,	40,	"3Save Game",				&SIsavegametexture,		SIFTRUE,			SISaveGame,		SIShopTexture		},
	{120,	160,	40,	40,	"3Load Game",				&SIloadgametexture,		SILoadGameThere,	SILoadGame,		SIShopTexture		},
	{80,	160,	40,	40,	"3RocketLauncher (1500)",	&SIcannontexture,		SISPRocketThere,	SISPRocket,		SIShopTexture,	1	},

	{160,	40,		40,	40,	"3Upgrade Cannon1(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 0},
	{200,	40,		40,	40,	"3Upgrade Cannon2(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 1},
	{240,	40,		40,	40,	"3Upgrade Cannon3(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 2},
	{280,	40,		40,	40,	"3Upgrade Cannon4(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 3},
	{320,	40,		40,	40,	"3Upgrade Cannon5(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 4},
	{360,	40,		40,	40,	"3Upgrade Cannon6(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 5},
	{400,	40,		40,	40,	"3Upgrade Cannon7(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 6},
	{440,	40,		40,	40,	"3Upgrade Cannon8(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 7},
	{480,	40,		40,	40,	"3Upgrade Cannon9(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 8},
	{520,	40,		40,	40,	"3Upgrade Cannon10(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 9},
	{560,	40,		40,	40,	"3Upgrade Cannon11(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 10},
	{600,	40,		40,	40,	"3Upgrade Cannon12(600)",	NULL,					SISPCannonUpgradeThere,	SISPCannonUpgrade,	SISPCannonUpgradeTexture, 11},

	{160,	80,		40,	40,	"3 Boost Cannon1 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 0},
	{200,	80,		40,	40,	"3 Boost Cannon2 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 1},
	{240,	80,		40,	40,	"3 Boost Cannon3 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 2},
	{280,	80,		40,	40,	"3 Boost Cannon4 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 3},
	{320,	80,		40,	40,	"3 Boost Cannon5 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 4},
	{360,	80,		40,	40,	"3 Boost Cannon6 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 5},
	{400,	80,		40,	40,	"3 Boost Cannon7 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 6},
	{440,	80,		40,	40,	"3 Boost Cannon8 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 7},
	{480,	80,		40,	40,	"3 Boost Cannon9 (600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 8},
	{520,	80,		40,	40,	"3 Boost Cannon10(600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 9},
	{560,	80,		40,	40,	"3 Boost Cannon11(600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 10},
	{600,	80,		40,	40,	"3 Boost Cannon12(600)",	&SIrapidtexture,					SISPCannonBoostThere,	SISPCannonBoost,	SIShopTexture, 11},
};

ShopRegion_t *CurrentRegion = &ShopRegion[0];

SIbullet_t *SI_NewBullet (int bulletlevel)
{
	int a;	
	for (a = 0; a < MAXBULLETS; a++)
	{
		if (!SIbullet[a].inuse)
		{
#if 0
			memset(&SIbullet[a], 0, sizeof(SIbullet_t));			
			SIbullet[a].damage = bulletlevel+1;
			SIbullet[a].inuse = true;
			SIbullet[a].alpha = 1.0;
			SIbullet[a].tex = &SIbullettexture[bulletlevel];
#else
			if (bulletlevel < 0)
			{
				memcpy(&SIbullet[a], &SIweapons[0], sizeof(SIbullet_t));
				SIbullet[a].type = bulletlevel;
				SIbullet[a].alphachange+=0.05f;
			}
			else			
				memcpy(&SIbullet[a], &SIweapons[bulletlevel], sizeof(SIbullet_t));
			SIbullet[a].inuse = true;						
#endif
 			return &SIbullet[a];
		}
	}
	return NULL;
}

SIbullet_t *SI_NewExplosion (float cx, float cy, float size)
{
	int a;
	for (a = 0; a < MAXBULLETS; a++)
	{
		if (!SIbullet[a].inuse)
		{
			memset(&SIbullet[a], 0, sizeof(SIbullet_t));
			SIbullet[a].inuse = true;
			SIbullet[a].alpha = (float)0.005;
			SIbullet[a].alphachange = (float)0.01;
			SIbullet[a].tex = &SIexplosiontexture;

			SIbullet[a].width = size;
			SIbullet[a].height = size;
			SIbullet[a].xpos = cx - size/2;
			SIbullet[a].ypos = cy - size/2;
			return &SIbullet[a];
		}
	}
	return NULL;
}

float random(void)
{		
	return (float)(rand() & 0x7fff) / (float)0x7fff;
}

void Draw_String2C(int x, int y, char *string, int extraparam)
{
	string++;
	x -= strlen(string)*4;
	while(*string)
	{
		Draw_Character(x+=8, y, *string++);
	}
}

void Draw_Picture(texture tex, float x, float y, float w, float h)
{
	Draw_Image(x, y, w, h, 0, 0, 1, 1, tex);
}

int levelnametime = 0;
int frame = 0;
void SI_2D (void)
{	
	int a;
	int b;
	int i;
	float x;
	float y;

	frame++;
	if (SIShop)
	{
		if (lastlevel)
		{
			Draw_Colour4f(1, 1, 1, 1);
			Draw_String2C(vid.width / 2, vid.height / 3, "3You have compleated the game.", 2);
			Draw_String2C(vid.width / 2, vid.height / 2, "3Well done.", 2);
			Draw_String2C(vid.width / 2, (vid.height / 3)*2, "3Press any key to restart.", 2);

			Draw_String2C(vid.width / 2, vid.height-8, va("3Cash: %i", SIp.cash), 2);
			return;
		}
		if (SIp.powerups & PUP_SHRUNKEN)
		{
			SIp.width = 16;
			SIp.height = 16;
		}
		else
		{
			SIp.width = 32;
			SIp.height = 32;
		}


		for (a = 0; a < sizeof(ShopRegion)/sizeof(ShopRegion_t); a++)
		{			
			if ((*ShopRegion[a].AppearCondition) (ShopRegion[a].ident))
				Draw_Image(ShopRegion[a].x, ShopRegion[a].y, ShopRegion[a].width, ShopRegion[a].height, 0, 0, 1, 1, *(*ShopRegion[a].Texture) (ShopRegion[a].tex, ShopRegion[a].ident));
		}

		if (SIp.health < 4)
			Draw_String2C(8, 8, va("4%i", SIp.health), 2);
		else
			Draw_String2C(8, 8, va("3%i", SIp.health), 2);
		
		Draw_Picture(SIplayertexture, SIp.x, SIp.y, SIp.width, SIp.height);
		for (a = 0; a < MAXADDONS; a++)
		{
			if (SIp.cannon[a].power)
			{
				if (SIp.cannon[a].tex)
					Draw_Picture(*SIp.cannon[a].tex, SIp.x + SIp.width*SIp.cannon[a].xoff, SIp.y + SIp.width*SIp.cannon[a].yoff, SIp.width, SIp.height);
			}
		}
//		if (SIp.powerups & PUP_CANNON)
//			Draw_Picture(SIcannontexture, SIp.x - SIp.width, SIp.y, SIp.width, SIp.height);
//		if (SIp.powerups & PUP_CANNON2)
//			Draw_Picture(SIcannontexture, SIp.x + SIp.width, SIp.y, SIp.width, SIp.height);

		if (notenoughcash)
		{
			notenoughcash--;			
			Draw_Colour4f(1, 1, 1, (float)notenoughcash / 250);
			Draw_String2C(vid.width / 2, vid.height / 2, "4Not enough cash!", 2);
		}
		Draw_Colour4f(1, 1, 1, 1);
		if (CurrentRegion)
			Draw_String2C(vid.width / 2, 8, CurrentRegion->text, 2);
		Draw_String2C(vid.width / 2, vid.height-8, va("3Cash: %i", SIp.cash), 2);
		return;
	}

	if (SIp.health > 0)
	{
		Draw_Colour4f(1, 1, 1,				1);
		/*		

		was a flame trail. :(
		grDisable(GL_TEXTURE_2D);
		grShadeModel(GL_SMOOTH);
		grBegin(GL_TRIANGLES);

		x = SIp.x+SIp.width/2;
		y = SIp.y+SIp.height;

		i = 12+rand()%8;

		grColor4f(0, 0, 1, 1);
		grVertex2f(x, y);
		grColor4f(1, 0, 1, 0);
		grVertex2f(x, y-i/2);
		grVertex2f(x+i, y);

		grColor4f(0, 0, 1, 1);
		grVertex2f(x, y);
		grColor4f(1, 0, 1, 0);
		grVertex2f(x+i, y);
		grVertex2f(x, y+i*2);

		grColor4f(0, 0, 1, 1);
		grVertex2f(x, y);
		grColor4f(1, 0, 1, 0);
		grVertex2f(x-i, y);
		grVertex2f(x, y+i*2);

		grColor4f(0, 0, 0, 1);
		grVertex2f(x, y);
		grColor4f(1, 0, 1, 0);
		grVertex2f(x, y-i/2);
		grVertex2f(x-i, y);

		/*
		grColor4f(1, 1, 1, 0);
		grVertex2f(x+8, y);
		grColor4f(1, 1, 1,				1);
		grVertex2f(x, y);
		grVertex2f(x, 0);
		grColor4f(1, 1, 1, 0);
		grVertex2f(x+8, 0);

		grColor4f(1, 1, 1,				1);
		grVertex2f(x, y);
		grColor4f(1, 1, 1, 0);
		grVertex2f(x-8, y);
		grVertex2f(x-8, 0);
		grColor4f(1, 1, 1,				1);
		grVertex2f(x, 0);
		

		grColor4f(1, 1, 1, 0);
		grVertex2f(x+8, y);
		grColor4f(1, 1, 1,				1);
		grVertex2f(x, y);
		grColor4f(1, 1, 1, 0);
		grVertex2f(x, y+8);
		grVertex2f(x+8, y+8);

		grVertex2f(x, y+8);
		grColor4f(1, 1, 1,				1);
		grVertex2f(x, y);
		grColor4f(1, 1, 1, 0);
		grVertex2f(x-8, y);
		grVertex2f(x-8, y+8);
		*/
/*
		grEnd();
		Draw_Colour4f(1, 1, 1, 1);
*/
		Draw_Picture(SIplayertexture, SIp.x, SIp.y, SIp.width, SIp.height);
		for (a = 0; a < MAXADDONS; a++)
		{
			if (SIp.cannon[a].power)
			{
				if (SIp.cannon[a].tex)
					Draw_Picture(*SIp.cannon[a].tex, SIp.x + SIp.width*SIp.cannon[a].xoff, SIp.y + SIp.width*SIp.cannon[a].yoff, SIp.width, SIp.height);
			}
		}
	}

	for (a = 0; a < MAXMONSTERS; a++)
	{
		if (SImonster[a].health > 0)
		{
			Draw_Picture(*SImonster[a].EnemyType->picture, SImonster[a].xpos, SImonster[a].ypos, SImonster[a].width, SImonster[a].height);
		}
	}

	//grEnable(GL_BLEND);
	for (a = 0; a < MAXBULLETS; a++)
	{
		if (SIbullet[a].inuse)
		{
			switch (SIbullet[a].type)
			{
			case 1:
				for (b = 0; b < SIbullet[a].charge; b++)
				{
					Draw_Colour4f(random(), random(), random(), SIbullet[a].alpha);
					Draw_Line (SIbullet[a].xpos+SIbullet[a].width/2,
						SIbullet[a].ypos+SIbullet[a].height/2,
						SIbullet[a].xpos+SIbullet[a].width/2+(float)sin((frame+2)*(a+1)*(b+1))*SIbullet[a].width,
						SIbullet[a].ypos+SIbullet[a].height/2+(float)cos((frame+2)*(a+1)*(b+1))*SIbullet[a].height);
				}
				break;

			case 2:
				x=realtime*50;

				Draw_Colour4f(1, 1, 1, SIbullet[a].alpha);
				Draw_Picture(*SIbullet[a].tex, SIbullet[a].xpos, 0, SIbullet[a].width, SIbullet[a].ypos+SIbullet[a].height);

				break;

/*			case 3:
				x=realtime*50;
				grColor4f(1, 1, 1, SIbullet[a].alpha);

				grDisable(GL_TEXTURE_2D);
				grBegin(GL_LINE_STRIP);
//				grVertex2f(SIbullet[a].xpos, SIbullet[a].ypos);
				for (y = SIbullet[a].ypos; y>0; y-=SIbullet[a].yvel*5+0.2f,x+=1)
					grVertex2f((float)SIbullet[a].xpos+(float)SIbullet[a].charge*(float)sin(x), y);
				
//				for (y = SIbullet[a].ypos; y>0; y-=25,x+=1)
//					grVertex2f((float)SIbullet[a].xpos+(float)SIbullet[a].charge*(float)sin(x), y);				

//				for (y = SIbullet[a].ypos; y>0; y-=25,x+=1)
//					grVertex2f((float)SIbullet[a].xpos+(float)SIbullet[a].charge*(float)sin(x), y);
				grEnd();
				grEnable(GL_TEXTURE_2D);
				break;
*/
			default:
				Draw_Colour4f(1, 1, 1, SIbullet[a].alpha);
				Draw_Picture(*SIbullet[a].tex, SIbullet[a].xpos, SIbullet[a].ypos, SIbullet[a].width, SIbullet[a].height);
				break;
			}
		}
	}
	Draw_Colour4f(1, 1, 1, 1);

	if (SIp.health <= 0)
		Draw_String2C(vid.width / 2, vid.height / 2, "4You LOOSE!", 2);
	else if (livingmonsters == 0)
	{
		Draw_String2C(vid.width / 2, vid.height / 2, "4You WIN!", 2);
		if (SIp.y + SIp.height < 0)
			Draw_String2C(vid.width / 2, (vid.height / 4) * 3, "3Press space!", 2);
	}
	else if (levelnametime)
	{
		Draw_Colour4f(1, 1, 1, (float)levelnametime/500);
		levelnametime--;
		Draw_String2C(vid.width/2, vid.height/2, SILevelName, 2);
		Draw_Colour4f(1, 1, 1, 1);
	}

	Draw_String2C(vid.width / 2, 4, va("3%i", SIp.cash), 1);
	if (SIp.health < 4)
		Draw_String2C(8, 8, va("4%i", SIp.health), 2);
	else
		Draw_String2C(8, 8, va("3%i", SIp.health), 2);
}

void SI_LoadTextures (void)
{
	int a;
	for (a = 0; a < monsterpics; a++)
		SIbaddietexture[a]	= Draw_LoadImage(va("spaceinv/enemy%i", a+1), false);
	for (a = 0; a < weaponlevels;a++)
		SIbullettexture[a]	= Draw_LoadImage(va("spaceinv/gun%i", a+1), false);
	SIplayertexture		= Draw_LoadImage("spaceinv/siplayer", false);
	SIexplosiontexture	= Draw_LoadImage("spaceinv/bang", false);
	SIhealthtexture		= Draw_LoadImage("spaceinv/health", false);
	SIleaveshoptexture	= Draw_LoadImage("spaceinv/leaveshop", false);
	SIshrinktexture		= Draw_LoadImage("spaceinv/shrink",  false);
	SIcannontexture		= Draw_LoadImage("spaceinv/cannon", false);
	SIquittexture		= Draw_LoadImage("spaceinv/quit", false);
	SIsavegametexture	= Draw_LoadImage("spaceinv/save", false);
	SIloadgametexture	= Draw_LoadImage("spaceinv/load", false);	
	SIsideshottexture	= Draw_LoadImage("spaceinv/sideshot", false);
	SIrapidtexture		= Draw_LoadImage("spaceinv/rapid", false);	
}

//Add side cannons?
bool SIHitPlayer(float x, float y, float width, float height)
{		
	if (x + width > SIp.x && x < SIp.x + SIp.width && y + height > SIp.y && y < SIp.y + SIp.height)
		return true;
	else
		return false;
}

SImonster_t *SIHitEnemy(float x, float y, float width, float height)
{
	int a;
	for (a = 0; a < MAXMONSTERS; a++)
	{
		if (SImonster[a].health)
		{
			if (x + width > SImonster[a].xpos && x < SImonster[a].xpos + SImonster[a].width && y + height > SImonster[a].ypos && y < SImonster[a].ypos + SImonster[a].height)
				return &SImonster[a];
		}
	}
	return NULL;
}

void SIKillEnemy(SImonster_t *en)
{
	en->health = 0;
	livingmonsters--;

	SI_NewExplosion(en->xpos + en->width/2, en->ypos+ en->height/2, (en->width + en->height)/2);

	SIp.cash += en->EnemyType->reward;

	Sound_Start(SoundExplosion);

	if (livingmonsters <= 0)
		Sound_Start(SoundWin);
}

void SIHurtPlayer(int dam)
{
	if (SIp.health <= 0)	//already dead
		return;
	SIp.health -= dam;
	if (SIp.health < 0)
		SIp.health = 0;	//no negative health
	if (SIp.health <= 0)	//if final blow...
	{
		SI_NewExplosion(SIp.x + SIp.width/2, SIp.y + SIp.height/2, (SIp.width + SIp.height));
		Sound_Start(SoundLoose);

		deadtime = realtime + 4.0f;
	}
	else
		Sound_Start(SoundHit);
}

float nextthink;

void SI_MouseMove(int x, int y);
void SI_Main(int mousex, int mousey)
{
	int a;
	int i;
	int tm;
	float tmdist;

	SIbullet_t *bul;
	SImonster_t *en;

	vec3_t v;

	if (nextthink > realtime)
	{
//		Con_Printf("no time\n");
		return;
	}
//	Con_Printf("time\n");
	nextthink += 0.02f;

	if (nextthink < realtime-1)	//don't run more than a second behind
		nextthink = realtime-1;

	if (SIShop)
	{		
		if (lastlevel)
		{
//			Con_Printf("Lastlevel\n");
			return;
		}

//		Con_Printf("%i (%f %f)\n", SIp.contols, SIp.x, SIp.y);

		if (!SIp.health)
			SIp.health=1;

		if (SIp.contols & CONT_LEFTKEY)
			SIp.x -= PLAYERSHIPSPEEDX;
		if (SIp.contols & CONT_RIGHTKEY)
			SIp.x += PLAYERSHIPSPEEDX;

		if (SIp.x < 0)
			SIp.x = 0;
		if (SIp.x + SIp.width >= vid.width)
			SIp.x = vid.width - SIp.width;

		if (SIp.contols & CONT_DOWNKEY)
			SIp.y += PLAYERSHIPSPEEDY;
		if (SIp.contols & CONT_UPKEY)
			SIp.y -= PLAYERSHIPSPEEDY;

		if (SIp.y < 0)
			SIp.y = 0;
		if (SIp.y + SIp.height >= vid.height)
			SIp.y = vid.height - SIp.height;

		CurrentRegion = NULL;
		for (a = 0; a < sizeof(ShopRegion) / sizeof(ShopRegion_t); a++)
		{			
			if (SIp.x+SIp.width/2 < ShopRegion[a].x + ShopRegion[a].width && SIp.x+SIp.width/2 > ShopRegion[a].x &&
				SIp.y+SIp.height/2 < ShopRegion[a].y + ShopRegion[a].height && SIp.y+SIp.height/2 > ShopRegion[a].y)
			{
				if ((*ShopRegion[a].AppearCondition) (ShopRegion[a].ident))
				{
					CurrentRegion = &ShopRegion[a];				
					break;
				}
			}
		}

		if (SIp.contols & CONT_FIREKEY && CurrentRegion)
		{
			(*CurrentRegion->CallFunc) (CurrentRegion->ident);
			SIp.contols &= ~CONT_FIREKEY;
		}		

		return;
	}

	leveltime += 0.02f;


	if (SIp.health > 0)
	{
		if (SIp.contols & CONT_LEFTKEY || (SIp.contols & CONT_MOUSE && SIp.x+SIp.width/2 > mousex))
			SIp.x -= PLAYERSHIPSPEEDX;	
		if (SIp.contols & CONT_RIGHTKEY || (SIp.contols & CONT_MOUSE && SIp.x+SIp.width/2 < mousex))
			SIp.x += PLAYERSHIPSPEEDX;

		if (SIp.x < 0)
			SIp.x = 0;
		if (SIp.x + SIp.width >= vid.width)
			SIp.x = vid.width - SIp.width;

		if (livingmonsters > 0)
		{
			if (SIp.contols & CONT_DOWNKEY || (SIp.contols & CONT_MOUSE && SIp.y+SIp.height/2 < mousey))
				SIp.y += PLAYERSHIPSPEEDY;
			if (SIp.contols & CONT_UPKEY || (SIp.contols & CONT_MOUSE && SIp.y+SIp.height/2 > mousey))
				SIp.y -= PLAYERSHIPSPEEDY;

			if (SIp.y < (vid.height / 2))
				SIp.y = (float)(vid.height / 2);
			if (SIp.y + SIp.height >= vid.height)
				SIp.y = vid.height - SIp.height;
		}
		else
		{
			SIp.y -= PLAYERSHIPSPEEDY;
		}

		if (SIp.contols & CONT_FIREKEY)
		{		
			/*
  			if (bul = SI_NewBullet(SIp.weaponpower))
			{						

				Sound_Start(SoundFire);

				bul->ypos = SIp.y - bul->height;
				bul->xpos = SIp.x + (SIp.width - bul->width) / 2;
				bul->xvel = 0;

				SIp.refire = SIp.reloadtime;
				*/

#if 1
				for (a = FIRSTCANNON; a <= LASTCANNON; a++)
				{
					if (SIp.cannon[a].power && SIp.cannon[a].refire < leveltime)
					{						
						if (bul = SI_NewBullet(SIp.cannon[a].power - 1))
						{
							bul->ypos = SIp.y - bul->height;
							bul->xpos = SIp.x + SIp.width*SIp.cannon[a].xoff + (SIp.width - bul->width) / 2;		
							bul->xvel = 0;

							SIp.cannon[a].refire = leveltime + SIp.cannon[a].reloadtime;

							Sound_Start(SoundFire);
						}
					}	
				}

				for (a = FIRSTCENTRALMOD; a <= LASTCENTRALMOD; a++)
				{
					if (SIp.cannon[a].power && SIp.cannon[a].refire < leveltime)
					{						
						if (bul = SI_NewBullet(SIp.cannon[a].power - 1))
						{
							bul->ypos = SIp.y - bul->height;
							bul->xpos = SIp.x + SIp.width*SIp.cannon[a].xoff + (SIp.width - bul->width) / 2;		
							bul->xvel = 0;							

							SIp.cannon[a].refire = leveltime + SIp.cannon[a].reloadtime;

							Sound_Start(SoundFire);
						}
					}	
				}

				
				for (a = FIRSTROCKET; a <= LASTROCKET; a++)
				{
					if (SIp.cannon[a].power && SIp.cannon[a].refire < leveltime)
					{						
						if (bul = SI_NewBullet(-1))
						{
							bul->ypos = SIp.y - bul->height;
							bul->xpos = SIp.x + SIp.width*SIp.cannon[a].xoff + (SIp.width - bul->width) / 2;		
							bul->xvel = 0;
							bul->yvel = -1;

							SIp.cannon[a].refire = leveltime + SIp.cannon[a].reloadtime;

							Sound_Start(SoundFire);
						}
					}	
				}

#else
				if (SIp.powerups & PUP_CANNON)
				{
					bul = SI_NewBullet(SIp.weaponpower);
					if (bul)
					{
					bul->width = 10;
					bul->height = 10;
					bul->damage = 1;
					bul->ypos = SIp.y - bul->height;
					bul->xpos = SIp.x - SIp.width + (SIp.width - bul->width) / 2;
					bul->xvel = 0;
					bul->yvel = (float)0.004;
					bul->yaccel = (float)0.002;				
					}
				}
				if (SIp.powerups & PUP_CANNON2)
				{
					bul = SI_NewBullet(SIp.weaponpower);
					if (bul)
					{
						bul->width = 10;
						bul->height = 10;
						bul->damage = 1;
						bul->ypos = SIp.y - bul->height;
						bul->xpos = SIp.x + SIp.width + (SIp.width - bul->width) / 2;
						bul->xvel = 0;
						bul->yvel = (float)0.004;
						bul->yaccel = (float)0.002;
					}
				}
#endif
				/*
				if (SIp.sidegun)
				{					
					if (bul = SI_NewBullet(SIp.sidegun -1))
					{
						bul->ypos = SIp.y - bul->height;
						bul->xpos = SIp.x + (SIp.width - bul->width) / 2;
						bul->xvel = (float)(random()-0.5)*2;
					}
				}				
			}
			*/
		}
	}
	else
	{
		if (deadtime < realtime)
			SIRestartGame();
	}

	for (a = 0; a < MAXSQUADS; a++)
	{
		if (SISquad[a].number && SISquad[a].time < leveltime)
		{
			SISquad[a].time += SISquad[a].interval;

			for (i = 0; i < MAXMONSTERS; i++)
			{
				if (SImonster[i].health <= 0)
				{
					SImonster[i].routenum = SISquad[a].route;
					SImonster[i].destnum = 1;
					SImonster[i].width = SIEnemyType[SISquad[a].type].width;
					SImonster[i].height = SIEnemyType[SISquad[a].type].height;
					SImonster[i].xpos = ((SIRoute[SImonster[i].routenum].pos[0])[0] - SImonster[i].width/2)/100.f*vid.height;
					SImonster[i].ypos = ((SIRoute[SImonster[i].routenum].pos[0])[1] - SImonster[i].height/2)/100.f*vid.height;

					SImonster[i].EnemyType = &SIEnemyType[SISquad[a].type];
					SImonster[i].health = SIEnemyType[SISquad[a].type].health;
					SImonster[i].yvel = ENEMYSHIPSPEEDY;
					SImonster[i].xvel = (float)ENEMYSHIPSPEEDX;
					SISquad[a].number-=1;
					break;
				}
			}
		}
	}

	tm = -1;
	tmdist = 32767;
	for (a = 0; a < MAXMONSTERS; a++)
	{
		if (SImonster[a].health > 0)
		{			
			if (tmdist > sqrt((SIp.x - SImonster[a].xpos)*(SIp.x - SImonster[a].xpos)+(SIp.y - SImonster[a].ypos)*(SIp.y - SImonster[a].ypos)))
			{
				tm = a;
				tmdist = (float)sqrt((SIp.x - SImonster[a].xpos)*(SIp.x - SImonster[a].xpos)+(SIp.y - SImonster[a].ypos)*(SIp.y - SImonster[a].ypos));
			}
			if (SImonster[a].EnemyType->AI)
			{
				if (SImonster[a].xpos+SImonster[a].width/2 > SIp.x+SIp.width/2)
				{
					if (SImonster[a].xpos+SImonster[a].width/2 - 0.8 < SIp.x+SIp.width/2)
						SImonster[a].xvel = SIp.x+SIp.width/2-(SImonster[a].xpos+SImonster[a].width/2);
					else
						SImonster[a].xvel = (float)-0.8;
				}
				else if (SImonster[a].xpos < SIp.x)
				{					
					if (SImonster[a].xpos+SImonster[a].width/2 + 0.8 > SIp.x+SIp.width/2)
						SImonster[a].xvel = SIp.x+SIp.width/2-(SImonster[a].xpos+SImonster[a].width/2);
					else
						SImonster[a].xvel = (float)0.8;
				}

				SImonster[a].ypos += SImonster[a].yvel;
				SImonster[a].xpos += SImonster[a].xvel;
				SImonster[a].yvel = 0;

				if (SImonster[a].xpos < 0)
					SImonster[a].xpos = 0;
				if (SImonster[a].xpos + SImonster[a].width >= vid.width)
					SImonster[a].xpos = vid.width - SImonster[a].width;

				if (SIHitPlayer(SImonster[a].xpos, SImonster[a].ypos, SImonster[a].width, SImonster[a].height) && SIp.health > 0)
				{				
					SIHurtPlayer(SImonster[a].health);
					SIKillEnemy(&SImonster[a]);				
				}
				else if (random() < SImonster[a].EnemyType->FireProb && SIp.health > 0 && SImonster[a].EnemyType->shotdamage >= 0)
				{
					if (bul = SI_NewBullet(SImonster[a].EnemyType->shotdamage))
					{
						bul->yvel *= -1;
						bul->yvel -= SImonster[a].yvel;
						bul->yaccel *= -1;
						bul->xpos = SImonster[a].xpos + SImonster[a].width/2-bul->width/2;
						bul->ypos = SImonster[a].ypos + SImonster[a].height;
	//					bul->width = 10;
	//					bul->height = 10;
					}
				}
			}
			else
			{
				SImonster[a].ypos += SImonster[a].yvel;
				SImonster[a].xpos += SImonster[a].xvel;

				if (SImonster[a].xpos < 0)
					SImonster[a].xpos = 0;
				if (SImonster[a].xpos + SImonster[a].width >= vid.width)
					SImonster[a].xpos = vid.width - SImonster[a].width;

				if (SIHitPlayer(SImonster[a].xpos, SImonster[a].ypos, SImonster[a].width, SImonster[a].height) && SIp.health > 0)
				{				
					SIHurtPlayer(SImonster[a].health);
					SIKillEnemy(&SImonster[a]);				
				}
				else if (SImonster[a].ypos > vid.height)
				{
					SImonster[a].ypos = 0 - SImonster[a].height;
					SImonster[a].xpos = random() * vid.width;
				}
				else if (random() < SImonster[a].EnemyType->FireProb && SIp.health > 0 && SImonster[a].EnemyType->shotdamage >= 0)
				{
					if (bul = SI_NewBullet(SImonster[a].EnemyType->shotdamage))
					{
						bul->yvel *= -1;
						bul->yvel -= SImonster[a].yvel;
						bul->yaccel *= -1;
						bul->xpos = SImonster[a].xpos + SImonster[a].width/2-bul->width/2;
						bul->ypos = SImonster[a].ypos + SImonster[a].height;
	//					bul->width = 10;
	//					bul->height = 10;
					}
				}
				else if (random() < 0.001)
					SImonster[a].xvel = (float)ENEMYSHIPSPEEDX;
			}
			if (SIEnemyType[a].healthregen)
			{
				if (SImonster[a].health < SImonster[a].health)
				{
					if (SIEnemyType[a].healthregen)
						SIEnemyType[a].healthregen--;
					else
					{
						SIEnemyType[a].healthregen = SImonster[a].timetoregen;
						SIEnemyType[a].health++;
					}
				}
			}
		}	
	}

	for (a = 0; a < MAXBULLETS; a++)
	{
		if (SIbullet[a].inuse)
		{
			if (SIbullet[a].type == -2)
			{
				SIbullet[a].ypos += SIbullet[a].yvel;
				SIbullet[a].xpos += SIbullet[a].xvel;

				SIbullet[a].alpha += SIbullet[a].alphachange;
				if (SIbullet[a].alpha >= 1 && SIbullet[a].alphachange > 0)
					SIbullet[a].alphachange *= -1;
				else if (SIbullet[a].alphachange && SIbullet[a].alpha <= 0)
					SIbullet[a].inuse = false;
			}
			else if (SIbullet[a].type == -1)
			{
				if (tm >= 0)
				{
					v[0] = SImonster[tm].xpos+SImonster[tm].width/2 - SIbullet[a].xpos;
					v[1] = SImonster[tm].ypos+SImonster[tm].height/2 - SIbullet[a].ypos;
					v[2] = 0;
					vecNorm(v, v);

					if (bul = SI_NewBullet(-2))
					{
						bul->xvel = -v[0]*3;
						bul->yvel = -v[1]*3;
						bul->alphachange = 0.05f;
						bul->xpos = SIbullet[a].xpos;
						bul->ypos = SIbullet[a].ypos;
					}

					v[0] += SIbullet[a].xvel;
					v[1] += SIbullet[a].yvel;

					vecNorm(v, v);
					SIbullet[a].xvel = v[0]*5;
					SIbullet[a].yvel = v[1]*5;

				}
				SIbullet[a].ypos += SIbullet[a].yvel;
				SIbullet[a].xpos += SIbullet[a].xvel;

				if (en = SIHitEnemy(SIbullet[a].xpos, SIbullet[a].ypos, SIbullet[a].width, SIbullet[a].height))
				{
					en->health -= SIbullet[a].damage;
					if (en->health <= 0)
					{
						SIKillEnemy(en);
					}
					else
					{
						Sound_Start(SoundHit);
					}

					SIbullet[a].alpha = 0.5f;
					SIbullet[a].alphachange = 0.04f;
					SIbullet[a].tex = &SIexplosiontexture;
					SIbullet[a].xvel = 0;
					SIbullet[a].yvel = 0;
					SIbullet[a].damage = 0;
					SIbullet[a].yaccel = 0;
					SIbullet[a].charge = 0;
					SIbullet[a].type = 0;
				}

				if (SIbullet[a].ypos + SIbullet[a].height < 0 || SIbullet[a].ypos > vid.height)
					SIbullet[a].inuse = false;
			}
			else if (SIbullet[a].ypos + SIbullet[a].height < 0 || SIbullet[a].ypos > vid.height)
			{
				SIbullet[a].inuse = false;
			}
			else
			{
				SIbullet[a].yvel += SIbullet[a].yaccel;
				SIbullet[a].ypos -= SIbullet[a].yvel;
				SIbullet[a].xpos += SIbullet[a].xvel;

				if (SIbullet[a].type == 2)
				{
					if (en = SIHitEnemy(SIbullet[a].xpos, 0, SIbullet[a].width, SIbullet[a].ypos))
					{
						en->health -= SIbullet[a].damage;
						if (en->health <= 0)
						{
							SIKillEnemy(en);
						}
						else
						{
							Sound_Start(SoundHit);
						}

//						SIbullet[a].alpha = 0.5f;
//						SIbullet[a].alphachange = 0.04f;
//						SIbullet[a].tex = &SIexplosiontexture;
//						SIbullet[a].xvel = 0;
//						SIbullet[a].yvel = 0;
//						SIbullet[a].damage = 0;
//						SIbullet[a].yaccel = 0;
//						SIbullet[a].charge = 0;
//						SIbullet[a].type = 0;
					}
				}
				else if (SIbullet[a].yvel > 0)
				{
					if (en = SIHitEnemy(SIbullet[a].xpos, SIbullet[a].ypos, SIbullet[a].width, SIbullet[a].height))
					{
						en->health -= SIbullet[a].damage;
						if (en->health <= 0)
						{
							SIKillEnemy(en);
						}
						else
						{
							Sound_Start(SoundHit);
						}

						SIbullet[a].alpha = 0.5f;
						SIbullet[a].alphachange = 0.04f;
						SIbullet[a].tex = &SIexplosiontexture;
						SIbullet[a].xvel = 0;
						SIbullet[a].yvel = 0;
						SIbullet[a].damage = 0;
						SIbullet[a].yaccel = 0;
						SIbullet[a].charge = 0;
						SIbullet[a].type = 0;
					}
				}
				else if (SIbullet[a].yvel < 0)
				{
					if (SIHitPlayer(SIbullet[a].xpos, SIbullet[a].ypos, SIbullet[a].width, SIbullet[a].height))
					{
						SIHurtPlayer(SIbullet[a].damage);
						SIbullet[a].alpha = 0.5f;
						SIbullet[a].alphachange = 0.04f;
						SIbullet[a].tex = &SIexplosiontexture;
						SIbullet[a].xvel = 0;
						SIbullet[a].yvel = 0;
						SIbullet[a].damage = 0;
						SIbullet[a].yaccel = 0;
						SIbullet[a].charge = 0;
						SIbullet[a].type = 0;
					}
				}

				SIbullet[a].alpha += SIbullet[a].alphachange;
				if (SIbullet[a].alpha >= 1 && SIbullet[a].alphachange > 0)
					SIbullet[a].alphachange *= -1;
				else if (SIbullet[a].alphachange && SIbullet[a].alpha <= 0)
					SIbullet[a].inuse = false;
			}
		}
	}
}

void SI_KeyUp(int k)
{
	if (k == K_LEFTARROW)
		SIp.contols &= ~CONT_LEFTKEY;
	else if (k == K_RIGHTARROW)
		SIp.contols &= ~CONT_RIGHTKEY;
	else if (k == K_UPARROW)
		SIp.contols &= ~CONT_UPKEY;
	else if (k == K_DOWNARROW)
		SIp.contols &= ~CONT_DOWNKEY;
	else if (k == K_MOUSE1 || k == K_SPACE)
		SIp.contols &= ~CONT_FIREKEY;
	else if (k == K_MOUSE2)
		SIp.contols &= ~CONT_MOUSE;
}
void SI_KeyDown(int k)
{
	if (SIShop && lastlevel)
	{
		SIlevel = 0;
		lastlevel = false;
		return;
	}
	else if (livingmonsters <= 0 && SIp.y + SIp.height < 0 && SIShop == false)
	{
		SIShop = true;
		SIlevel+=1;
		SIp.x = ShopRegion[0].x + ShopRegion[0].width/2;
		SIp.y = ShopRegion[0].y + ShopRegion[0].height/2;
		return;
	}
	else if (SIp.health <= 0)
	{
		if (k == K_SPACE && !(SIp.contols & CONT_FIREKEY))
			deadtime += 1;
		return;
	}

	if (k == K_LEFTARROW)
		SIp.contols |= CONT_LEFTKEY;
	else if (k == K_RIGHTARROW)
		SIp.contols |= CONT_RIGHTKEY;
	else if (k == K_UPARROW)
		SIp.contols |= CONT_UPKEY;	
	else if (k == K_DOWNARROW)
		SIp.contols |= CONT_DOWNKEY;
	else if (k == K_MOUSE1 || k == K_SPACE)
		SIp.contols |= CONT_FIREKEY;
	else if (k == K_MOUSE2)
		SIp.contols |= CONT_MOUSE;
	else if (k == '1')
		SIp.health = 9;
	else if (k == '2')
		SIp.cash += 100;
	else if (k == K_ESCAPE)
		Menu_Control(0);
	else
		Con_Printf("key %i not recognised\n", k);

	Con_Printf("%i (%i)\n", SIp.contols, k);
}

void SI_MouseMove(int x, int y)
{
	static int ox;
	static int oy;
	if (SIShop && (ox != x || oy != y))
	{
		SIp.x = (float)x - SIp.width/2;
		SIp.y = (float)y - SIp.height/2;

		ox = x;
		oy = y;
	}
}

qboolean FS_GetS(char *buffer, int buffersize, qhandle_t handle)
{
	int i;
	buffersize--;
	for (i = 0; i < buffersize; i+=1)
	{
		if (FS_Read(handle, buffer+i, 1) <= 0) break;
		if (buffer[i] == '\n') break;
	}

	buffer[i] = '\0';

	if (!i)
		return false;
	return true;
}

void SI_Initialize(void)
{
	int a;

/*	SoundWin = Sound_LoadSound("spaceinv/sound/win.wav");
	SoundLoose = Sound_LoadSound("spaceinv/sound/loose.wav");
	SoundFire = Sound_LoadSound("spaceinv/sound/fire.wav");
	SoundExplosion = Sound_LoadSound("spaceinv/sound/explode.wav");
	SoundHit = Sound_LoadSound("spaceinv/sound/explode.wav");
*/
	if (SIEnemyType)	//if one of these is defined, they must all be
	{
		free(SIEnemyType);
		free(SImonster);
		free(SIbullet);		

		free(SIbaddietexture);
		free(SIbullettexture);
		SIbullettexture = NULL;

		free(SISquad);
		free(SIRoute);
	}
	if (SIweapons)
		free(SIweapons);

	{	
		bool section = false;
	//	FILE *F;
		char line[1024];
		char *s;
		char *val;
		int len;

		qhandle_t f;

		len = FS_Open("spaceinv/weapons.txt", &f, 1);
		if (len>=0)
		{
			a = -1;		
			while (1)
			{
				if (!FS_GetS(line, 1024, f))
					break;
	//			if (!fgets(line, 1024, F))
	//				break;	//eof

				s = line;
				while(*s == ' ' || *s == '\t')	//ignore indents
					s++;
				
				if (*s == '\n' || *s == '\r' || *s == '#')	//ignore comments/blank lines
					continue;

				val = s + strlen(s)-1;
				while (*val == '\n' || *val == '\r' || *val == ' ' || *val == '\t')
				{
					*val = 0;
					val--;
				}


				val = strchr(s, '=');
				if (val)
				{
					*val = 0;
					val++;
				}

				if (section == 2 && *s != '[')
				{
					if (!strcasecmp(s, "monsterpics"))
						monsterpics = atoi(val);
					else if (!strcasecmp(s, "maxmonsters"))
						MAXMONSTERS = atoi(val);
					else if (!strcasecmp(s, "maxbullets"))
						MAXBULLETS = atoi(val);
					else if (!strcasecmp(s, "maxroutepoints"))
						MAXROUTEPOINTS = atoi(val);
					else if (!strcasecmp(s, "maxroutes"))
						MAXROUTES = atoi(val);
					else if (!strcasecmp(s, "maxsquads"))
						MAXSQUADS = atoi(val);
					else
						Con_Printf("Bad command \"%s\" in \"%s\"\n", s, "spaceinv\\weapons.txt");
				}
				else if (section == 1 && *s != '[')
				{
					if (!strcasecmp(s, "width"))
						SIweapons[a].width = (float)atof(val);
					else if (!strcasecmp(s, "height"))
						SIweapons[a].height = (float)atof(val);
					else if (!strcasecmp(s, "yvel"))
						SIweapons[a].yvel = (float)atof(val);
					else if (!strcasecmp(s, "yaccel"))
						SIweapons[a].yaccel = (float)atof(val);
					else if (!strcasecmp(s, "damage"))
						SIweapons[a].damage = atoi(val);
					else if (!strcasecmp(s, "alpha"))
						SIweapons[a].alpha = (float)atof(val);
					else if (!strcasecmp(s, "alphachange"))
						SIweapons[a].alphachange = (float)atof(val);
					else if (!strcasecmp(s, "texturenum"))
					{
						if (!SIbullettexture)
						{
							SIbullettexture = malloc(sizeof(texture) * weaponlevels);
							memset(SIbullettexture, 0, sizeof(texture) * weaponlevels);
						}
						SIweapons[a].tex = &SIbullettexture[atoi(val)-1];	//allow different pictures
					}
					else if (!strcasecmp(s, "charge"))
						SIweapons[a].charge = atoi(val);	//allow different pictures
					else if (!strcasecmp(s, "type"))
						SIweapons[a].type = atoi(val);
					else
						Con_Printf("Bad command \"%s\" in \"%s\"\n", s, "spaceinv\\weapons.txt");
				}
				else if (!strcasecmp(s, "weapons"))			
				{
					weaponlevels = atoi(val);
	//				if (weaponlevels > SHOTLEVELS)
	//				{
	//					Log("Too many weapons set\n");
	//					weaponlevels = SHOTLEVELS;
	//				}

					SIweapons = malloc(sizeof(SIbullet_t) * weaponlevels);
					memset(SIweapons, 0, sizeof(SIbullet_t) * weaponlevels);
				}
				else if (!strcasecmp(s, "[weapon"))
				{
					section = 1;
					if (weaponlevels == 0)
						Sys_Error("SILoadWeapons: number of weapons not set yet.");
					a = atoi(val);
					if (a > SHOTLEVELS)
					{
						a=0;
						Con_Printf("Bad type\n");
					}
					else
						a-=1;
				}
				else if (!strcasecmp(s, "[info]"))
				{
					section = 2;
				}
				else
					Con_Printf("Bad command \"%s\" in \"%s\"\n", s, "spaceinv\\weapons.txt");

			}

			FS_Close(f);
		}
		else
		{
			//GameCompleate = true;
			//NextLevel = 0;
			Sys_Error("Couldn't open file\n");
		}
	}

	if (!SIbullettexture)
	{
		SIbullettexture = malloc(sizeof(texture) * weaponlevels);
		memset(SIbullettexture, 0, sizeof(texture) * weaponlevels);
	}

	SIEnemyType = malloc(sizeof(SIEnemyType_t) * ENEMYTYPES);
	memset(SIEnemyType, 0, sizeof(SIEnemyType_t) * ENEMYTYPES);
	SImonster = malloc(sizeof(SImonster_t) * MAXMONSTERS);
	memset(SImonster, 0, sizeof(SImonster_t) * MAXMONSTERS);
	SIbullet = malloc(sizeof(SIbullet_t) * MAXBULLETS);
	memset(SIbullet, 0, sizeof(SIbullet_t) * MAXBULLETS);

	SIbaddietexture = malloc(sizeof(texture) * ENEMYTYPES);
	memset(SIbaddietexture, 0, sizeof(texture) * ENEMYTYPES);

	SIRoute = malloc((sizeof(SIRoute_t)+(MAXROUTEPOINTS-1)*sizeof(vec3_t)) * MAXROUTES);
	memset(SIRoute, 0, (sizeof(SIRoute_t)+(MAXROUTEPOINTS-1)*sizeof(vec3_t)) * MAXROUTES);
	SISquad = malloc(sizeof(SISquad_t) * MAXSQUADS);
	memset(SISquad, 0, sizeof(SISquad_t) * MAXSQUADS);

	//loaded elsewhere
//	SIweapons = mmalloc(sizeof(SIbullet_t) * weaponlevels);

	SI_LoadTextures ();
 
	SIRestartGame ();
}

void NextLevel (void)
{
	int a;
//	int numenemysoftype[ENEMYTYPES] = {5, 5, 50};
	int etype = 0;

	SIEnemyType_t *et;

 	if (SIp.powerups & PUP_SHRUNKEN)
	{
		SIp.height = 16;
		SIp.width = 16;

		SIp.powerups &= ~PUP_SHRUNKEN;
	}
	else
	{
		SIp.height = 32;
		SIp.width = 32;
	}
	SIp.y = vid.height - SIp.height;
	SIp.x = (vid.width - SIp.width) / 2;	
	SIShop = false;
	deadtime = 0;

	nextthink = realtime;


	leveltime = 0;

	for (a = 0; a < MAXSQUADS; a++)
		SISquad[a].number = 0;

#if 1
{
	int i;
	int usedtypes = false;	
	int sectiontype=0;
	qhandle_t F;
	char line[1024];
	char *s;
	char *val;

	int len;
	int rofs = 0;

	len = FS_Open(va("spaceinv/level%i.txt", SIlevel+1), &F, 1);
	if (len < 0)
	{
		SIlevel = 0;
		len = FS_Open(va("spaceinv/level%i.txt", SIlevel+1), &F, 1);	//loop back to the start
	}
//	F = fopen(Sva("%sspaceinv\\level%i.txt", funcs->exe->gamepath, SIlevel+1), "rb");
	if (len>=0)
	{
		a = -1;
		memset(SIEnemyType, 0, sizeof(SIEnemyType));
//		memset(numenemysoftype, 0, sizeof(numenemysoftype));
		while (1)
		{
			if (!FS_GetS(line, 1024, F))
				break;	//eof

			s = line;
			while(*s == ' ' || *s == '\t')	//ignore indents
				s++;
			
			if (*s == '\n' || *s == '\r' || *s == '#')	//ignore comments/blank lines
				continue;

			val = s + strlen(s)-1;
			while (*val == '\n' || *val == '\r' || *val == ' ' || *val == '\t')
			{
				*val = 0;
				val--;
			}


			val = strchr(s, '=');
			if (val)
			{
				*val = 0;
				val++;
			}

			if (*s == '[')
			{
				if (!strcasecmp(s+1, "info]"))
				{
					sectiontype = 1;
				}
				else if (!strcasecmp(s+1, "type"))
				{					
					if (usedtypes == 0)
						Con_Printf("SINextLevel: Types not set yet");
					a = atoi(val);
					if (a > ENEMYTYPES || a < 1)
					{
						a=0;
						Con_Printf("Bad type\n");
					}
					else
						a-=1;

					memset(&SIEnemyType[a], 0, sizeof(SIEnemyType_t));

					sectiontype = 2;				
				}
				else if (!strcasecmp(s+1, "route"))
				{					
					a = atoi(val);
					if (a > ENEMYTYPES || a < 1)
					{
						a=0;
						Con_Printf("Bad route\n");
					}
					else
						a-=1;

					memset(&SIRoute[a], 0, sizeof(SIRoute_t)+(MAXROUTEPOINTS-1)*sizeof(vec3_t));

					sectiontype = 3;				
				}
				else if (!strcasecmp(s+1, "squad"))
				{					
					a = atoi(val);
					if (a > MAXSQUADS || a < 1)
					{
						a=0;
						Con_Printf("Bad squad\n");
					}
					else
						a-=1;

					memset(&SISquad[a], 0, sizeof(SISquad_t));

					sectiontype = 4;				
				}
				else
					Con_Printf("Bad block \"%s\" in \"%s\"\n", s, va("level%i.txt", SIlevel));
			}
			else if (sectiontype == 0)
			{
				if (!strcasecmp(s, "types"))			
					usedtypes = atoi(val);
				else
					Con_Printf("Bad command \"%s\" in \"%s\" (out of block)\n", s, va("level%i.txt", SIlevel));
			}
			else if (sectiontype == 1)
			{
				if (!strcasecmp(s, "lastlevel"))
					lastlevel = atoi(val);
				else if (!strcasecmp(s, "levelname"))
				{
					strcpy(SILevelName, val);
					levelnametime = 500;
				}
				else
					Con_Printf("Bad command \"%s\" in \"%s\"\n", s, va("level%i.txt", SIlevel));
			}			
			else if (sectiontype == 2)
			{
				if (!strcasecmp(s, "width"))
					SIEnemyType[a].width = (float)atof(val);
				else if (!strcasecmp(s, "height"))
					SIEnemyType[a].height = (float)atof(val);
				else if (!strcasecmp(s, "power"))
				{					
					SIEnemyType[a].shotdamage = atoi(val) - 1;
					if (SIEnemyType[a].shotdamage >= weaponlevels)
					{
						Con_Printf("Enemy %i has weapon number %i\n", a+1, SIEnemyType[a].shotdamage+1);
						SIEnemyType[a].shotdamage = weaponlevels-1;
					}
				}
				else if (!strcasecmp(s, "health"))
					SIEnemyType[a].health = atoi(val);
				else if (!strcasecmp(s, "reward"))
					SIEnemyType[a].reward = atoi(val);
				else if (!strcasecmp(s, "number"))
					SIEnemyType[a].number = atoi(val);
				else if (!strcasecmp(s, "ai"))
					SIEnemyType[a].AI = atoi(val);
				else if (!strcasecmp(s, "healthregen"))
					SIEnemyType[a].healthregen = atoi(val);
				else if (!strcasecmp(s, "picture"))
				{
					i = atoi(val) - 1;
					if (i < 0 || i >= monsterpics)
					{
						Sys_Errorf("Monster picture has the wrong value (%i)", i+1);
					}
					else
						SIEnemyType[a].picture = &SIbaddietexture[i];	//allow different pictures
				}
				else if (!strcasecmp(s, "fireprob"))
					SIEnemyType[a].FireProb = (float)atof(val);	//allow different pictures				
				else
					Con_Printf("Bad command \"%s\" in \"%s\"\n", s, va("level%i.txt", SIlevel));
			}
			else if (sectiontype == 3)
			{
				if (!strcasecmp(s, "coords"))
					SIRoute[a].numpoints = atoi(val);
				else if (*s == 'x' || *s == 'X')
					(SIRoute[a].pos[atoi(s+1)-1])[0] = (float)atof(val);
				else if (*s == 'y' || *s == 'Y')
					(SIRoute[a].pos[atoi(s+1)-1])[1] = (float)atof(val);
				else if (*s == 'z' || *s == 'Z')
					(SIRoute[a].pos[atoi(s+1)-1])[2] = (float)atof(val);
				else
					Con_Printf("Bad route command \"%s\" in \"%s\"\n", s, va("level%i.txt", SIlevel));
			}
			else if (sectiontype == 4)
			{
				if (!strcasecmp(s, "route"))
					SISquad[a].route = atoi(val)-1;
				else if (!strcasecmp(s, "type"))
					SISquad[a].type = atoi(val)-1;
				else if (!strcasecmp(s, "number"))
					SISquad[a].number = atoi(val);
				else if (!strcasecmp(s, "interval"))
					SISquad[a].interval = (float)atof(val);
				else if (!strcasecmp(s, "speed"))
					SISquad[a].speed = (float)atof(val);
				else if (!strcasecmp(s, "time"))
					SISquad[a].time = (float)atof(val);
				else
					Con_Printf("Bad squad command \"%s\" in \"%s\"\n", s, va("level%i.txt", SIlevel));
			}
			else
				Con_Printf("Bad command \"%s\" in \"%s\"\n", s, va("level%i.txt", SIlevel));

		}

		FS_Close(F);
	}
	else
	{
		//GameCompleate = true;
		//NextLevel = 0;
		Sys_Errorf("Couldn't open file\n", va("spaceinv/level%i.txt", SIlevel+1));
	}
}
#else

	for (a = 0; a < ENEMYTYPES; a++)
	{
		SIEnemyType[a].health = a+1;
		SIEnemyType[a].height = (float)(16*(a+1));
		SIEnemyType[a].width = (float)(16*(a+1));
		SIEnemyType[a].picture = &SIbaddietexture;
		SIEnemyType[a].shotpicture = &SIbullettexture[0];
		SIEnemyType[a].shotdamage = a-1;
		SIEnemyType[a].reward = (a+1) * 25;
	}
#endif

	memset(SImonster, 0, sizeof(*SImonster)*MAXMONSTERS);

	livingmonsters = 0;
	while (SIEnemyType[etype].number <= 0)
	{
		etype++;
		if (etype >= ENEMYTYPES)
			break;
	}
	for (a = 0; a < MAXMONSTERS; a++)
	{
		if (etype < ENEMYTYPES)
		{
			et = &SIEnemyType[etype];
			SImonster[a].EnemyType = et;
			SImonster[a].health = et->health;
			SImonster[a].xpos = random() * vid.width;
			SImonster[a].ypos = (random() * vid.height) / 2;
			SImonster[a].width = et->width;
			SImonster[a].height = et->height;
			SImonster[a].yvel = ENEMYSHIPSPEEDY;
			SImonster[a].xvel = (float)ENEMYSHIPSPEEDX;
			livingmonsters++;

			SIEnemyType[etype].number-=1;
			while (SIEnemyType[etype].number <= 0)
			{
				etype++;
				if (etype >= ENEMYTYPES)
					break;
			}
			if (SIEnemyType[etype].number <= 0)
				break;
		}
		else
			SImonster[a].health = 0;
	}
	for (a = 0; a < MAXSQUADS; a++)
		livingmonsters += SISquad[a].number;
	deadtime = 0;

	memset(SIbullet, 0, sizeof(SIbullet) * MAXBULLETS);

	for (a = 0; a < MAXADDONS; a++)
		SIp.cannon[a].refire = 0;
}

void SIRestartGame (void)
{
	int a;
	SIp.health = 9;
	SIp.cash = 500;
	SIp.powerups = 0;		

	SIp.height = 32;
	SIp.width = 32;

	SIp.y = (vid.height - SIp.height)/2;
	SIp.x = (vid.width - SIp.width) / 2;	

	for (a = 0; a < MAXADDONS; a++)
	{
		SIp.cannon[a].xoff = 0;
		SIp.cannon[a].yoff = 0;

		SIp.cannon[a].power = 0;
		SIp.cannon[a].reloadtime = 0.8f;

		SIp.cannon[a].tex = NULL;
	}

#if 1
	for (a = FIRSTCANNON; a <= LASTCANNON; a++)
	{
		SIp.cannon[a].xoff = (int)((float)a-FIRSTCANNON+1.5f) / 2.0f;
		if (a%2)
			SIp.cannon[a].xoff *= -1;
		else
			SIp.cannon[a].xoff+=1;
		SIp.cannon[a].yoff = 0;	

		SIp.cannon[a].tex = &SIcannontexture;
	}
	for (a = FIRSTROCKET; a <= LASTROCKET; a++)
	{
		SIp.cannon[a].xoff = (int)(a-FIRSTROCKET+1.5f) / 2.0f;
		SIp.cannon[a].xoff -= 0.5f;
		if (a%2)	
			SIp.cannon[a].xoff *= -1.0f;
		else
			SIp.cannon[a].xoff += 1.0f;
		SIp.cannon[a].yoff = 0;

		SIp.cannon[a].tex = &SIcannontexture;
	}
#else
	SIp.cannon[0].xoff = -1;
	SIp.cannon[1].xoff = 1;
	SIp.cannon[2].xoff = -2;
	SIp.cannon[3].xoff = 2;
#endif

	SIp.cannon[M_FORWARD].power = 1;
	SIp.cannon[M_FORWARD].tex = &SIplayertexture;

	SIlevel = 0;
	lastlevel = false;
	SIShop = true;		
	deadtime = 0;
	nextthink = 0;
}










int Plug_MenuEvent(int *args)
{

//	args[2]=(int)(args[2]*640.0f/vid.width);
//	args[3]=(int)(args[3]*480.0f/vid.height);

	switch(args[0])
	{
	case 0:	//draw
		SI_Main(args[2], args[3]);
		SI_2D();
		break;
	case 1:	//keydown
		SI_KeyDown(args[1]);
		break;
	case 2:	//keyup
		SI_KeyUp(args[1]);
		break;
	case 3:	//menu closed (this is called even if we change it).
		break;
	case 4:	//mousemove
		break;
	}

	return 0;
}

int Plug_Tick(int *args)
{
	realtime = args[0]/1000.0f;
	return true;
}

int Plug_ExecuteCommand(int *args)
{
	char cmd[256];
	Cmd_Argv(0, cmd, sizeof(cmd));
	if (!strcmp("spaceinv", cmd))
	{
		Cvar_SetFloat("gl_blend2d", 1);
		SI_LoadTextures();
		Menu_Control(1);
		return 1;
	}
	return 0;
}

int Plug_Init(int *args)
{
	if (Plug_Export("Tick", Plug_Tick) &&
		Plug_Export("ExecuteCommand", Plug_ExecuteCommand) &&
		Plug_Export("MenuEvent", Plug_MenuEvent))
	{

		K_UPARROW		= Key_GetKeyCode("uparrow");
		K_DOWNARROW		= Key_GetKeyCode("downarrow");
		K_LEFTARROW		= Key_GetKeyCode("leftarrow");
		K_RIGHTARROW	= Key_GetKeyCode("rightarrow");
		K_ESCAPE		= Key_GetKeyCode("escape");
		K_HOME			= Key_GetKeyCode("home");
		K_MOUSE1		= Key_GetKeyCode("mouse1");
		K_MOUSE2		= Key_GetKeyCode("mouse2");
		K_SHIFT			= Key_GetKeyCode("shift");
		K_SPACE			= Key_GetKeyCode("space");
		K_F1			= Key_GetKeyCode("f1");
		K_F2			= Key_GetKeyCode("f2");

		Cmd_AddCommand("spaceinv");

		SI_Initialize();
		SI_LoadTextures();

		return 1;
	}
	return 0;
}
