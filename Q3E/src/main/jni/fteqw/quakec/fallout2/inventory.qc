float IID_NONE = 0;

#define IsMelee(iid) (iid == IID_NONE || (iid >= IID_WP_TOOLKIT && iid <= IID_WP_POWERAXE))
#define IsGrenade(iid) (iid >= IID_GREN_FRAG && iid <= IID_GREN_FLASH)
#define IsRanged(iid) (iid >= IID_WP_USP && iid <= IID_WP_ROCKETLAUNCHER)

//weapons with ammo, things with a toggle state
#define NotStackable(iid) (IsRanged(iid))

//the ammoless weapons
float IID_WP_TOOLKIT = 400;
float IID_WP_KNIFE = 401;
float IID_WP_AXE = 402;
float IID_WP_VIBROBLADE = 403;
float IID_WP_POWERAXE = 404;

//the ammoed weapons
float IID_WP_USP = 405;
float IID_WP_DEAGLE = 406;
float IID_WP_NEEDLER = 407;
float IID_WP_ALIENBLASTER = 408;
float IID_WP_PIPERIFLE = 409;
float IID_WP_WINCHESTER = 410;
float IID_WP_MOSSBERG = 411;
float IID_WP_JACKHAMMER = 412;
float IID_WP_MP9 = 413;
float IID_WP_MP7 = 414;
float IID_WP_RANGEMASTER = 415;
float IID_WP_AK112 = 416;
float IID_WP_AK74 = 417;
float IID_WP_DKS1 = 418;
float IID_WP_MOONLIGHT = 419;
float IID_WP_SA80 = 420;
float IID_WP_GAUSERIFLE = 421;      //2mm EC
float IID_WP_PULSERIFLE = 422;
float IID_WP_FNFAL = 423;		//.308 AP
float IID_WP_ROCKETLAUNCHER = 430;


//and ammo for those guns
float IID_AM_NEEDLER = 507;
float IID_AM_10MM = 508; //10mm ammo";
float IID_AM_556MM = 509; //"5.56mm ammo";
float IID_AM_5MMHIGHVEL = 510; //"5mm high-velocity ammo";
float IID_AM_12GAUGESHELLS = 511; //"12-guage shotgun shells";
float IID_AM_ENERGYCELL = 512; //"small energy cell";
float IID_AM_2MMEC = 513; //12mmec ammo";
float IID_AM_762MM = 514; //"7.62mm ammo";
float IID_AM_44MAGNUM = 515; //".44 magnum ammo";
float IID_AM_45ACP = 516; //".45 ACP ammo";
float IID_AM_ROCKET = 517; //"explosive rocket"

//grenade items
float IID_GREN_FRAG = 101;
float IID_GREN_EMP = 102;
float IID_GREN_SMOKE = 103;
float IID_GREN_FLASH = 104;

//armour
#define IsArmor(iid) (iid >= IID_ARM_SHIRT && iid <= IID_ARM_LPOWER)
float IID_ARM_SHIRT = 201;
float IID_ARM_LEATHER = 202;
float IID_ARM_KEVLAR = 203;
float IID_ARM_METAL = 204;
float IID_ARM_COMBAT = 205;
float IID_ARM_BROTHERHOOD = 206;
float IID_ARM_FORCE = 207;
float IID_ARM_LPOWER = 208;

//stims
#define IsChem(iid) (iid >= IID_CHEM_STIMPACK && iid <= IID_CHEM_BESERK)
float IID_CHEM_STIMPACK = 301;
float IID_CHEM_MEDICALBAG = 302;
float IID_CHEM_SUPERSTIM = 303;
float IID_CHEM_ADRENALINE = 304;
float IID_CHEM_PSYCHO = 305;
float IID_CHEM_BESERK = 306;

float IID_BUILD_MRAMMO = 350;
float IID_BUILD_SHIELDGEN = 351;
float IID_BUILD_AUTODOC = 352;
float IID_BUILD_ROBOFANG = 353;
float IID_BUILD_TTURRET = 354;
float IID_BUILD_TELEPAD = 355;
float IID_BUILD_RTURRET = 356;
float IID_BUILD_GTURRET = 357;

float IID_EQUIP_CLIMBINGGEAR = 375;
float IID_EQUIP_STEALTHBOY = 376;
float IID_EQUIP_SPRINTKIT = 377;
float IID_EQUIP_HOVERBOOTS = 378;

#define IsShootable(iid) (IsMelee(iid) || IsRanged(iid) || IsGrenade(iid))
#define ToIID(it) floor(it/512)
#define ToStatus(it) (it&511)
#define SlotVal(iid,st) ((iid*512) | (st&511))

float(float slotno, float iid) FitsInSlot;

//slot1 and slot2 are the two hand slots
//slot3 is the armour slot.
//the other slots are for misilaneous things.

#define MAXSLOTS 16

#ifndef CSQC
.float          islot1;
.float          islot2;
.float          islot3;
.float          islot4;
.float          islot5;
.float          islot6;
.float          islot7;
.float          islot8;
.float          islot9;
.float          islot10;
.float          islot11;
.float          islot12;
.float          islot13;
.float          islot14;
.float          islot15;
.float          islot16;

void(float stnum, float sttype, .float fieldname) clientstatf = #232;
void() SetupStats =
{
	//2 is ev_float
	//32 is the first stat we are allowed to use
	clientstatf(32, 2, islot1);
	clientstatf(33, 2, islot2);
	clientstatf(34, 2, islot3);
	clientstatf(35, 2, islot4);
	clientstatf(36, 2, islot5);
	clientstatf(37, 2, islot6);
	clientstatf(38, 2, islot7);
	clientstatf(39, 2, islot8);
	clientstatf(40, 2, islot9);
	clientstatf(41, 2, islot10);
	clientstatf(42, 2, islot11);
	clientstatf(43, 2, islot12);
	clientstatf(44, 2, islot13);
	clientstatf(45, 2, islot14);
	clientstatf(46, 2, islot15);
	clientstatf(47, 2, islot16);

	clientstatf(61, 2, current_slot);
	clientstatf(62, 2, team);
	clientstatf(63, 2, class);
};

float(entity e, float slotno) ItemInSlot =
{
	if (slotno == 1)
		return e.islot1;
	if (slotno == 2)
		return e.islot2;
	if (slotno == 3)
		return e.islot3;
	if (slotno == 4)
		return e.islot4;
	if (slotno == 5)
		return e.islot5;
	if (slotno == 6)
		return e.islot6;
	if (slotno == 7)
		return e.islot7;
	if (slotno == 8)
		return e.islot8;
	if (slotno == 9)
		return e.islot9;
	if (slotno == 10)
		return e.islot10;
	if (slotno == 11)
		return e.islot11;
	if (slotno == 12)
		return e.islot12;
	if (slotno == 13)
		return e.islot13;
	if (slotno == 14)
		return e.islot14;
	if (slotno == 15)
		return e.islot15;
	if (slotno == 16)
		return e.islot16;

	return 0;
};

void(entity e, float slotno, float item) SetItemSlot =
{
	if (slotno == 1)
		e.islot1 = item;
	else if (slotno == 2)
		e.islot2 = item;
	else if (slotno == 3)
		e.islot3 = item;
	else if (slotno == 4)
		e.islot4 = item;
	else if (slotno == 5)
		e.islot5 = item;
	else if (slotno == 6)
		e.islot6 = item;
	else if (slotno == 7)
		e.islot7 = item;
	else if (slotno == 8)
		e.islot8 = item;
	else if (slotno == 9)
		e.islot9 = item;
	else if (slotno == 10)
		e.islot10 = item;
	else if (slotno == 11)
		e.islot11 = item;
	else if (slotno == 12)
		e.islot12 = item;
	else if (slotno == 13)
		e.islot13 = item;
	else if (slotno == 14)
		e.islot14 = item;
	else if (slotno == 15)
		e.islot15 = item;
	else if (slotno == 16)
		e.islot16 = item;
};

float(entity e, float iid) SlotOfItem =
{
	if (ToIID(e.islot1) == iid)
		return 1;
	if (ToIID(e.islot2) == iid)
		return 2;
	if (ToIID(e.islot3) == iid)
		return 3;
	if (ToIID(e.islot4) == iid)
		return 4;
	if (ToIID(e.islot5) == iid)
		return 5;
	if (ToIID(e.islot6) == iid)
		return 6;
	if (ToIID(e.islot7) == iid)
		return 7;
	if (ToIID(e.islot8) == iid)
		return 8;
	if (ToIID(e.islot9) == iid)
		return 9;
	if (ToIID(e.islot10) == iid)
		return 10;
	if (ToIID(e.islot11) == iid)
		return 11;
	if (ToIID(e.islot12) == iid)
		return 12;
	if (ToIID(e.islot13) == iid)
		return 13;
	if (ToIID(e.islot14) == iid)
		return 14;
	if (ToIID(e.islot15) == iid)
		return 15;
	if (ToIID(e.islot16) == iid)
		return 16;

	return 0;
};

float(entity e, float iid) TotalQuantity =
{	//iid must be stackable.
	local float ret;
	if (ToIID(e.islot1) == iid)
		ret += ToStatus(e.islot1);
	if (ToIID(e.islot2) == iid)
		ret += ToStatus(e.islot2);
	if (ToIID(e.islot3) == iid)
		ret += ToStatus(e.islot3);
	if (ToIID(e.islot4) == iid)
		ret += ToStatus(e.islot4);
	if (ToIID(e.islot5) == iid)
		ret += ToStatus(e.islot5);
	if (ToIID(e.islot6) == iid)
		ret += ToStatus(e.islot6);
	if (ToIID(e.islot7) == iid)
		ret += ToStatus(e.islot7);
	if (ToIID(e.islot8) == iid)
		ret += ToStatus(e.islot8);
	if (ToIID(e.islot9) == iid)
		ret += ToStatus(e.islot9);
	if (ToIID(e.islot10) == iid)
		ret += ToStatus(e.islot10);
	if (ToIID(e.islot11) == iid)
		ret += ToStatus(e.islot11);
	if (ToIID(e.islot12) == iid)
		ret += ToStatus(e.islot12);
	if (ToIID(e.islot13) == iid)
		ret += ToStatus(e.islot13);
	if (ToIID(e.islot14) == iid)
		ret += ToStatus(e.islot14);
	if (ToIID(e.islot15) == iid)
		ret += ToStatus(e.islot15);
	if (ToIID(e.islot16) == iid)
		ret += ToStatus(e.islot16);

	return ret;
};

float(entity e) FindEmptySlot =
{
	if (ToIID(e.islot3) == IID_NONE)
		return 3;
	if (ToIID(e.islot4) == IID_NONE)
		return 4;
	if (ToIID(e.islot5) == IID_NONE)
		return 5;
	if (ToIID(e.islot6) == IID_NONE)
		return 6;
	if (ToIID(e.islot7) == IID_NONE)
		return 7;
	if (ToIID(e.islot8) == IID_NONE)
		return 8;
	if (ToIID(e.islot9) == IID_NONE)
		return 9;
	if (ToIID(e.islot10) == IID_NONE)
		return 10;
	if (ToIID(e.islot11) == IID_NONE)
		return 11;
	if (ToIID(e.islot12) == IID_NONE)
		return 12;
	if (ToIID(e.islot13) == IID_NONE)
		return 13;
	if (ToIID(e.islot14) == IID_NONE)
		return 14;
	if (ToIID(e.islot15) == IID_NONE)
		return 15;
	if (ToIID(e.islot16) == IID_NONE)
		return 16;

	return 0;
};

float(entity e, float iid) FindSuitableEmptySlot =
{
	if (ToIID(e.islot1) == IID_NONE)
		if (FitsInSlot(1, iid))
			return 1;
	if (ToIID(e.islot2) == IID_NONE)
		if (FitsInSlot(2, iid))
			return 2;
	if (ToIID(e.islot3) == IID_NONE)
		if (FitsInSlot(3, iid))
			return 3;
	if (ToIID(e.islot4) == IID_NONE)
		if (FitsInSlot(4, iid))
			return 4;
	if (ToIID(e.islot5) == IID_NONE)
		if (FitsInSlot(5, iid))
			return 5;
	if (ToIID(e.islot6) == IID_NONE)
		if (FitsInSlot(6, iid))
			return 6;
	if (ToIID(e.islot7) == IID_NONE)
		if (FitsInSlot(7, iid))
			return 7;
	if (ToIID(e.islot8) == IID_NONE)
		if (FitsInSlot(8, iid))
			return 8;
	if (ToIID(e.islot9) == IID_NONE)
		if (FitsInSlot(9, iid))
			return 9;
	if (ToIID(e.islot10) == IID_NONE)
		if (FitsInSlot(10, iid))
			return 10;
	if (ToIID(e.islot11) == IID_NONE)
		if (FitsInSlot(11, iid))
			return 11;
	if (ToIID(e.islot12) == IID_NONE)
		if (FitsInSlot(12, iid))
			return 12;
	if (ToIID(e.islot13) == IID_NONE)
		if (FitsInSlot(13, iid))
			return 13;
	if (ToIID(e.islot14) == IID_NONE)
		if (FitsInSlot(14, iid))
			return 14;
	if (ToIID(e.islot15) == IID_NONE)
		if (FitsInSlot(15, iid))
			return 15;
	if (ToIID(e.islot16) == IID_NONE)
		if (FitsInSlot(16, iid))
			return 16;

	return 0;
};

typedef .float slot_t;
slot_t(float slot) SlotField =
{
	if (slot == 1)
		return islot1;
	if (slot == 2)
		return islot2;
	if (slot == 3)
		return islot3;
	if (slot == 4)
		return islot4;
	if (slot == 5)
		return islot5;
	if (slot == 6)
		return islot6;
	if (slot == 7)
		return islot7;
	if (slot == 8)
		return islot8;
	if (slot == 9)
		return islot9;
	if (slot == 10)
		return islot10;
	if (slot == 11)
		return islot11;
	if (slot == 12)
		return islot12;
	if (slot == 13)
		return islot13;
	if (slot == 14)
		return islot14;
	if (slot == 15)
		return islot15;
	if (slot == 16)
		return islot16;


	bprint(PRINT_MEDIUM, "ERROR: Invalid slot number (", ftos(slot), ")\n");
	return islot1;
};
#endif


string(float iid) GetItemVModel =
{
	if (iid == IID_NONE)
		return "progs/v_fist.mdl";
	if (iid == IID_WP_TOOLKIT)
		return "progs/v_span.mdl";
	if (iid == IID_WP_KNIFE)
		return "progs/v_knife.mdl";
	if (iid == IID_WP_AXE)
		return "progs/v_axe.mdl";
	if (iid == IID_WP_VIBROBLADE)
		return "progs/v_knife.mdl";
	if (iid == IID_WP_POWERAXE)
		return "progs/v_axe.mdl";
	if (iid == IID_WP_USP)
		return "progs/v_1911.mdl";

	if (iid == IID_WP_DEAGLE)
		return "progs/v_deagle.mdl";
	if (iid == IID_WP_NEEDLER)
		return "progs/v_1911.mdl";
	if (iid == IID_WP_ALIENBLASTER)
		return "progs/v_alien.mdl";
	if (iid == IID_WP_PIPERIFLE)
		return "progs/v_piperifle.mdl";
	if (iid == IID_WP_WINCHESTER)
		return "progs/v_double.mdl";
	if (iid == IID_WP_MOSSBERG)
		return "progs/v_shotgun.mdl";

	if (iid == IID_WP_JACKHAMMER)
		return "progs/v_jackhammer.mdl";
	if (iid == IID_WP_MP9)
		return "progs/v_mp9.mdl";
	if (iid == IID_WP_MP7)
		return "progs/v_smg.mdl";
	if (iid == IID_WP_RANGEMASTER)
 		return "progs/v_rangem.mdl";
	if (iid == IID_WP_AK112)
		return "progs/v_ak47.mdl";
	if (iid == IID_WP_AK74)
		return "progs/v_ak47.mdl";
	if (iid == IID_WP_DKS1)
		return "progs/v_srifle.mdl";
	if (iid == IID_WP_MOONLIGHT)
		return "progs/v_night.mdl";
	if (iid == IID_WP_FNFAL)
 		return "progs/v_rangem.mdl";


	if (iid == IID_WP_SA80)
		return "progs/v_sa80.mdl";
	if (iid == IID_WP_GAUSERIFLE)
		return "progs/v_gauss.mdl";
	if (iid == IID_WP_PULSERIFLE)
		return "progs/v_carbine.mdl";

	if (iid == IID_WP_ROCKETLAUNCHER)
		return "progs/v_rocket.mdl";

	if (iid == IID_GREN_FRAG)
		return "progs/v_handgren.mdl";
	if (iid == IID_GREN_EMP)
		return "progs/v_handgren.mdl";
	if (iid == IID_GREN_SMOKE)
		return "progs/v_handgren.mdl";
	if (iid == IID_GREN_FLASH)
		return "progs/v_handgren.mdl";

	bprint(PRINT_MEDIUM, ftos(iid), " without a vmodel!\n");
	return "";
};

string(float iid) GetItemWModel =
{
	if (iid == IID_WP_KNIFE)
		return "progs/w_knife.mdl";
	if (iid == IID_WP_AXE)
		return "progs/w_knife.mdl";
	if (iid == IID_WP_VIBROBLADE)
		return "progs/w_axe.mdl";
	if (iid == IID_WP_POWERAXE)
		return "progs/w_axe.mdl";

	if (iid == IID_WP_USP)
		return "progs/w_1911.mdl";
	if (iid == IID_WP_DEAGLE)
		return "progs/w_deagle.mdl";
	if (iid == IID_WP_NEEDLER)
		return "progs/w_1911.mdl";
	if (iid == IID_WP_ALIENBLASTER)
		return "progs/w_alien.mdl";
	if (iid == IID_WP_PIPERIFLE)
		return "progs/w_pipe.mdl";
	if (iid == IID_WP_WINCHESTER)
		return "progs/w_shotgun.mdl";
	if (iid == IID_WP_MOSSBERG)
		return "progs/w_pipe.mdl";
	if (iid == IID_WP_JACKHAMMER)
		return "progs/w_jackhammer.mdl";

	if (iid == IID_WP_MP9)
		return "progs/w_mp9.mdl";
	if (iid == IID_WP_MP7)
		return "progs/w_mp7.mdl";
	if (iid == IID_WP_RANGEMASTER)
		return "progs/w_rangem.mdl";
	if (iid == IID_WP_AK112)
		return "progs/w_ak47.mdl";
	if (iid == IID_WP_AK74)
		return "progs/w_ak47.mdl";
	if (iid == IID_WP_DKS1)
		return "progs/w_srifle.mdl";
	if (iid == IID_WP_MOONLIGHT)
		return "progs/w_night.mdl";
	if (iid == IID_WP_SA80)
		return "progs/w_sa80.mdl";
	if (iid == IID_WP_FNFAL)
		return "progs/w_rangem.mdl";

	if (iid == IID_WP_GAUSERIFLE)
		return "progs/w_gauss.mdl";
	if (iid == IID_WP_PULSERIFLE)
		return "progs/w_carbine.mdl";
	if (iid == IID_WP_ROCKETLAUNCHER)
		return "progs/w_rocket.mdl";

	if (iid == IID_GREN_FRAG)
		return "progs/grenade2.mdl";
	if (iid == IID_GREN_EMP)
		return "progs/grenade2.mdl";
	if (iid == IID_GREN_SMOKE)
		return "progs/grenade2.mdl";
	if (iid == IID_GREN_FLASH)
		return "progs/grenade2.mdl";

	if (iid == IID_CHEM_STIMPACK)
		return "maps/b_bh10.bsp";
	if (iid == IID_CHEM_MEDICALBAG)
		return "maps/b_bh25.bsp";
	if (iid == IID_CHEM_SUPERSTIM)
		return "maps/b_bh100.bsp";

//fixme
	if (iid == IID_CHEM_ADRENALINE)
		return "maps/b_bh10.bsp";
	if (iid == IID_CHEM_PSYCHO)
		return "maps/b_bh25.bsp";
	if (iid == IID_CHEM_BESERK)
		return "maps/b_bh100.bsp";


	return "progs/s_light.spr";	//no model. :/
};

float(float iid) WeaponAmmoType =
{
	if (iid == IID_WP_USP)
		return IID_AM_10MM;
	if (iid == IID_WP_DEAGLE)
		return IID_AM_44MAGNUM;
	if (iid == IID_WP_NEEDLER)
		return IID_AM_NEEDLER;
	if (iid == IID_WP_ALIENBLASTER)
		return IID_AM_ENERGYCELL;
	if (iid == IID_WP_PIPERIFLE)
		return IID_AM_10MM;
	if (iid == IID_WP_WINCHESTER)
		return IID_AM_12GAUGESHELLS;
	if (iid == IID_WP_MOSSBERG)
		return IID_AM_12GAUGESHELLS;
	if (iid == IID_WP_JACKHAMMER)
		return IID_AM_12GAUGESHELLS;
	if (iid == IID_WP_MP9)
		return IID_AM_10MM;
	if (iid == IID_WP_MP7)
		return IID_AM_10MM;
	if (iid == IID_WP_RANGEMASTER)
		return IID_AM_556MM;
	if (iid == IID_WP_AK112)
		return IID_AM_5MMHIGHVEL;
	if (iid == IID_WP_AK74)
		return IID_AM_5MMHIGHVEL;
	if (iid == IID_WP_DKS1)
		return IID_AM_762MM;
	if (iid == IID_WP_MOONLIGHT)
		return IID_AM_556MM;
	if (iid == IID_WP_SA80)
		return IID_AM_556MM;
	if (iid == IID_WP_FNFAL)
		return IID_AM_762MM;

	if (iid == IID_WP_GAUSERIFLE)
		return IID_AM_2MMEC;
	if (iid == IID_WP_PULSERIFLE)
		return IID_AM_ENERGYCELL;
	if (iid == IID_WP_ROCKETLAUNCHER)
		return IID_AM_ROCKET;

	return IID_NONE;
};

float(float iid) WeaponMagQuant =
{
	if (iid == IID_WP_USP)
		return 12;
	if (iid == IID_WP_DEAGLE)
		return 7;
	if (iid == IID_WP_NEEDLER)
		return 15;
	if (iid == IID_WP_ALIENBLASTER)
		return 6;
	if (iid == IID_WP_PIPERIFLE)
		return 1;
	if (iid == IID_WP_WINCHESTER)
		return 2;
	if (iid == IID_WP_MOSSBERG)
		return 6;
	if (iid == IID_WP_JACKHAMMER)
		return 10;
	if (iid == IID_WP_MP9)
		return 30;
	if (iid == IID_WP_MP7)
		return 30;
	if (iid == IID_WP_RANGEMASTER)
		return 10;
	if (iid == IID_WP_AK112)
		return 24;
	if (iid == IID_WP_AK74)
		return 30;
	if (iid == IID_WP_DKS1)
		return 8;
	if (iid == IID_WP_MOONLIGHT)
		return 30;
	if (iid == IID_WP_SA80)
		return 30;
	if (iid == IID_WP_GAUSERIFLE)
		return 10;
	if (iid == IID_WP_PULSERIFLE)
		return 40;
	if (iid == IID_WP_FNFAL)
		return 20;
	if (iid == IID_WP_ROCKETLAUNCHER)
		return 1;

	return 0;
};

float(float iid) GetItemWeight =
{
	if (iid == IID_NONE)
		return 0;
	if (iid == IID_WP_KNIFE)
		return 1;
	if (iid == IID_WP_AXE)
		return 2;
	if (iid == IID_WP_VIBROBLADE)
		return 8;
	if (iid == IID_WP_POWERAXE)
		return 6;
	if (iid == IID_WP_USP)
		return 1;
	if (iid == IID_WP_DEAGLE)
		return 2;
	if (iid == IID_WP_NEEDLER)
		return 2;
	if (iid == IID_WP_ALIENBLASTER)
		return 2;
	if (iid == IID_WP_PIPERIFLE)
		return 3;
	if (iid == IID_WP_WINCHESTER)
		return 4;
	if (iid == IID_WP_MOSSBERG)
		return 5;
	if (iid == IID_WP_JACKHAMMER)
		return 6;
	if (iid == IID_WP_MP9)
		return 3;
	if (iid == IID_WP_MP7)
		return 3;
	if (iid == IID_WP_RANGEMASTER)
		return 5;
	if (iid == IID_WP_AK112)
		return 5;
	if (iid == IID_WP_AK74)
		return 5;
	if (iid == IID_WP_DKS1)
		return 7;
	if (iid == IID_WP_MOONLIGHT)
		return 5;
	if (iid == IID_WP_SA80)
		return 5;
	if (iid == IID_WP_GAUSERIFLE)
		return 9;
	if (iid == IID_WP_PULSERIFLE)
		return 12;
	if (iid == IID_WP_FNFAL)
		return 9;
	if (iid == IID_WP_ROCKETLAUNCHER)
		return 10;


	if (iid == IID_ARM_SHIRT)
		return 3;
	if (iid == IID_ARM_LEATHER)
		return 5;
	if (iid == IID_ARM_KEVLAR)
		return 9;
	if (iid == IID_ARM_METAL)
		return 15;
	if (iid == IID_ARM_COMBAT)
		return 12;
	if (iid == IID_ARM_BROTHERHOOD)
		return 17;
	if (iid == IID_ARM_FORCE)
		return 6;
	if (iid == IID_ARM_LPOWER)
		return 20;

	if (iid == IID_BUILD_MRAMMO)
		return 2;
	if (iid == IID_BUILD_SHIELDGEN)
		return 4;
	if (iid == IID_BUILD_AUTODOC)
		return 3;
	if (iid == IID_BUILD_ROBOFANG)
		return 2;
	if (iid == IID_BUILD_TTURRET)
		return 3;
	if (iid == IID_BUILD_GTURRET)
		return 3;
	if (iid == IID_BUILD_RTURRET)
		return 3;


//	bprint(PRINT_MEDIUM, ftos(iid), " without a weight!\n");
	return 0;
};

float(float item) GetItemsWeight =
{
	local float iid;
	iid = ToIID(item);
	if (NotStackable(iid))
		return GetItemWeight(iid);
	else
		return GetItemWeight(iid) * ToStatus(item);
};


string(float iid) GetItemName =
{
	if (iid == IID_NONE)
		return "nothing";


	if (iid == IID_WP_TOOLKIT)
		return "toolkit";
	if (iid == IID_WP_KNIFE)
		return "knife";
	if (iid == IID_WP_AXE)
		return "axe";
	if (iid == IID_WP_VIBROBLADE)
		return "ripper";
	if (iid == IID_WP_POWERAXE)
		return "poweraxe";

	if (iid == IID_WP_USP)
		return "1911 (10mm)";
	if (iid == IID_WP_DEAGLE)
		return "desert eagle (.44mag)";
	if (iid == IID_WP_NEEDLER)
		return "needler";
	if (iid == IID_WP_ALIENBLASTER)
		return "alien blaster (energy)";
	if (iid == IID_WP_PIPERIFLE)
		return "pipe rifle (10mm)";
	if (iid == IID_WP_WINCHESTER)
		return "winchester (12g)";
	if (iid == IID_WP_MOSSBERG)
		return "mossberg (12g)";
	if (iid == IID_WP_JACKHAMMER)
		return "jackhammer (12g)";
	if (iid == IID_WP_MP9)
		return "mp9 (10mm)";
	if (iid == IID_WP_MP7)
		return "grease gun (10mm)";
	if (iid == IID_WP_RANGEMASTER)
		return "rangemaster (5.56mm)";
	if (iid == IID_WP_AK112)
		return "ak-112 (5mm-hv)";
	if (iid == IID_WP_AK74)
		return "ak-74 (5mm-hv)";
	if (iid == IID_WP_DKS1)
		return "dks-1 (7.62mm)";
	if (iid == IID_WP_MOONLIGHT)
		return "moonlight (5.56mm)";
	if (iid == IID_WP_SA80)
		return "sa-80 (5.56mm)";
	if (iid == IID_WP_GAUSERIFLE)
		return "gauss rifle (2mm EC)";
	if (iid == IID_WP_PULSERIFLE)
		return "laser carbine (energy)";
	if (iid == IID_WP_FNFAL)
		return "fn-fal (7.62mm)";
	if (iid == IID_WP_ROCKETLAUNCHER)
		return "rocket launcher";



	if (iid == IID_AM_NEEDLER)
		return "needler cartidge";
	if (iid == IID_AM_2MMEC)
		return "2mm EC ammo";
	if (iid == IID_AM_10MM)
		return "10mm FMJ";
	if (iid == IID_AM_556MM)
		return "5.56mm FMJ";
	if (iid == IID_AM_5MMHIGHVEL)
		return "5mm JHP";
	if (iid == IID_AM_12GAUGESHELLS)
		return "12-guage shotgun shells";
	if (iid == IID_AM_ENERGYCELL)
		return "small energy cell";
	if (iid == IID_AM_762MM)
		return "7.62mm AP";
	if (iid == IID_AM_44MAGNUM)
		return ".44 magnum ammo";
	if (iid == IID_AM_45ACP)
		return ".45 ACP ammo";
	if (iid == IID_AM_ROCKET)
		return "explosive rocket";


	if (iid == IID_GREN_FRAG)
		return "frag grenade";
	if (iid == IID_GREN_EMP)
		return "emp grenade";
	if (iid == IID_GREN_SMOKE)
		return "smoke grenade";
	if (iid == IID_GREN_FLASH)
		return "flashbang";


	if (iid == IID_ARM_SHIRT)
		return "bulletproof shirt";
	if (iid == IID_ARM_LEATHER)
		return "leather armor";
	if (iid == IID_ARM_KEVLAR)
		return "kevlar armor";
	if (iid == IID_ARM_METAL)
		return "metal armor";
	if (iid == IID_ARM_COMBAT)
		return "combat armor";
	if (iid == IID_ARM_BROTHERHOOD)
		return "brotherhood armor";
	if (iid == IID_ARM_FORCE)
		return "force armor";
	if (iid == IID_ARM_LPOWER)
		return "light power armor";


	if (iid == IID_CHEM_STIMPACK)
		return "stimpack";
	if (iid == IID_CHEM_MEDICALBAG)
		return "medical bag";
	if (iid == IID_CHEM_SUPERSTIM)
		return "superstim";
	if (iid == IID_CHEM_ADRENALINE)
		return "adrenaline";
	if (iid == IID_CHEM_PSYCHO)
		return "psycho";
	if (iid == IID_CHEM_BESERK)
		return "beserk";


	if (iid == IID_BUILD_MRAMMO)
		return "mr. ammo";
	if (iid == IID_BUILD_SHIELDGEN)
		return "shield-gen";
	if (iid == IID_BUILD_AUTODOC)
		return "auto-doc";
	if (iid == IID_BUILD_ROBOFANG)
		return "robofang";
	if (iid == IID_BUILD_TTURRET)
		return "tesla turret";
	if (iid == IID_BUILD_RTURRET)
		return "rocket turret";
	if (iid == IID_BUILD_GTURRET)
		return "machine-gun turret";

	if (iid == IID_EQUIP_CLIMBINGGEAR)
		return "climbing gear";
	if (iid == IID_EQUIP_STEALTHBOY)
		return "stealthboy";
	if (iid == IID_EQUIP_SPRINTKIT)
		return "lucozade sport!";
	if (iid == IID_EQUIP_HOVERBOOTS)
		return "hover boots";

	bprint(PRINT_MEDIUM, ftos(iid), " without a name!\n");
	return strcat("unknown", ftos(iid));
};

string(float iid) GetItemDesc =
{
	if (iid == IID_NONE)
		return "";


	if (iid == IID_WP_TOOLKIT)
		return "a wattz(tm) brand toolkit consisting of many handy instruments, including a wrench. this kit is a repairman's dream, and allows those with the proper knowledge to perform technical tasks.";
	if (iid == IID_WP_AK74)
		return "the ak74 is the predecessor of the legendary ak-47. it is still the same old killing machine, except that it uses the 5.45mm high-velocity round that was common in the latest assault rifles just before the war.";
	if (iid == IID_BUILD_MRAMMO)
		return "a military prototype, the mr. ammo was designed to allow soldiers an extra source of ammunition in remote areas, as the handy little machine is capable of churning out ammo from things such as rocks, debris, and metal scraps.";
	if (iid == IID_EQUIP_CLIMBINGGEAR)
		return "FIXME:\nActivate, and then run at and up a wall. Don't look too far away from the wall!";

	bprint(PRINT_MEDIUM, ftos(iid), " without a desc!\n");
	return strcat("<no description>", ftos(iid));
};

string(float iid) GetItemImage =
{
	if (iid == IID_NONE)
		return "blank.jpg";


	if (iid == IID_WP_TOOLKIT)
		return "toolkit.jpg";
	if (iid == IID_WP_KNIFE)
		return "knife.jpg";
	if (iid == IID_WP_AXE)
		return "axe.jpg";
	if (iid == IID_WP_VIBROBLADE)
		return "ripper.jpg";
	if (iid == IID_WP_POWERAXE)
		return "poweraxe.jpg";

	if (iid == IID_WP_USP)
		return "usp.jpg";
	if (iid == IID_WP_DEAGLE)
		return "deagle.jpg";
	if (iid == IID_WP_NEEDLER)
		return "needler.jpg";
	if (iid == IID_WP_ALIENBLASTER)
		return "blaster.jpg";
	if (iid == IID_WP_PIPERIFLE)
		return "prifle.jpg";
	if (iid == IID_WP_WINCHESTER)
		return "double.jpg";
	if (iid == IID_WP_MOSSBERG)
		return "mossberg.jpg";
	if (iid == IID_WP_JACKHAMMER)
		return "jackhammer.jpg";
	if (iid == IID_WP_MP9)
		return "mp9.jpg";
	if (iid == IID_WP_MP7)
		return "ggun.jpg";
	if (iid == IID_WP_RANGEMASTER)
		return "rangem.jpg";
	if (iid == IID_WP_AK112)
		return "ak112.jpg";
	if (iid == IID_WP_AK74)
		return "ak74.jpg";
	if (iid == IID_WP_DKS1)
		return "dks1.jpg";
	if (iid == IID_WP_MOONLIGHT)
		return "moonlight.jpg";
	if (iid == IID_WP_SA80)
		return "sa80.jpg";
	if (iid == IID_WP_GAUSERIFLE)
		return "grifle.jpg";
	if (iid == IID_WP_PULSERIFLE)
		return "lcarbine.jpg";
	if (iid == IID_WP_FNFAL)
		return "fnfal.jpg";
	if (iid == IID_WP_ROCKETLAUNCHER)
		return "rpg.jpg";


	if (iid == IID_AM_NEEDLER)
		return "needles.jpg";
	if (iid == IID_AM_2MMEC)
		return "2mm.jpg";
	if (iid == IID_AM_10MM)
		return "10mm.jpg";
	if (iid == IID_AM_556MM)
		return "556mm.jpg";
	if (iid == IID_AM_5MMHIGHVEL)
		return "5mmhv.jpg";
	if (iid == IID_AM_12GAUGESHELLS)
		return "12gauge.jpg";
	if (iid == IID_AM_ENERGYCELL)
		return "cell.jpg";
	if (iid == IID_AM_762MM)
		return "762mm.jpg";
	if (iid == IID_AM_44MAGNUM)
		return "44mag.jpg";
	if (iid == IID_AM_45ACP)
		return "45acp.jpg";
	if (iid == IID_AM_ROCKET)
		return "rocket.jpg";


	if (iid == IID_GREN_FRAG)
		return "frag.jpg";
	if (iid == IID_GREN_EMP)
		return "emp.jpg";
	if (iid == IID_GREN_SMOKE)
		return "smoke.jpg";
	if (iid == IID_GREN_FLASH)
		return "flash.jpg";


	if (iid == IID_ARM_SHIRT)
		return "shirt.jpg";
	if (iid == IID_ARM_LEATHER)
		return "leather.jpg";
	if (iid == IID_ARM_KEVLAR)
		return "kevlar.jpg";
	if (iid == IID_ARM_METAL)
		return "metal.jpg";
	if (iid == IID_ARM_COMBAT)
		return "combat.jpg";
	if (iid == IID_ARM_BROTHERHOOD)
		return "brotherhood.jpg";
	if (iid == IID_ARM_FORCE)
		return "force.jpg";
	if (iid == IID_ARM_LPOWER)
		return "power.jpg";


	if (iid == IID_CHEM_STIMPACK)
		return "stimpack.jpg";
	if (iid == IID_CHEM_MEDICALBAG)
		return "medbag.jpg";
	if (iid == IID_CHEM_SUPERSTIM)
		return "superstim.jpg";
	if (iid == IID_CHEM_ADRENALINE)
		return "adrenaline.jpg";
	if (iid == IID_CHEM_PSYCHO)
		return "psycho.jpg";
	if (iid == IID_CHEM_BESERK)
		return "beserk.jpg";


	if (iid == IID_BUILD_MRAMMO)
		return "mrammo.jpg";
	if (iid == IID_BUILD_SHIELDGEN)
		return "shieldgen.jpg";
	if (iid == IID_BUILD_AUTODOC)
		return "autodoc.jpg";
	if (iid == IID_BUILD_ROBOFANG)
		return "robofang.jpg";
	if (iid == IID_BUILD_TTURRET)
		return "tesla.jpg";
	if (iid == IID_BUILD_RTURRET)
		return "rturret.jpg";
	if (iid == IID_BUILD_GTURRET)
		return "gturret.jpg";

	if (iid == IID_EQUIP_CLIMBINGGEAR)
		return "robofang.jpg";
	if (iid == IID_EQUIP_STEALTHBOY)
		return "stealthboy.jpg";
	if (iid == IID_EQUIP_SPRINTKIT)
		return "sprintkit.jpg";

	bprint(PRINT_MEDIUM, ftos(iid), " without a name!\n");
	return strcat("unknown.jpg", ftos(iid));
};


float(string itname) ItemIDOfName = 
{
	if (itname == "nothing")
		return IID_NONE;
	if (itname == "toolkit")
		return IID_WP_TOOLKIT;
	if (itname == "knife")
		return IID_WP_KNIFE;
	if (itname == "axe")
		return IID_WP_AXE;
	if (itname == "ripper")
		return IID_WP_VIBROBLADE;
	if (itname == "poweraxe")
		return IID_WP_POWERAXE;
	if (itname == "1911")
		return IID_WP_USP;
	if (itname == "desert eagle")
		return IID_WP_DEAGLE;
	if (itname == "deagle")
		return IID_WP_DEAGLE;
	if (itname == "needler")
		return IID_WP_NEEDLER;
	if (itname == "alien blaster")
		return IID_WP_ALIENBLASTER;
	if (itname == "blaster")
		return IID_WP_ALIENBLASTER;
	if (itname == "pipe rifle")
		return IID_WP_PIPERIFLE;
	if (itname == "winchester")
		return IID_WP_WINCHESTER;
	if (itname == "mossberg")
		return IID_WP_MOSSBERG;
	if (itname == "jackhammer")
		return IID_WP_JACKHAMMER;
	if (itname == "mp9")
		return IID_WP_MP9;
	if (itname == "grease gun")
		return IID_WP_MP7;
	if (itname == "rangemaster")
		return IID_WP_RANGEMASTER;
	if (itname == "fnfal")
		return IID_WP_FNFAL;
	if (itname == "ak-112")
		return IID_WP_AK112;
	if (itname == "ak-74")
		return IID_WP_AK74;
	if (itname == "dks-1")
		return IID_WP_DKS1;
	if (itname == "moonlight")
		return IID_WP_MOONLIGHT;
	if (itname == "sa-80")
		return IID_WP_SA80;
	if (itname == "gauss rifle")
		return IID_WP_GAUSERIFLE;
	if (itname == "laser carbine")
		return IID_WP_PULSERIFLE;
	if (itname == "rocket")
		return IID_WP_ROCKETLAUNCHER;
	if (itname == "frag grenade")
		return IID_GREN_FRAG;
	if (itname == "emp grenade")
		return IID_GREN_EMP;
	if (itname == "smoke grenade")
		return IID_GREN_SMOKE;
	if (itname == "flashbang")
		return IID_GREN_FLASH;
	if (itname == "bulletproof shirt")
		return IID_ARM_SHIRT;
	if (itname == "leather armor")
		return IID_ARM_LEATHER;
	if (itname == "kevlar armor")
		return IID_ARM_KEVLAR;
	if (itname == "metal armor")
		return IID_ARM_METAL;
	if (itname == "combat armor")
		return IID_ARM_COMBAT;
	if (itname == "brotherhood armor")
		return IID_ARM_BROTHERHOOD;
	if (itname == "force armor")
		return IID_ARM_FORCE;
	if (itname == "light power armor")
		return IID_ARM_LPOWER;
	if (itname == "stimpacks")
		return IID_CHEM_STIMPACK;
	if (itname == "stimpack")
		return IID_CHEM_STIMPACK;
	if (itname == "medical bag")
		return IID_CHEM_MEDICALBAG;
	if (itname == "bandages")
		return IID_CHEM_MEDICALBAG;
	if (itname == "superstims")
		return IID_CHEM_SUPERSTIM;
	if (itname == "adrenaline")
		return IID_CHEM_ADRENALINE;
	if (itname == "psycho")
		return IID_CHEM_PSYCHO;
	if (itname == "beserk")
		return IID_CHEM_BESERK;
	if (itname == "mr.ammo")
		return IID_BUILD_MRAMMO;
	if (itname == "mrammo")
		return IID_BUILD_MRAMMO;
	if (itname == "shield-gen")
		return IID_BUILD_SHIELDGEN;
	if (itname == "shieldgen")
		return IID_BUILD_SHIELDGEN;
	if (itname == "auto-doc")
		return IID_BUILD_AUTODOC;
	if (itname == "autodoc")
		return IID_BUILD_AUTODOC;
	if (itname == "robofang")
		return IID_BUILD_ROBOFANG;
	if (itname == "doggie")
		return IID_BUILD_ROBOFANG;
	if (itname == "climbinggear")
		return IID_EQUIP_CLIMBINGGEAR;
	if (itname == "gear")
		return IID_EQUIP_CLIMBINGGEAR;
	if (itname == "stealthboy")
		return IID_EQUIP_STEALTHBOY;

	if (itname == "sprintkit")
		return IID_EQUIP_SPRINTKIT;

	if (itname == "tesla")
		return IID_BUILD_TTURRET;

	return 0;
};



float(float slotno, float iid) FitsInSlot =
{
	if (!iid)	//nothing can be put in every slot.
		return true;
	if (slotno == 1 || slotno == 2)
		return IsShootable(iid);
	if (slotno == 3)
		return IsArmor(iid);
	return true;
};