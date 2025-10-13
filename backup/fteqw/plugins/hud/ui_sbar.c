#include "../plugin.h"

#ifdef _MSC_VER
#pragma warning(4: 4244)
#pragma warning(4: 4305)
#endif

#define DEFAULTHUDNAME "ftehud.hud"

#define MAX_ELEMENTS 128

int K_UPARROW;
int K_DOWNARROW;
int K_LEFTARROW;
int K_RIGHTARROW;
int K_ESCAPE;
int K_MOUSE1;
int K_MOUSE2;
int K_HOME;
int K_SHIFT;
int K_MWHEELDOWN;
int K_MWHEELUP;
int K_PAGEUP;
int K_PAGEDOWN;


#define	MAX_CL_STATS		32
#define	STAT_HEALTH			0
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
#define	STAT_ITEMS			15

// context menus

#define CONTEXT_NONE			0
#define CONTEXT_MAIN			1
#define CONTEXT_NEW_ITEM		2
#define CONTEXT_NEW_ITEM_SUB	3

//some engines can use more.
//any mod specific ones should be 31 and downwards rather than upwards.


#define	IT_GUN1			(1<<0)
#define	IT_GUN2			(1<<1)		//the code assumes these are linear.
#define	IT_GUN3			(1<<2)		//be careful with strange mods.
#define	IT_GUN4			(1<<3)
#define	IT_GUN5			(1<<4)
#define	IT_GUN6			(1<<5)
#define	IT_GUN7			(1<<6)
#define	IT_GUN8			(1<<7)	//quake doesn't normally use this.

#define	IT_AMMO1		(1<<8)
#define	IT_AMMO2		(1<<9)
#define	IT_AMMO3		(1<<10)
#define	IT_AMMO4		(1<<11)

#define	IT_GUN0			(1<<12)

#define	IT_ARMOR1		(1<<13)
#define	IT_ARMOR2		(1<<14)
#define	IT_ARMOR3		(1<<15)
#define	IT_SUPERHEALTH	(1<<16)

#define	IT_PUP1			(1<<17)
#define	IT_PUP2			(1<<18)
#define	IT_PUP3			(1<<19)
#define	IT_PUP4			(1<<20)
#define	IT_PUP5			(1<<21)
#define	IT_PUP6			(1<<22)

#define	IT_RUNE1		(1<<23)
#define	IT_RUNE2		(1<<24)
#define	IT_RUNE3		(1<<25)
#define	IT_RUNE4		(1<<26)

//these are linear and treated the same
#define	numpups			6

//the names of the cvars, as they will appear on the console
#define UI_NOSBAR "ui_defaultsbar"
#define UI_NOIBAR "ui_noibar"
#define UI_NOFLASH "ui_nosbarflash"

static char *weaponabbreviation[] = {	//the postfix for the weapon anims
	"shotgun",  // shotgun
	"sshotgun", // super shotgun
	"nailgun",  // nailgun
	"snailgun", // super nailgun
	"rlaunch",  // grenade launcher
	"srlaunch", // rocket launcher
	"lightng"   // thunderbolt
};
#define numweaps (sizeof(weaponabbreviation) / sizeof(char *))

static char *pupabbr[] = {	//the postfix for the powerup anims
	"key1",
	"key2",
	"invis",
	"invul",
	"suit",
	"quad"
};
static char *pupabbr2[] = {	//the postfix for the powerup anims
	"key1",
	"key2",
	"invis",
	"invuln",
	"suit",
	"quad"
};

//0 = owned, 1 selected, 2-7 flashing
static qhandle_t pic_cursor;
static qhandle_t con_chars;
static qhandle_t pic_weapon[8][numweaps];
static qhandle_t sbarback, ibarback;

//0 = owned, 1-6 flashing
static qhandle_t pic_pup[7][numpups];
static qhandle_t pic_armour[3];
static qhandle_t pic_ammo[4];
static qhandle_t pic_rune[4];
static qhandle_t pic_num[13];
static qhandle_t pic_anum[11];

//faces
static qhandle_t pic_face[5];
static qhandle_t pic_facep[5];
static qhandle_t pic_facequad;
static qhandle_t pic_faceinvis;
static qhandle_t pic_faceinvisinvuln;
static qhandle_t pic_faceinvuln;
//static qhandle_t pic_faceinvulnquad;

static int currenttime;
static int gotweapontime[numweaps];
static int gotpuptime[numpups];

float	sbarminx;
float	sbarminy;
float	sbarscalex;
float	sbarscaley;
float	sbaralpha;
int		sbartype;
int sbarindex;

static int hudedit;
static int typetoinsert;

enum {
	DZ_BOTTOMLEFT,
	DZ_BOTTOMRIGHT
};

typedef void drawelementfnc_t(void);
typedef struct {
	float defaultx;	//used if couldn't load a config
	float defaulty;
	int defaultzone;
	float defaultalpha;
	drawelementfnc_t *DrawElement;
	int subtype;
} huddefaultelement_t;





drawelementfnc_t Hud_SBar;
drawelementfnc_t Hud_StatSmall;
drawelementfnc_t Hud_StatBig;
drawelementfnc_t Hud_ArmourPic;
drawelementfnc_t Hud_HealthPic;
drawelementfnc_t Hud_CurrentAmmoPic;
drawelementfnc_t Hud_IBar;
drawelementfnc_t Hud_Weapon;
drawelementfnc_t Hud_W_Lightning;
drawelementfnc_t Hud_Powerup;
drawelementfnc_t Hud_Rune;
drawelementfnc_t Hud_Ammo;
drawelementfnc_t Hud_ScoreCard;
drawelementfnc_t Hud_ScoreName;
drawelementfnc_t Hud_Blackness;
drawelementfnc_t Hud_TeamScore;
drawelementfnc_t Hud_TeamName;
drawelementfnc_t Hud_Tracking;
drawelementfnc_t Hud_TeamOverlay;
// TODO: more elements
// - generalized graphic elements
// - cvar controlled small and big numbers
// - alias controlled graphic elements (both +/-showscores like and alias calling?)
// - Q2-style current weapon icon

int statsremap[] =
{
	STAT_HEALTH,
	STAT_ARMOR,
	STAT_AMMO,
	STAT_SHELLS,
	STAT_NAILS,
	STAT_ROCKETS,
	STAT_CELLS
};

struct subtypenames {
	char *name;
};
typedef struct {
	drawelementfnc_t *draw;
	char *name;
	int width, height;
	int maxsubtype;
	struct subtypenames subtypename[20];
} drawelement_t;
drawelement_t drawelement[] =
{
	{Hud_SBar,		"Status bar",	320,	24,	0},
	{Hud_StatSmall,		"Stat (small)",	8*3,	8,	6, {"Health", "Armour", "Ammo", "Shells", "Nails", "Rockets", "Cells"}}, // equal to sizeof(statsremap)/sizeof(statsremap[0])-1
	{Hud_StatBig,		"Stat (big)",	24*3,	24,	6, {"Health", "Armour", "Ammo", "Shells", "Nails", "Rockets", "Cells"}}, // equal to sizeof(statsremap)/sizeof(statsremap[0])-1
	{Hud_ArmourPic,		"Armor pic",	24,	24,	0},
	{Hud_HealthPic,		"Health pic",	24,	24,	0},
	{Hud_CurrentAmmoPic,	"Ammo pic",	24,	24,	0},
	{Hud_IBar,		"Info bar",	320,	24,	0},
	{Hud_Weapon,		"Weapon pic",	24,	16,	6, {"Shotgun", "Super Shotgun", "Nailgun", "Super Nailgun", "Grenade Launcher", "Rocket Launcher", "Thunderbolt"}},
	{Hud_W_Lightning,	"Shaft pic",	24,	16,	0},
	{Hud_Powerup,		"Powerup pic",	16,	16,	5, {"Key 1", "Key 2", "Ring of Invis", "Pentagram", "Biosuit", "Quad"}},
	{Hud_Rune,		"Rune pic",	8,	16,	3, {"Rune 1", "Rune 2", "Rune 3", "Rune 4"}},
	{Hud_Ammo,		"Ammo display",	42,	11,	3, {"Shells", "Spikes", "Rockets", "Cells"}},
	{Hud_Blackness,		"Blackness",	16,	16,	9, {"10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"}},
	{Hud_ScoreCard,		"Scorecard",	32,	8,	15, {"Player 0", "Player 1", "Player 2", "Player 3", "Player 4", "Player 5", "Player 6", "Player 7", "Player 8", "Player 9", "Player 10", "Player 11", "Player 12", "Player 13", "Player 14", "Player 15"}},
	{Hud_ScoreName,		"Scorename",	128,	8,	7, {"Player 0", "Player 1", "Player 2", "Player 3", "Player 4", "Player 5", "Player 6", "Player 7"}},
	{Hud_TeamScore,		"TeamScore",	32,	8,	7, {"Team 0", "Team 1", "Team 2", "Team 3", "Team 4", "Team 5", "Team 6", "Team 7"}},
	{Hud_TeamName,		"TeamName",	128,	8,	7, {"Team 0", "Team 1", "Team 2", "Team 3", "Team 4", "Team 5", "Team 6", "Team 7"}},
	{Hud_Tracking,		"Tracking",	128,	8,	0},
	{Hud_TeamOverlay,	"Team overlay",	256,	64,	0}
};

huddefaultelement_t hedefaulttype[] = {
	{
		0, -24, DZ_BOTTOMLEFT,
		0.3f,
		Hud_SBar
	},

	{
		0, -24, DZ_BOTTOMLEFT,
		1,
		Hud_ArmourPic
	},
	{
		24, -24, DZ_BOTTOMLEFT,
		1,
		Hud_StatBig,
		1
	},

	{
		112, -24, DZ_BOTTOMLEFT,
		1,
		Hud_HealthPic
	},
	{
		24*6, -24, DZ_BOTTOMLEFT,
		1,
		Hud_StatBig,
		0
	},

	{
		224, -24, DZ_BOTTOMLEFT,
		1,
		Hud_CurrentAmmoPic
	},
	{
		248, -24, DZ_BOTTOMLEFT,
		1,
		Hud_StatBig,
		2
	},

	{
		0, -48, DZ_BOTTOMLEFT,
		0.3f,
		Hud_IBar
	},

	{
		0, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Weapon,
		0
	},
	{
		24, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Weapon,
		1
	},
	{
		48, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Weapon,
		2
	},
	{
		72, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Weapon,
		3
	},
	{
		96, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Weapon,
		4
	},
	{
		120, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Weapon,
		5
	},
	{
		146, -40, DZ_BOTTOMLEFT,
		1,
		Hud_W_Lightning
	},
	{
		194, -40, DZ_BOTTOMLEFT,
		0.3f,
		Hud_Powerup,
		0
	},
	{
		208, -40, DZ_BOTTOMLEFT,
		0.3f,
		Hud_Powerup,
		1
	},
	{
		224, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Powerup,
		2
	},
	{
		240, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Powerup,
		3
	},
	{
		256, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Powerup,
		4
	},
	{
		272, -40, DZ_BOTTOMLEFT,
		1,
		Hud_Powerup,
		5
	},
	{
		288, -40, DZ_BOTTOMLEFT,
		0.3f,
		Hud_Rune,
		0
	},
	{
		296, -40, DZ_BOTTOMLEFT,
		0.3f,
		Hud_Rune,
		1
	},
	{
		304, -40, DZ_BOTTOMLEFT,
		0.3f,
		Hud_Rune,
		2
	},
	{
		312, -40, DZ_BOTTOMLEFT,
		0.3f,
		Hud_Rune,
		3
	},

	{
		48*0+3, -48, DZ_BOTTOMLEFT,
		1,
		Hud_Ammo,
		0
	},
	{
		48*1+3, -48, DZ_BOTTOMLEFT,
		1,
		Hud_Ammo,
		1
	},
	{
		48*2+3, -48, DZ_BOTTOMLEFT,
		1,
		Hud_Ammo,
		2
	},
	{
		48*3+3, -48, DZ_BOTTOMLEFT,
		1,
		Hud_Ammo,
		3
	},

	{
		42*3, -48, DZ_BOTTOMLEFT,
		1,
		Hud_ScoreCard
	}
};
typedef struct {
	int type;
	int subtype;

	float x, y;
	float scalex;
	float scaley;
	float alpha;
} hudelement_t;
hudelement_t element[MAX_ELEMENTS];	//look - Spike used a constant - that's a turn up for the books!
int numelements;

int currentitem;
int hoveritem;
qboolean mousedown, shiftdown;
float mouseofsx, mouseofsy;
qboolean context;

vec3_t player_location[32];
int player_armor[32];
int player_health[32];
unsigned int player_items[32];
char *player_nick[32] =
{
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", ""
};
unsigned int player_nicklength;

void Hud_TeamOverlayUpdate(void);

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension, int maxlen)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strlcpy (path+strlen(path), extension, maxlen);
}

void UI_DrawPic(qhandle_t pic, int x, int y, int width, int height)
{
	Draw_Image((float)x*sbarscalex+sbarminx, (float)y*sbarscaley+sbarminy, (float)width*sbarscalex, (float)height*sbarscaley, 0, 0, 1, 1, pic);
}
void UI_DrawChar(unsigned int c, int x, int y)
{
static float size = 1.0f/16.0f;
	float s1 = size * (c&15);
	float t1 = size * ((c>>4)&15);
	Draw_Image((float)x*sbarscalex+sbarminx, (float)y*sbarscaley+sbarminy, 8*sbarscalex, 8*sbarscaley, s1, t1, s1+size, t1+size, con_chars);
}
void UI_DrawString(char *s, int x, int y)
{
	while(*s)
	{
		UI_DrawChar((unsigned int)*s++, x, y);
		x+=8;
	}
}
void UI_DrawAltString(char *s, int x, int y, qboolean shouldmask)
{
	int mask = shouldmask?128:0;
	while(*s)
	{
		UI_DrawChar((unsigned int)*s++ | mask, x, y);
		x+=8;
	}
}


void UI_DrawBigNumber(int num, int x, int y, qboolean red)
{
	char *s;
	int len;
	s = va("%i", num);


	len = strlen(s);
	if (len < 3)
		x += 24*(3-len);
	else
		s += len-3;

	if (red)
	{
		while(*s)
		{
			if (*s == '-')
				UI_DrawPic (pic_anum[10], x, y, 24, 24);
			else
				UI_DrawPic (pic_anum[*s-'0'], x, y, 24, 24);
			s++;
			x+=24;
		}
	}
	else
	{
		while(*s)
		{
			if (*s == '-')
				UI_DrawPic (pic_num[10], x, y, 24, 24);
			else
				UI_DrawPic (pic_num[*s-'0'], x, y, 24, 24);
			s++;
			x+=24;
		}
	}
}

void SBar_FlushAll(void)
{
	numelements = 0;
}

static int idxforfunc(drawelementfnc_t *fnc)
{
	int i;
	for (i = 0; i < sizeof(drawelement)/sizeof(drawelement[0]); i++)
	{
		if (drawelement[i].draw == fnc)
			return i;
	}
	return -10000;	//try and crash
}

void SBar_ReloadDefaults(void)
{
	int i;
	for (i = 0; i < sizeof(hedefaulttype)/sizeof(hedefaulttype[0]); i++)
	{
		if (hedefaulttype[i].defaultalpha)
		{
			if (numelements >= MAX_ELEMENTS)
				break;
			element[numelements].type = idxforfunc(hedefaulttype[i].DrawElement);
			element[numelements].alpha = hedefaulttype[i].defaultalpha;
			element[numelements].scalex = 1;
			element[numelements].scaley = 1;
			element[numelements].subtype = hedefaulttype[i].subtype;
			switch(hedefaulttype[i].defaultzone)
			{
			case DZ_BOTTOMLEFT:
				element[numelements].x = hedefaulttype[i].defaultx;
				element[numelements].y = 480+hedefaulttype[i].defaulty;
				break;
			case DZ_BOTTOMRIGHT:
				element[numelements].x = 640+hedefaulttype[i].defaultx;
				element[numelements].y = 480+hedefaulttype[i].defaulty;
				break;
			}
			numelements++;
		}
	}
}

void UI_SbarInit(void)
{
	int i;
	int j;

//main bar (add cvars later)
	ibarback = Draw_LoadImage("ibar", true);
	sbarback = Draw_LoadImage("sbar", true);

	con_chars = Draw_LoadImage("conchars", true);

//load images.
	for (i = 0; i < 10; i++)
	{
		pic_num[i] = Draw_LoadImage(va("num_%i", i), true);
		pic_anum[i] = Draw_LoadImage(va("anum_%i", i), true);
	}
	pic_num[10] = Draw_LoadImage("num_minus", true);
	pic_anum[10] = Draw_LoadImage("anum_minus", true);
	pic_num[11] = Draw_LoadImage("num_colon", true);
	pic_num[12] = Draw_LoadImage("num_slash", true);

	for (i = 0; i < numweaps; i++)
	{
		gotweapontime[i] = 0;
		pic_weapon[0][i] = Draw_LoadImage(va("inv_%s", weaponabbreviation[i]), true);
		pic_weapon[1][i] = Draw_LoadImage(va("inv2_%s", weaponabbreviation[i]), true);
		for (j = 0; j < 5; j++)
		{
			pic_weapon[2+j][i] = Draw_LoadImage(va("inva%i_%s", j+1, weaponabbreviation[i]), true);
		}
	}
	for (i = 0; i < numpups; i++)
	{
		gotpuptime[i] = 0;
		pic_pup[0][i] = Draw_LoadImage(va("sb_%s", pupabbr2[i]), true);
		for (j = 0; j < 5; j++)
		{
			pic_pup[1+j][i] = Draw_LoadImage(va("sba%i_%s", j+1, pupabbr[i]), true);
		}
	}
	pic_cursor = Draw_LoadImage("gfx/cursor", false);

	pic_armour[0] = Draw_LoadImage("sb_armor1", true);
	pic_armour[1] = Draw_LoadImage("sb_armor2", true);
	pic_armour[2] = Draw_LoadImage("sb_armor3", true);

	pic_ammo[0] = Draw_LoadImage("sb_shells", true);
	pic_ammo[1] = Draw_LoadImage("sb_nails", true);
	pic_ammo[2] = Draw_LoadImage("sb_rocket", true);
	pic_ammo[3] = Draw_LoadImage("sb_cells", true);

	pic_rune[0] = Draw_LoadImage("sb_sigil1", true);
	pic_rune[1] = Draw_LoadImage("sb_sigil2", true);
	pic_rune[2] = Draw_LoadImage("sb_sigil3", true);
	pic_rune[3] = Draw_LoadImage("sb_sigil4", true);

	pic_face[0] = Draw_LoadImage("face1", true);
	pic_face[1] = Draw_LoadImage("face2", true);
	pic_face[2] = Draw_LoadImage("face3", true);
	pic_face[3] = Draw_LoadImage("face4", true);
	pic_face[4] = Draw_LoadImage("face5", true);

	pic_facep[0] = Draw_LoadImage("face_p1", true);
	pic_facep[1] = Draw_LoadImage("face_p2", true);
	pic_facep[2] = Draw_LoadImage("face_p3", true);
	pic_facep[3] = Draw_LoadImage("face_p4", true);
	pic_facep[4] = Draw_LoadImage("face_p5", true);

	pic_facequad = Draw_LoadImage("face_quad", true);
	pic_faceinvis = Draw_LoadImage("face_invis", true);
	pic_faceinvisinvuln = Draw_LoadImage("face_inv2", true);
	pic_faceinvuln = Draw_LoadImage("face_invul2", true);
//	pic_faceinvulnquad = Draw_LoadImage("face_invul1", true);

	SBar_FlushAll();
	SBar_ReloadDefaults();
}

unsigned int stats[MAX_CL_STATS];

void Hud_SBar(void)
{
	UI_DrawPic(sbarback, 0, 0, 320, 24);
}

void Hud_ArmourPic(void)
{
	if (stats[STAT_ITEMS] & IT_ARMOR3)
		UI_DrawPic(pic_armour[2], 0, 0, 24, 24);
	else if (stats[STAT_ITEMS] & IT_ARMOR2)
		UI_DrawPic(pic_armour[1], 0, 0, 24, 24);
	else if (stats[STAT_ITEMS] & IT_ARMOR1 || hudedit)
		UI_DrawPic(pic_armour[0], 0, 0, 24, 24);
}

void Hud_HealthPic(void)
{
	int hl;

	if (stats[STAT_ITEMS] & IT_PUP3)
	{	//invisability
		if (stats[STAT_ITEMS] & IT_PUP4)
			UI_DrawPic(pic_faceinvisinvuln, 0, 0, 24, 24);
		else
			UI_DrawPic(pic_faceinvis, 0, 0, 24, 24);
		return;
	}

	if (stats[STAT_ITEMS] & IT_PUP4)
	{	//invuln
//		if (stats[STAT_ITEMS] & IT_PUP6)
//			UI_DrawPic(pic_faceinvulnquad, 0, 0, 24, 24);
//		else
			UI_DrawPic(pic_faceinvuln, 0, 0, 24, 24);
		return;
	}
	if (stats[STAT_ITEMS] & IT_PUP6)
	{
		UI_DrawPic(pic_facequad, 0, 0, 24, 24);
		return;
	}

	hl = stats[STAT_HEALTH]/20;
	if (hl > 4)
		hl = 4;
	if (hl < 0)
		hl = 0;

	//FIXME
//	if (innpain)
//		UI_DrawPic(pic_facep[4-hl], 0, 0, 24, 24);
//	else
		UI_DrawPic(pic_face[4-hl], 0, 0, 24, 24);
}

void Hud_StatBig(void)
{
	int i = stats[statsremap[sbartype]];

	UI_DrawBigNumber(i, 0, 0, i < 25);
}

void Hud_StatSmall(void)
{
	int i = stats[statsremap[sbartype]];

	// TODO: need some sort of options thing to change between brown/white/gold text
	UI_DrawChar(i%10+18, 19, 0);
	i/=10;
	if (i)
		UI_DrawChar(i%10+18, 11, 0);
	i/=10;
	if (i)
		UI_DrawChar(i%10+18, 3, 0);
}

void Hud_CurrentAmmoPic(void)
{
		 if ((stats[STAT_ITEMS] & IT_AMMO1))
		UI_DrawPic(pic_ammo[0], 0, 0, 24, 24);
	else if (stats[STAT_ITEMS] & IT_AMMO2)
		UI_DrawPic(pic_ammo[1], 0, 0, 24, 24);
	else if (stats[STAT_ITEMS] & IT_AMMO3)
		UI_DrawPic(pic_ammo[2], 0, 0, 24, 24);
	else if (stats[STAT_ITEMS] & IT_AMMO4 || hudedit)
		UI_DrawPic(pic_ammo[3], 0, 0, 24, 24);
}

void Hud_IBar(void)
{
	UI_DrawPic(ibarback, 0, 0, 320, 24);
}

void Hud_Weapon(void)
{
	int flash;
	if (!(stats[STAT_ITEMS] & (IT_GUN1 << sbartype)) && !hudedit)
	{
		gotweapontime[sbartype] = 0;
		return;
	}

	if (!gotweapontime[sbartype])
		gotweapontime[sbartype] = currenttime;
	flash = (currenttime - gotweapontime[sbartype])/100;
	if (flash < 0)	//errr... whoops...
		flash = 0;

	if (flash > 10)
	{
		if (stats[STAT_ACTIVEWEAPON] & (IT_GUN1 << sbartype))
			flash = 1;	//selected.
		else
			flash = 0;
	}
	else
		flash = (flash%5) + 2;

	UI_DrawPic(pic_weapon[flash][sbartype], 0, 0, 24, 16);
}

void Hud_W_HalfLightning(void)	//left half only (needed due to LG icon being twice as wide)
{
	int flash;
	int wnum = 6;

	if (!(stats[STAT_ITEMS] & (IT_GUN1 << wnum)) && !hudedit)
	{
		gotweapontime[wnum] = 0;
		return;
	}

	if (!gotweapontime[wnum])
		gotweapontime[wnum] = currenttime;
	flash = (currenttime - gotweapontime[wnum])/100;
	if (flash < 0)	//errr... whoops...
		flash = 0;

	if (flash > 10)
	{
		if (stats[STAT_ACTIVEWEAPON] & (IT_GUN1 << wnum))
			flash = 1;	//selected.
		else
			flash = 0;
	}
	else
		flash = (flash%5) + 2;

	Draw_Image(sbarminx, sbarminy, (float)24*sbarscalex, (float)16*sbarscaley, 0, 0, 0.5, 1, pic_weapon[flash][wnum]);
}
void Hud_W_Lightning(void)
{
	int flash;
	int wnum = 6;

	if (!(stats[STAT_ITEMS] & (IT_GUN1 << wnum)) && !hudedit)
	{
		gotweapontime[wnum] = 0;
		return;
	}

	if (!gotweapontime[wnum])
		gotweapontime[wnum] = currenttime;
	flash = (currenttime - gotweapontime[wnum])/100;
	if (flash < 0)	//errr... whoops...
		flash = 0;

	if (flash > 10)
	{
		if (stats[STAT_ACTIVEWEAPON] & (IT_GUN1 << wnum))
			flash = 1;	//selected.
		else
			flash = 0;
	}
	else
		flash = (flash%5) + 2;

	UI_DrawPic(pic_weapon[flash][wnum], 0, 0, 48, 16);
}

void Hud_Powerup(void)
{
	int flash;
	if (!(stats[STAT_ITEMS] & (IT_PUP1 << sbartype)) && !hudedit)
		return;

	if (!gotpuptime[sbartype])
		gotpuptime[sbartype] = currenttime;
	flash = (currenttime - gotpuptime[sbartype])/100;
	if (flash < 0)	//errr... whoops...
		flash = 0;

	if (flash > 10)
	{
		flash = 0;
	}
	else
		flash = (flash%5) + 2;

	UI_DrawPic(pic_pup[flash][sbartype], 0, 0, 16, 16);
}

void Hud_Rune(void)
{
	if (!(stats[STAT_ITEMS] & (IT_RUNE1 << sbartype)) && !hudedit)
		return;
	UI_DrawPic(pic_rune[sbartype], 0, 0, 8, 16);
}

void Hud_Ammo(void)
{
	int num;
	Draw_Image(sbarminx, sbarminy, (float)42*sbarscalex, (float)11*sbarscaley, (3+(sbartype*48))/320.0f, 0, (3+(sbartype*48)+42)/320.0f, 11/24.0f, ibarback);

	num = stats[STAT_SHELLS+sbartype];
	if (hudedit)
		num = 999;

	UI_DrawChar(num%10+18, 19, 0);
	num/=10;
	if (num)
		UI_DrawChar(num%10+18, 11, 0);
	num/=10;
	if (num)
		UI_DrawChar(num%10+18, 3, 0);
}

float pc[16][3] =
{
/*
	{235, 235, 235},
	{143, 111, 035},
	{139, 139, 203},
	{107, 107, 015},
	{127, 000, 000},
	{175, 103, 035},
	{255, 243, 027},
	{227, 179, 151},
	{171, 139, 163},
	{187, 115, 159},
	{219, 195, 187},
	{111, 131, 123},
	{255, 243, 027},
	{000, 000, 255},
	{247, 211, 139}
*/
	{0.922, 0.922, 0.922},
	{0.560, 0.436, 0.137},
	{0.545, 0.545, 0.796},
	{0.420, 0.420, 0.059},
	{0.498, 0.000, 0.000},
	{0.686, 0.404, 0.137},
	{1.000, 0.953, 0.106},
	{0.890, 0.702, 0.592},
	{0.671, 0.545, 0.639},
	{0.733, 0.451, 0.624},
	{0.859, 0.765, 0.733},
	{0.436, 0.514, 0.482},
	{1.000, 0.953, 0.106},
	{0.000, 0.000, 1.000},
	{0.969, 0.827, 0.545}
};

int numsortedplayers;
int trackedplayer;
int sortedplayers[32];
plugclientinfo_t players[32];

void SortPlayers(void)
{
	int i, j;
	int temp;

	numsortedplayers = 0;
	trackedplayer = -1;
	for (i = 0; i < 32; i++)
	{
		if (GetPlayerInfo(i, &players[i])>0)
			trackedplayer = i;
		if (players[i].spectator)
			continue;
		if (*players[i].name != 0)
			sortedplayers[numsortedplayers++] = i;
	}

	for (i = 0; i < numsortedplayers; i++)
	{
		for (j = i+1; j < numsortedplayers; j++)
		{
			if (players[sortedplayers[i]].frags < players[sortedplayers[j]].frags)
			{
				temp = sortedplayers[j];
				sortedplayers[j] = sortedplayers[i];
				sortedplayers[i] = temp;
			}
		}
	}
}

typedef struct {
	char name[8];
	int frags;
	int tc;
	int bc;
} teams_t;
teams_t team[32];
int numsortedteams;

void SortTeams(void)
{
	teams_t temp;
	int i, j;

	numsortedplayers = 0;
	trackedplayer = -1;
	for (i = 0; i < 32; i++)
	{
		if (GetPlayerInfo(i, &players[i])>0)
			trackedplayer = i;
		if (players[i].spectator)
			continue;
		if (*players[i].name != 0)
			sortedplayers[numsortedplayers++] = i;

		for (j = 0; j < numsortedteams; j++)
		{
			if (!strcmp(team[j].name, players[i].name))
			{
				team[j].frags += players[i].frags;
				while(j > 0)
				{
					if (team[j-1].frags < team[j].frags)
					{
						memcpy(&temp, &team[j], sizeof(teams_t));
						memcpy(&team[j], &team[j-1], sizeof(teams_t));
						memcpy(&team[j-1], &temp, sizeof(teams_t));
						j--;
					}
					else
						break;
				}
			}
		}
		if (j == numsortedteams)
		{
			strlcpy(team[j].name, players[i].name, sizeof(team[j].name));
			team[j].frags = players[i].frags;
			team[j].tc = players[i].topcolour;
			team[j].bc = players[i].bottomcolour;
			numsortedteams++;
		}
	}
}

void Hud_ScoreCard(void)
{
	int frags, tc, bc, p;
	int brackets;
	char number[6];

	if (hudedit)
	{
		frags = sbartype;
		tc = 0;
		bc = 0;
		brackets = 1;
	}
	else
	{
		SortPlayers();
		if (sbartype>=numsortedplayers)
			return;
		p = sortedplayers[sbartype];
		bc = players[p].bottomcolour;
		tc = players[p].topcolour;
		frags = players[p].frags;
		brackets = p==trackedplayer;
	}

	Draw_Colour4f(pc[tc][0], pc[tc][1], pc[tc][2], sbaralpha);
	Draw_Fill(sbarminx, sbarminy, (float)32*sbarscalex, (float)4*sbarscaley);
	Draw_Colour4f(pc[bc][0], pc[bc][1], pc[bc][2], 	sbaralpha);
	Draw_Fill(sbarminx, sbarminy+4*sbarscaley, (float)32*sbarscalex, (float)4*sbarscaley);

	Draw_Colour4f(1, 1, 1, sbaralpha);
	if (brackets)
	{
		UI_DrawChar(16, 0, 0);
		UI_DrawChar(17, 24, 0);
	}

	snprintf(number, sizeof(number), "%-3i", frags);
	UI_DrawChar(number[0], 4, 0);
	UI_DrawChar(number[1], 12, 0);
	UI_DrawChar(number[2], 20, 0);

	Draw_Colour4f(1,1,1,1);
}
void Hud_ScoreName(void)
{
	int p;
	char *name;
	if (hudedit)
	{
		name = va("Player %i", sbartype);
	}
	else
	{
		SortPlayers();
		if (sbartype>=numsortedplayers)
			return;
		p = sortedplayers[sbartype];
		name = players[p].name;
	}
	UI_DrawString(name, 0, 0);
}

void Hud_TeamScore(void)
{
	int frags, tc, bc, p;
	int brackets;
	char number[6];

	if (hudedit)
	{
		frags = sbartype;
		tc = 0;
		bc = 0;
		brackets = 1;
	}
	else
	{
		SortPlayers();
		if (sbartype>=numsortedteams)
			return;
		p = sbartype;
		bc = team[p].bc;
		tc = team[p].tc;
		frags = team[p].frags;
		brackets = p==trackedplayer;
	}

	Draw_Colour4f(pc[tc][0], pc[tc][1], pc[tc][2], sbaralpha);
	Draw_Fill(sbarminx, sbarminy, (float)32*sbarscalex, (float)4*sbarscaley);
	Draw_Colour4f(pc[bc][0], pc[bc][1], pc[bc][2], 	sbaralpha);
	Draw_Fill(sbarminx, sbarminy+4*sbarscaley, (float)32*sbarscalex, (float)4*sbarscaley);

	Draw_Colour4f(1, 1, 1, sbaralpha);
	if (brackets)
	{
		UI_DrawChar(16, 0, 0);
		UI_DrawChar(17, 24, 0);
	}

	snprintf(number, sizeof(number), "%-3i", frags);
	UI_DrawChar(number[0], 4, 0);
	UI_DrawChar(number[1], 12, 0);
	UI_DrawChar(number[2], 20, 0);



	Draw_Colour4f(1,1,1,1);
}

void Hud_TeamName(void)
{
	int p;
	char *tname;

	if (hudedit)
	{
		tname = va("T%-3i", sbartype);
	}
	else
	{
		SortTeams();
		if (sbartype>=numsortedteams)
			return;
		p = sbartype;
		tname = team[p].name;
	}

	Draw_Colour4f(1, 1, 1, sbaralpha);

	if (tname[0])
	{
		UI_DrawChar(tname[0], 0, 0);
		if (tname[1])
		{
			UI_DrawChar(tname[1], 8, 0);
			if (tname[2])
			{
				UI_DrawChar(tname[2], 8, 0);
				if (tname[3])
				{
					UI_DrawChar(tname[3], 8, 0);
				}
			}
		}
	}
	Draw_Colour4f(1,1,1,1);
}

void Hud_TeamOverlay(void)
{
	static int current_player = -1;
	int offset = 0;

	if (hudedit)
	{
		UI_DrawString("UnnamedPlayer1: r999/999 rlg ra-mega", 0, 0);
		UI_DrawString("UnnamedPlayer2: r999/999 rlg ra-mega", 0, 16);
		UI_DrawString("UnnamedPlayer3: r999/999 rlg ra-mega", 0, 32);
	}
	else
	{
		unsigned int i;

		// If the tracking has changed, flush the old teaminfo
		if (current_player != players[trackedplayer].userid)
		{
			int j;

			for (j = 0; j < 32; j++)
				player_nick[j] = "";
			player_nicklength = 0;

			current_player = players[trackedplayer].userid;

			return;
		}

		for (i = 0; i < 32; i++)
		{
			// Empty nicknames are defined as not being printed
			if (player_nick[i] != "")
			{
				char armortype = ' ';
				char* bestweap = "    ";
				char loc[256], str[256], spacing[64], spacing_a[64], spacing_h[64];
				unsigned int j;

				// More info about armortype
				if (player_items[i] & IT_ARMOR3)
					armortype = 'r';
				else if (player_items[i] & IT_ARMOR2)
					armortype = 'y';
				else if (player_items[i] & IT_ARMOR1)
					armortype = 'g';

				// Only care about reporting weapons that have some meaning
				if ((player_items[i] & (IT_GUN6 | IT_GUN7)) == (IT_GUN6 | IT_GUN7))
					bestweap = "rlg ";
				else if ((player_items[i] & IT_GUN7) == IT_GUN7)
					bestweap = "lg  ";
				else if ((player_items[i] & IT_GUN6) == IT_GUN6)
					bestweap = "rl  ";
				else if ((player_items[i] & IT_GUN5) == IT_GUN5)
					bestweap = "gl  ";
				else if ((player_items[i] & IT_GUN4) == IT_GUN4)
					bestweap = "sng ";
				else if ((player_items[i] & IT_GUN2) == IT_GUN2)
					bestweap = "ssg ";

				GetLocationName(player_location[i], loc, sizeof(loc));

				// Format spacing

				// Nicknames
				for (j = 0; j < (player_nicklength - strlen(player_nick[i]) + 1) && j < sizeof(spacing); j++)
					spacing[j] = ' ';
				spacing[j] = '\0';

				// Armor
				if (player_armor[i] % 10 == player_armor[i])
					strlcpy(spacing_a, "  ", sizeof(spacing_a));	// 0 - 9
				else if (player_armor[i] % 100 == player_armor[i])
					strlcpy(spacing_a, " ", sizeof(spacing_a));	// 10 - 99
				else
					strlcpy(spacing_a, "", sizeof(spacing_a));

				// Health
				if (player_health[i] % 10 == player_health[i])
					strlcpy(spacing_h, "  ", sizeof(spacing_h));	// 0 - 9
				else if (player_health[i] % 100 == player_health[i])
					strlcpy(spacing_h, " ", sizeof(spacing_h));	// 10 - 99
				else
					strlcpy(spacing_h, "", sizeof(spacing_h));

				// TODO: Translate $5 and similar macros in loc names.
				snprintf(str, sizeof(str), "%s%c%s%s%c%d%c%d %s%s%c%s%c",
					player_nick[i],		// player netname
					':'+128,			// colored colon
					spacing,			// spacing
					spacing_a,			// armor spacing
					armortype,			// armor type: r, y, g or none
					player_armor[i],	// current armor
					'/'+128,			// colored slash
					player_health[i],	// current health
					spacing_h,			// health spacing
					bestweap,			// best weapon
					'\x10',				// left bracket
					loc,				// player location
					'\x11'				// right bracket
					);

				UI_DrawString(str, 0, offset);
				offset += 16;
			}
		}
	}
}

void Hud_TeamOverlayUpdate(void)
{
	int user;
	char str[256];

	// Number in the players[] array
	Cmd_Argv(1, str, sizeof(str));
	user = atoi(str);

	// Position in the x-axis
	Cmd_Argv(2, str, sizeof(str));
	player_location[user][0] = atoi(str);

	// Position in the y-axis
	Cmd_Argv(3, str, sizeof(str));
	player_location[user][1] = atoi(str);

	// Position in the z-axis
	Cmd_Argv(4, str, sizeof(str));
	player_location[user][2] = atoi(str);

	// Player health
	Cmd_Argv(5, str, sizeof(str));
	player_health[user] = atoi(str);

	// Player armor
	Cmd_Argv(6, str, sizeof(str));
	player_armor[user] = atoi(str);

	// Player item bitmask
	Cmd_Argv(7, str, sizeof(str));
	player_items[user] = atoi(str);

	// Setting this nick will make this info print
	player_nick[user] = players[user].name;

	// Check who has the longest nick for a better overlay format
	if (strlen(player_nick[user]) > player_nicklength)
		player_nicklength = strlen(player_nick[user]);

	return;
}

//fixme: draw dark blobs
void Hud_Blackness(void)
{
	Draw_Colour4f(0, 0, 0, (sbartype+1)/10.0f);

	if (hudedit)
	{
		if (sbarindex == currentitem)
		{
			float j = ((currenttime % 1000) - 500) / 500.0f;
			if (j < 0)
				j = -j;
			j/=3;

			Draw_Colour4f(j, 0, 0, (sbartype+1)/10.0f);
		}
		else if (sbarindex == hoveritem)
		{
			Draw_Colour4f(0.0, 0.2, 0.0, (sbartype+1)/10.0f);
		}
	}
	Draw_Fill(sbarminx, sbarminy, (float)16*sbarscalex, (float)16*sbarscaley);
	Draw_Colour4f(1,1,1,1);
}

void Hud_Tracking(void)
{
	qboolean flag = false;
	char str[256];

	if (hudedit)
	{
		UI_DrawString("Tracking ...", 0, 0);
		return;
	}

	// FIXME: Need a check here to return if we are not spectating

	// Print it
	snprintf(str, sizeof(str), "Tracking %s", players[trackedplayer].name);
	UI_DrawString(str, 0, 0);
}

void UI_DrawHandles(int *arg, int i)
{
	int mt;
	float vsx, vsy;
	vsx = arg[3]/640.0f;
	vsy = arg[4]/480.0f;

	sbarminx = arg[1] + element[i].x*vsx;
	sbarminy = arg[2] + element[i].y*vsy;
	sbarscalex  = element[i].scalex*vsx;
	sbarscaley  = element[i].scaley*vsy;
	mt = element[i].type;
	sbartype = element[i].subtype;
	sbaralpha = element[i].alpha;

	Draw_Colour4f(1, 0, 0, 1);
	Draw_Fill(sbarminx+drawelement[mt].width*sbarscalex-((sbarscalex<0)?0:(vsx*4)), sbarminy-((sbarscaley>=0)?0:(vsy*4)), (float)4*vsx, (float)4*vsy);

	Draw_Colour4f(0, 1, 0, 1);
	Draw_Fill(sbarminx+drawelement[mt].width*sbarscalex-((sbarscalex<0)?0:(vsx*4)), sbarminy+drawelement[mt].height*sbarscaley-((sbarscaley<0)?0:(vsy*4)), (float)4*vsx, (float)4*vsy);
}

//draw body of sbar
//arg[0] is playernum
//arg[1]/arg[2] is x/y start of subwindow
//arg[3]/arg[4] is width/height of subwindow
int UI_StatusBar(int *arg)
{
	int i;

	float vsx, vsy;

	if (hudedit) // don't redraw twice
		return 1;

	if (arg[5])
		return 1;

	CL_GetStats(arg[0], stats, sizeof(stats)/sizeof(int));

	if (stats[STAT_HEALTH] <= 0)
		return 1;

	vsx = arg[3]/640.0f;
	vsy = arg[4]/480.0f;
	for (i = 0; i < numelements; i++)
	{
		sbarminx = arg[1] + element[i].x*vsx;
		sbarminy = arg[2] + element[i].y*vsy;
		sbarscalex  = element[i].scalex*vsx;
		sbarscaley  = element[i].scaley*vsy;
		sbartype = element[i].subtype;
		sbaralpha = element[i].alpha;
		drawelement[element[i].type].draw();
	}

	return 1;
}

int UI_StatusBarEdit(int *arg) // seperated so further improvements to editor view can be done
{
	int i;

	float vsx, vsy;
	qboolean clrset = false;

	CL_GetStats(arg[0], stats, sizeof(stats)/sizeof(int));

	vsx = arg[3]/640.0f;
	vsy = arg[4]/480.0f;
	for (i = 0; i < numelements; i++)
	{
		if (i == currentitem)
		{
			float j = ((currenttime % 1000) - 500) / 500.0f;
			if (j < 0)
				j = -j;

			Draw_Colour3f(1.0, j, j);
			clrset = true;
		}
		else if (i == hoveritem)
		{
			Draw_Colour3f(0.0, 1.0, 0.0);
			clrset = true;
		}

		sbarminx = arg[1] + element[i].x*vsx;
		sbarminy = arg[2] + element[i].y*vsy;
		sbarscalex  = element[i].scalex*vsx;
		sbarscaley  = element[i].scaley*vsy;
		sbartype = element[i].subtype;
		sbaralpha = element[i].alpha;
		sbarindex = i;
		drawelement[element[i].type].draw();

		if (clrset)
		{
			Draw_Colour3f(1.0, 1.0, 1.0);
			clrset = false;
		}
	}

	return true;
}

/*
int UI_ScoreBoard(int *arg)
{
	int i;

	if (!arg[5])
		return false;

	sbarminx = 320;
	sbarminy = 48;
	sbarscalex = 1;
	sbarscaley = 1;
	sbaralpha = 1;

	SortPlayers();
	for (i = 0; i < numsortedplayers; i++)
	{
		sbartype = i;
		Hud_ScoreCard();
		UI_DrawString(players[sortedplayers[i]].name, 40, 0);

		sbarminy += 16;
	}

	return true;
}
*/

#define HUD_VERSION 52345
void PutFloat(float f, char sep, qhandle_t handle)
{
	char *buffer;
	buffer = va("%f%c", f, sep);
	FS_Write(handle, buffer, strlen(buffer));
}
void PutInteger(int i, char sep, qhandle_t handle)
{
	char *buffer;
	buffer = va("%i%c", i, sep);
	FS_Write(handle, buffer, strlen(buffer));
}

void Hud_Save(char *fname)
{
	char name[256];
	int i;
	qhandle_t handle;
	if (!fname || !*fname)
		fname = DEFAULTHUDNAME;
	snprintf(name, sizeof(name)-5, "huds/%s", fname);
	COM_DefaultExtension(name, ".hud", sizeof(name));
	if (FS_Open(name, &handle, 2)<0)
	{
		Con_Printf("Couldn't open %s\n", name);
		return;
	}

	PutInteger(HUD_VERSION, '\n', handle);
	PutInteger(numelements, '\n', handle);
	for (i = 0; i < numelements; i++)
	{
		PutFloat(element[i].x, ' ', handle);
		PutFloat(element[i].y, ' ', handle);
		PutFloat(element[i].scalex, ' ', handle);
		PutFloat(element[i].scaley, ' ', handle);
		PutInteger(element[i].type, ' ', handle);
		PutInteger(element[i].subtype, ' ', handle);
		PutFloat(element[i].alpha, '\n', handle);
	}

	FS_Close(handle);
}
float GetFloat(char **f, qhandle_t handle)
{
	char *ts;
	while(**f <= ' ' && **f != 0)
		(*f)++;
	while(*f[0] == '/' && *f[1] == '/')
	{
		while(**f != '\n' && **f != 0)
			(*f)++;
		while(**f <= ' ' && **f != 0)
			(*f)++;
	}
	ts = *f;
	while (**f>' ')
		(*f)++;

	return (float)atof(ts);
}
int GetInteger(char **f, qhandle_t handle)
{
	char *ts;
	while(**f <= ' ' && **f != 0)
		(*f)++;
	while(*f[0] == '/' && *f[1] == '/')
	{
		while(**f != '\n' && **f != 0)
			(*f)++;
		while(**f <= ' ' && **f != 0)
			(*f)++;
	}
	ts = *f;
	while (**f>' ')
		(*f)++;

	return atoi(ts);
}
void Hud_Load(char *fname)
{
	char file[16384];
	char name[256];
	char *p;
	int len;
	int i;
	qhandle_t handle;
	int ver;

	float x, y, sx, sy, a;
	int type, subtype;

	if (!fname || !*fname)
		fname = DEFAULTHUDNAME;
	snprintf(name, sizeof(name)-5, "huds/%s", fname);
	COM_DefaultExtension(name, ".hud", sizeof(name));
	len = FS_Open(name, &handle, 1);
	if (len < 0)
	{
		Con_Printf("Couldn't load file\n");
		return;
	}
	if (len > 16383)
		len = 16383;
	FS_Read(handle, file, len);
	file[len] = 0;
	FS_Close(handle);

	p = file;

	ver = GetInteger(&p, handle);
	if (ver != HUD_VERSION)
	{
		Con_Printf("Hud version doesn't match (%i != %i)\n", ver, HUD_VERSION);
		return;
	}
	numelements = GetInteger(&p, handle);
	if (numelements > MAX_ELEMENTS)
	{
		numelements = 0;
		Con_Printf("Hud has too many elements\n");
		return;
	}
	for (i = 0; i < numelements; i++)
	{
		x = GetFloat(&p, handle);
		y = GetFloat(&p, handle);
		sx = GetFloat(&p, handle);
		sy = GetFloat(&p, handle);
		type = GetInteger(&p, handle);
		subtype = GetInteger(&p, handle);
		a = GetFloat(&p, handle);

		if (type<0 || type>=sizeof(drawelement)/sizeof(drawelement[0]))
		{
			numelements--;
			i--;
			continue;
		}

		element[i].x = x;
		element[i].y = y;
		element[i].scalex = sx;
		element[i].scaley = sy;
		element[i].alpha = a;
		element[i].type = type;
		element[i].subtype = subtype;
	}

	currentitem = -1;
}

// FindItemUnderMouse: given mouse coordinates, finds element number under mouse
// returns -1 if no element found
int FindItemUnderMouse(int mx, int my)
{
	int i;
	int rv;

	rv = -1;

	for (i = 0; i < numelements; i++)
	{
		if (element[i].scalex < 0)
		{
			if (element[i].x < mx)
				continue;
			if (element[i].x + element[i].scalex*drawelement[element[i].type].width > mx)
				continue;
		}
		else
		{
			if (element[i].x > mx)
				continue;
			if (element[i].x + element[i].scalex*drawelement[element[i].type].width < mx)
				continue;
		}
		if (element[i].scaley < 0)
		{
			if (element[i].y < my)
				continue;
			if (element[i].y + element[i].scaley*drawelement[element[i].type].height > my)
				continue;
		}
		else
		{
			if (element[i].y > my)
				continue;
			if (element[i].y + element[i].scaley*drawelement[element[i].type].height < my)
				continue;
		}

		rv = i;
	}

	return rv; // no element found
}

void DrawContextMenu(int mx, int my)
{
	int y;
	Draw_Colour4f(0, 0, 0, 0.4);
	Draw_Fill((mouseofsx-8)*sbarscalex, (mouseofsy-8)*sbarscaley, (float)112*sbarscalex, (float)(9*8)*sbarscaley);
	Draw_Colour4f(1,1,1,1);

	sbarminx = mouseofsx*sbarscalex;
	sbarminy = mouseofsy*sbarscaley;

	my -= mouseofsy;
	my/=8;
	my--;

	mx -= mouseofsx;

	if (mx < 0)
		my = -10;
	if (mx > 12*8)
		my = -10;

	y = 0;
	UI_DrawAltString("CONTEXT MENU", 0, y, 1);
	y+=8;
	UI_DrawAltString("------------", 0, y, 0);
	y+=8;
	UI_DrawAltString("New", 0, y, (my--)==1);
	y+=8;
	UI_DrawAltString("Snap To Grid", 0, y, (my--)==1);
	if (shiftdown)
		UI_DrawAltString("X", -8, y, (my)==1);
	y+=8;
	UI_DrawAltString("Save", 0, y, (my--)==1);
	y+=8;
	UI_DrawAltString("Reload", 0, y, (my--)==1);
	y+=8;
	UI_DrawAltString("Defaults", 0, y, (my--)==1);
	y+=8;
}

void DrawPrimaryCreationMenu(int mx, int my)
{
	int i;
	int y;
	int numopts;
	numopts = sizeof(drawelement)/sizeof(drawelement[0]);
	numopts += 2;
	Draw_Colour4f(0, 0, 0, 0.4);
	Draw_Fill((mouseofsx-8)*sbarscalex, (mouseofsy-8)*sbarscaley, (float)(17*8)*sbarscalex, (float)((numopts+2)*8)*sbarscaley);
	Draw_Colour4f(1,1,1,1);

	sbarminx = mouseofsx*sbarscalex;
	sbarminy = mouseofsy*sbarscaley;

	my -= mouseofsy;
	my/=8;
	my--;

	mx -= mouseofsx;

	if (mx < 0)
		my = -10;
	if (mx > 12*8)
		my = -10;

	y = 0;
	UI_DrawAltString("CREATE NEW ITEM", 0, y, 1);
	y+=8;
	UI_DrawAltString("------------", 0, y, 0);
	y+=8;


	for (i = 0; i < sizeof(drawelement)/sizeof(drawelement[0]); i++)
	{
		UI_DrawAltString(drawelement[i].name, 0, y, (my--)==1);
		y+=8;
	}
}

void DrawSecondaryCreationMenu(int mx, int my)
{
	int i;
	int y;
	int numopts;
	numopts = drawelement[typetoinsert].maxsubtype+1;
	numopts += 2;
	Draw_Colour4f(0, 0, 0, 0.4);
	Draw_Fill((mouseofsx-8)*sbarscalex, (mouseofsy-8)*sbarscaley, (float)(17*8)*sbarscalex, (float)((numopts+2)*8)*sbarscaley);
	Draw_Colour4f(1,1,1,1);

	sbarminx = mouseofsx*sbarscalex;
	sbarminy = mouseofsy*sbarscaley;

	my -= mouseofsy;
	my/=8;
	my--;

	mx -= mouseofsx;

	if (mx < 0)
		my = -10;
	if (mx > 12*8)
		my = -10;

	y = 0;
	UI_DrawAltString("CREATE NEW ITEM", 0, y, 1);
	y+=8;
	UI_DrawAltString("------------", 0, y, 0);
	y+=8;


	for (i = 0; i <= drawelement[typetoinsert].maxsubtype; i++)
	{
		UI_DrawAltString(drawelement[typetoinsert].subtypename[i].name, 0, y, (my--)==1);
		y+=8;
	}
}

void UI_KeyPress(int key, int mx, int my)
{
	int i;
	if (key == K_ESCAPE)
	{
		Menu_Control(0);
		return;
	}

	if (context)
	{
		if (key != K_MOUSE1)
		{
			context = false;
			return;
		}

		mx -= mouseofsx;
		if (mx < 0)
			my = -10;
		if (mx > 12*8)
			my = -10;

		i = my - mouseofsy;
		i /= 8;

		if (context == 3)
		{
			context = false;
			if ((unsigned)(i-2) > (unsigned)drawelement[typetoinsert].maxsubtype)
				return;
			currentitem = numelements;
			numelements++;

			if (typetoinsert == 12)
			{
				int j;

				// Blackness should be sent to the back
				for (j = currentitem; j > 0; j--)
				{
					element[j] = element[j-1];
				}

				currentitem = 0;
			}
			element[currentitem].type = typetoinsert;
			element[currentitem].alpha = 1;
			element[currentitem].scalex = 1;
			element[currentitem].scaley = 1;

			element[currentitem].x = 320;
			element[currentitem].y = 240;
			element[currentitem].subtype = i-2;
		}
		else if (context == 2)
		{
			typetoinsert = i-2;
			if ((unsigned)typetoinsert >= sizeof(drawelement)/sizeof(drawelement[0]))
			{
				context = false;
				return;
			}
			if (drawelement[typetoinsert].maxsubtype != 0)
				context = 3;
			else
			{
				context = false;
				currentitem = numelements;
				numelements++;

				element[currentitem].type = i-2;
				element[currentitem].alpha = 1;
				element[currentitem].scalex = 1;
				element[currentitem].scaley = 1;

				element[currentitem].x = 320;
				element[currentitem].y = 240;
				element[currentitem].subtype = 0;
			}
		}
		else
		{
			context = false;
			switch(i)
			{
			case 2:	//clone
				if (numelements==MAX_ELEMENTS)
					return;	//too many
				/*
				memcpy(element+numelements, element+currentitem, sizeof(hudelement_t));
				currentitem = numelements;
				numelements++;
				*/
				context = 2;
				break;
			case 3: //snap
				shiftdown ^= 1;
				break;
			case 4: //save
				Hud_Save(NULL);
				break;
			case 5: //reload
				Hud_Load(NULL);
				break;
			case 6: //defaults
				SBar_FlushAll();
				SBar_ReloadDefaults();
				break;
			}
		}
		return;
	}

	if (key == K_MOUSE1)
	{	//figure out which one our cursor is over...
		mousedown = false;

		i = FindItemUnderMouse(mx, my);
		if (i != -1)
		{
			int oldcurrent;
			float big;
			oldcurrent = currentitem;
			currentitem = i;

			mouseofsx = mx - element[i].x;
			mouseofsy = my - element[i].y;
			mousedown |= 1;

			if (element[i].scalex < 0)
			{
				if (mx > element[i].x+4)
					return;
			}
			else
			{
				big = element[i].scalex*drawelement[element[i].type].width;
				if (mx-element[i].x+4 < big)
					return;
			}

			if (my < element[i].y+4)
			{
				if (currentitem == oldcurrent)
					UI_KeyPress('d', 0, 0);
				return;
			}

			if (element[i].scaley < 0)
			{
				if (my > element[i].y+4)
					return;
			}
			else
			{
				big = element[i].scaley*drawelement[element[i].type].height;
				if (my-element[i].y+4 < big)
					return;
			}

			mouseofsx = mx - element[i].scalex*drawelement[element[i].type].width;
			mouseofsy = my - element[i].scaley*drawelement[element[i].type].height;
			mousedown |= 2;
		}
		else
			currentitem = -1;
	}

	// TODO: extra buttons
	// - toggle clip to edges and clip to other controls
	// - maybe toggle snap to grid instead of holding shift with mouse?

	else if (key == K_MOUSE2)
	{
		mousedown = false;	//perhaps not logically true, but it's safest this way.
		context = true;

		mouseofsx = mx;
		mouseofsy = my;
	}
	else if (key == 'n')
	{
		currentitem++;
		if (currentitem >= numelements)
			currentitem = 0;
	}
	else if (key == 'm')
	{
		currentitem--;
		if (currentitem < 0)
			currentitem = numelements ? numelements - 1 : 0;
	}
	else if (key == 'i')
	{
		if (numelements==MAX_ELEMENTS)
			return;	//too many

		element[numelements].scalex = 1;
		element[numelements].scaley = 1;
		element[numelements].alpha = 1;
		numelements++;
	}
	else if (key == K_SHIFT)
		shiftdown = true;
	else if (currentitem < numelements && currentitem != -1)
	{
		if (key == 'd')
		{
			mousedown = false;
			memcpy(element+currentitem, element+currentitem+1, sizeof(element[0]) * (numelements - currentitem-1));
			numelements--;
			currentitem = -1;
		}
		else if (key == 'c' || key == 'C')
		{
			if (numelements==MAX_ELEMENTS)
				return;	//too many
			memcpy(element+numelements, element+currentitem, sizeof(hudelement_t));
			currentitem = numelements;
			numelements++;
		}
		else if (key == K_PAGEUP)
		{	//send to back
			hudelement_t temp;

			memcpy(&temp, element+currentitem, sizeof(temp));
			memmove(element+1, element, sizeof(hudelement_t) * (currentitem));
			memcpy(element, &temp, sizeof(hudelement_t));
			currentitem = 0;
		}
		else if (key == K_PAGEDOWN)
		{	//bring to front
			hudelement_t temp;

			memcpy(&temp, element+currentitem, sizeof(temp));
			memcpy(element+currentitem, element+currentitem+1, sizeof(element[0]) * (numelements - currentitem-1));
			currentitem = numelements - 1;
			memcpy(element+currentitem , &temp, sizeof(hudelement_t));
		}
		else if (key == 'q')
		{
			element[currentitem].type--;
			if (element[currentitem].type < 0)
				element[currentitem].type = sizeof(drawelement)/sizeof(drawelement[0])-1;
		}
		else if (key == 'w')
		{
			element[currentitem].type++;
			if (element[currentitem].type >= sizeof(drawelement)/sizeof(drawelement[0]))
				element[currentitem].type = 0;
		}
		else if (key == ',' || key == K_MWHEELUP)
		{
			element[currentitem].subtype--;
			if (element[currentitem].subtype < 0)
				element[currentitem].subtype = drawelement[element[currentitem].type].maxsubtype;
		}
		else if (key == '.' || key == K_MWHEELDOWN)
		{
			element[currentitem].subtype++;
			if (element[currentitem].subtype > drawelement[element[currentitem].type].maxsubtype)
				element[currentitem].subtype = 0;
		}
		else if (key == K_UPARROW)
		{
			element[currentitem].y-=shiftdown?8:1;
		}
		else if (key == K_DOWNARROW)
		{
			element[currentitem].y+=shiftdown?8:1;
		}
		else if (key == K_LEFTARROW)
		{
			element[currentitem].x-=shiftdown?8:1;
		}
		else if (key == K_RIGHTARROW)
		{
			element[currentitem].x+=shiftdown?8:1;
		}
		else if (key == K_HOME)
		{
			element[currentitem].scalex=1.0f;
			element[currentitem].scaley=1.0f;
			element[currentitem].alpha=1.0f;
		}
		else if (key == '+')
		{
			element[currentitem].scalex*=1.1f;
			element[currentitem].scaley*=1.1f;
		}
		else if (key == '-')
		{
			element[currentitem].scalex/=1.1f;
			element[currentitem].scaley/=1.1f;
		}
	}
}

int Plug_MenuEvent(int *args)
{
	int altargs[5];
	float cursorbias;
	float cursorsize;

	args[2]=(int)(args[2]*640.0f/vid.width);
	args[3]=(int)(args[3]*480.0f/vid.height);

	switch(args[0])
	{
	case 0:	//draw

		// TODO: some sort of element property display
		if (context)
		{
		}
		else if (mousedown)
		{
			if (mousedown & 2)	//2 is 'on the scaler'
			{
				float w = args[2] - mouseofsx;
				float h = args[3] - mouseofsy;
				if (shiftdown || (mousedown & 4))	//4 is mouse2
				{
					w -= (int)w & 7;
					h -= (int)h & 7;
				}
				if (w < 8 && w > -8)
					w = 8;
				if (h < 8 && h > -8)
					h = 8;
				element[currentitem].scalex = w/drawelement[element[currentitem].type].width;
				element[currentitem].scaley = h/drawelement[element[currentitem].type].height;
			}
			else
			{
				element[currentitem].x = args[2] - mouseofsx;
				element[currentitem].y = args[3] - mouseofsy;
				if (shiftdown || (mousedown & 4))	//4 is mouse2
				{
					element[currentitem].x -= (int)element[currentitem].x & 7;
					element[currentitem].y -= (int)element[currentitem].y & 7;
				}
			}
		}
		else
			hoveritem = FindItemUnderMouse(args[2], args[3]); // this could possibly slow some things down...

		altargs[0] = 0;
		altargs[1] = 0;
		altargs[2] = 0;
		altargs[3] = vid.width;
		altargs[4] = vid.height;
		if (hudedit)
			UI_StatusBarEdit(altargs);
//		else
//			UI_StatusBar(altargs);	//draw it using the same function (we're lazy)

		if (currentitem >= 0)
			UI_DrawHandles(altargs, currentitem);

		sbarscalex = vid.width/640.0f;
		sbarscaley = vid.height/480.0f;
		switch (context)
		{
		case 1:
			DrawContextMenu(args[2], args[3]);
			break;
		case 2:
			DrawPrimaryCreationMenu(args[2], args[3]);
			break;
		case 3:
			DrawSecondaryCreationMenu(args[2], args[3]);
			break;
		default:
			break;
		}
		sbarminx = args[2];
		sbarminy = args[3];

		cursorbias = Cvar_GetFloat("cl_cursorbias");
		cursorsize = Cvar_GetFloat("cl_cursorsize");

		Draw_Colour4f(1,1,1,1);
		Draw_Image(((float)args[2]*sbarscalex)-cursorbias, ((float)args[3]*sbarscaley)-cursorbias, (float)cursorsize*sbarscalex, (float)cursorsize*sbarscaley, 0, 0, 1, 1, pic_cursor);
		break;
	case 1:	//keydown
		UI_KeyPress(args[1], args[2], args[3]);
		break;
	case 2:	//keyup
		if (args[1] == K_MOUSE1)
			mousedown = false;
		else if (args[1] == K_SHIFT)
			shiftdown = false;
		break;
	case 3:	//menu closed (this is called even if we change it).
		hudedit = false;
		break;
	case 4:	//mousemove
		break;
	}

	return 0;
}

int Plug_Tick(int *args)
{
	currenttime = args[0];
	return true;
}

int Plug_ExecuteCommand(int *args)
{
	char cmd[256];
	Cmd_Argv(0, cmd, sizeof(cmd));
	if (!strcmp("sbar_edit", cmd) || !strcmp("hud_edit", cmd))
	{
		Menu_Control(1);
		mousedown=false;
		hudedit=true;
		return 1;
	}
	if (!strcmp("sbar_save", cmd) || !strcmp("hud_save", cmd))
	{
		Cmd_Argv(1, cmd, sizeof(cmd));
		Hud_Save(cmd);
		mousedown=false;
		return 1;
	}
	if (!strcmp("sbar_load", cmd) || !strcmp("hud_load", cmd))
	{
		Cmd_Argv(1, cmd, sizeof(cmd));
		Hud_Load(cmd);
		mousedown=false;
		return 1;
	}
	if (!strcmp("sbar_defaults", cmd) || !strcmp("hud_defaults", cmd))
	{
		Cmd_Argv(1, cmd, sizeof(cmd));
		SBar_FlushAll();
		SBar_ReloadDefaults();
		mousedown=false;
		return 1;
	}
	// Modify a HUD element
	if (!strcmp("sbar", cmd) || !strcmp("hud", cmd))
	{
		// FIXME: add this command
		return 1;
	}
	// Support for KTX team overlay
	if (!strcmp("tinfo", cmd))
	{
		Hud_TeamOverlayUpdate();
		return 1;
	}
	return 0;
}

int Plug_Init(int *args)
{
	if (Plug_Export("Tick", Plug_Tick) &&
		Plug_Export("SbarBase", UI_StatusBar) &&
//		Plug_Export("SbarOverlay", UI_ScoreBoard) &&
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
		K_MWHEELDOWN	= Key_GetKeyCode("mwheeldown");
		K_MWHEELUP		= Key_GetKeyCode("mwheelup");
		K_SHIFT			= Key_GetKeyCode("shift");
		K_PAGEUP		= Key_GetKeyCode("pgup");
		K_PAGEDOWN		= Key_GetKeyCode("pgdn");

		Cmd_AddCommand("hud_edit");
		Cmd_AddCommand("sbar_edit");
		if (BUILTINISVALID(FS_Write))
		{
			Cmd_AddCommand("hud_save");
			Cmd_AddCommand("sbar_save");
		}
		if (BUILTINISVALID(FS_Read))
		{
			Cmd_AddCommand("hud_load");
			Cmd_AddCommand("sbar_load");
		}
		Cmd_AddCommand("hud_defaults");
		Cmd_AddCommand("sbar_defaults");
		
		// For modifying hud elements
		Cmd_AddCommand("hud");
		Cmd_AddCommand("sbar");

		// Teamoverlay support
		Cmd_AddCommand("tinfo");
		Cmd_AddText("newalias ktx_infoset \"cmd info ti 1\"\n", false);

		UI_SbarInit();

		if (BUILTINISVALID(FS_Read))
			Hud_Load("");

		return 1;
	}
	return 0;
}
