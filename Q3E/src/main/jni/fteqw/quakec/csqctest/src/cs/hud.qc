
string number[10] = {
	"num_0",
	"num_1",
	"num_2",
	"num_3",
	"num_4",
	"num_5",
	"num_6",
	"num_7",
	"num_8",
	"num_9"
};
string anumber[10] = {
	"anum_0",
	"anum_1",
	"anum_2",
	"anum_3",
	"anum_4",
	"anum_5",
	"anum_6",
	"anum_7",
	"anum_8",
	"anum_9"
};
void() Hud_Init
{
	float i;
	precache_pic("sbar", true);
	for (i = 0; i < 10; i++)
	{
		precache_pic(number[i], true);
		precache_pic(anumber[i], true);
	}
};

vector screensize;
float stat_items, stat_items2;

void(vector pos, float value, float threshhold) Hud_DrawLargeValue =
{
	float c;
	float len;
	string s;
	if (value < 0)
		value = 0;	//hrm
	if (value>999)
		value = 999;

	s = ftos(floor(value));
	len = strlen(s);

	pos_x += 24 * (3-len);
	

	if (value <= threshhold)
	{	//use alternate (red) numbers

		while(len>0)
		{
			len--;
			c = str2chr(s, len);

			drawpic(pos+len * '24 0 0', anumber[c-'0'], '24 24 0', '1 1 1', 1, 0);
		}
	}
	else
	{	//use normal numbers

		while(len>0)
		{
			len--;
			c = str2chr(s, len);

			drawpic(pos+len * '24 0 0', number[c-'0'], '24 24 0', '1 1 1', 1, 0);
		}
	}
};

void(float type, vector pos, float drawback) Hud_DrawAmmoCount =
{
	string s;
	float value;
	float len;
	float c;
	value = getstati(STAT_SHELLS+type);
	if (value < 0)
		value = 0;	//hrm
	if (value>999)
		value = 999;

	s = ftos(floor(value));
	len = strlen(s);

	pos_x += 8 * (3-len);
	while(len>0)
	{
		len--;
		c = str2chr(s, len);
		drawcharacter(pos+len * '8 0 0', (18-'0') + c, '8 8 0', '1 1 1', 1, 0);
	}
};

string weaponnames[7] = {
	"inv_shotgun",
	"inv_sshotgun",
	"inv_nailgun",
	"inv_sshotgun",
	"inv_rlaunch",
	"inv_srlaunch",
	"inv_lightng"
};
void(float num, vector pos) Hud_DrawWeapon
{
	drawpic(pos, weaponnames[num], '24 16 0', '1 1 1', 1, 0);
};
void(float num, vector pos) Hud_DrawWeaponWide	//for LG
{
	drawpic(pos, weaponnames[num], '48 16 0', '1 1 1', 1, 0);
};

string(float f, float chars, string lead) FormatFloat
{
	string s = ftos(f);
	if (f < 10 && chars >= 3)
		s = strcat(lead, lead, s);
	else if (f < 10 && chars == 2)
		s = strcat(lead, s);
	else if (f < 100 && chars >= 3)
		s = strcat(lead, s);
	return s;
};

void(vector pos) Hud_CoopScores_SBar =
{
	string s;
	float secs;
	float mins;
	//When you press tab in single player/coop, you get some replacement info over the sbar
	vector sbar = screensize_y * ' 0 1 0' - '0 24 0';
	drawpic(sbar, "scorebar", '320 24 0', '1 1 1', 0.333, 0);

	s = strcat("Monsters:", FormatFloat(getstatf(STAT_KILLEDMONSTERS), 3, " "), "/", FormatFloat(getstatf(STAT_TOTALMONSTERS), 3, " "));
	drawstring(sbar + '8 4', s, '8 8 0', '1 1 1', 1, 0);
	s = strcat("Secrets :", FormatFloat(getstatf(STAT_FOUNDSECRETS), 3, " "), "/", FormatFloat(getstatf(STAT_TOTALSECRETS), 3, " "));
	drawstring(sbar + '8 12', s, '8 8 0', '1 1 1', 1, 0);

	mins = floor(time/60);
	secs = floor(time - mins*60);
	s = strcat("Time :", FormatFloat(mins, 3, " "), ":", FormatFloat(secs, 2, "0"));
	drawstring(sbar + '184 4', s, '8 8 0', '1 1 1', 1, 0);

	drawstring(sbar + '232 12' - strlen(levelname)*'4 0', levelname, '8 8 0', '1 1 1', 1, 0);
};

void Hud_DrawSBar(vector pos)
{
	drawpic(pos, "sbar", '320 24 0', '1 1 1', 0.333, 0);

	if (stat_items & IT_INVULNERABILITY)
	{
		drawpic(pos, "disc", '24 24 0', '1 1 1', 1, 0);
		Hud_DrawLargeValue(pos+'24 0 0', 999, 25);
	}
	else
	{
		Hud_DrawLargeValue(pos+'24 0 0', getstatf(STAT_ARMOR), 25);
		if (stat_items & IT_ARMOR3)
			drawpic(pos, "sb_armor3", '24 24 0', '1 1 1', 1, 0);
		else if (stat_items & IT_ARMOR2)
			drawpic(pos, "sb_armor2", '24 24 0', '1 1 1', 1, 0);
		else if (stat_items & IT_ARMOR1)
			drawpic(pos, "sb_armor1", '24 24 0', '1 1 1', 1, 0);
	}

	Hud_DrawLargeValue(pos+'136 0 0', getstatf(STAT_HEALTH), 25);

	if (stat_items & IT_SHELLS)
		drawpic(pos+'224 0 0', "sb_shells", '24 24 0', '1 1 1', 1, 0);
	else if (stat_items & IT_NAILS)
		drawpic(pos+'224 0 0', "sb_nails", '24 24 0', '1 1 1', 1, 0);
	else if (stat_items & IT_ROCKETS)
		drawpic(pos+'224 0 0', "sb_rocket", '24 24 0', '1 1 1', 1, 0);
	else if (stat_items & IT_CELLS)
		drawpic(pos+'224 0 0', "sb_cells", '24 24 0', '1 1 1', 1, 0);
	Hud_DrawLargeValue(pos+'248 0 0', getstatf(STAT_AMMO), 10);
};

void Hud_DrawIBar(vector pos)
{
	drawpic(pos, "ibar", '320 24 0', '1 1 1', 0.333, 0);

	if (stat_items & IT_SHOTGUN)
		Hud_DrawWeapon(0, pos+'0 8 0');
	if (stat_items & IT_SUPER_SHOTGUN)
		Hud_DrawWeapon(1, pos+'24 8 0');
	if (stat_items & IT_NAILGUN)
		Hud_DrawWeapon(2, pos+'48 8 0');
	if (stat_items & IT_SUPER_NAILGUN)
		Hud_DrawWeapon(3, pos+'72 8 0');
	if (stat_items & IT_GRENADE_LAUNCHER)
		Hud_DrawWeapon(4, pos+'96 8 0');
	if (stat_items & IT_ROCKET_LAUNCHER)
		Hud_DrawWeapon(5, pos+'120 8 0');
	if (stat_items & IT_LIGHTNING)
		Hud_DrawWeaponWide(6, pos+'144 8 0');

	Hud_DrawAmmoCount(0, pos + '10 0 0', false);
	Hud_DrawAmmoCount(1, pos + '58 0 0', false);
	Hud_DrawAmmoCount(2, pos + '106 0 0', false);
	Hud_DrawAmmoCount(3, pos + '154 0 0', false);

	if (stat_items & IT_KEY1)
		drawpic(pos+'192 8 0', "sb_key1", '16 16 0', '1 1 1', 1, 0);
	if (stat_items & IT_KEY2)
		drawpic(pos+'208 8 0', "sb_key2", '16 16 0', '1 1 1', 1, 0);

	if (stat_items & IT_INVISIBILITY)
		drawpic(pos+'224 8 0', "sb_invis", '16 16 0', '1 1 1', 1, 0);
	if (stat_items & IT_INVULNERABILITY)
		drawpic(pos+'240 8 0', "sb_invuln", '16 16 0', '1 1 1', 1, 0);
	if (stat_items & IT_SUIT)
		drawpic(pos+'256 8 0', "sb_suit", '16 16 0', '1 1 1', 1, 0);
	if (stat_items & IT_QUAD)
		drawpic(pos+'272 8 0', "sb_quad", '16 16 0', '1 1 1', 1, 0);

	if (stat_items2 & 32)
		drawpic(pos+'288 8 0', "sb_sigil1", '8 16 0', '1 1 1', 1, 0);
	if (stat_items2 & 64)
		drawpic(pos+'296 8 0', "sb_sigil2", '8 16 0', '1 1 1', 1, 0);
	if (stat_items2 & 128)
		drawpic(pos+'304 8 0', "sb_sigil3", '8 16 0', '1 1 1', 1, 0);
	if (stat_items2 & 256)
		drawpic(pos+'312 8 0', "sb_sigil4", '8 16 0', '1 1 1', 1, 0);
};

string q3numbers[10] =
{
	"gfx/2d/numbers/zero_32b.tga",
	"gfx/2d/numbers/one_32b.tga",
	"gfx/2d/numbers/two_32b.tga",
	"gfx/2d/numbers/three_32b.tga",
	"gfx/2d/numbers/four_32b.tga",
	"gfx/2d/numbers/five_32b.tga",
	"gfx/2d/numbers/six_32b.tga",
	"gfx/2d/numbers/seven_32b.tga",
	"gfx/2d/numbers/eight_32b.tga",
	"gfx/2d/numbers/nine_32b.tga"
};
void Hud_DrawQ3Number(vector pos, float value, vector colours)
{
	float c;
	float len;
	string s;
	if (value < 0)
		value = 0;	//hrm
	if (value>999)
		value = 999;

	s = ftos(floor(value));
	len = strlen(s);

	pos_x += 24 * (3-len);
	
	while(len>0)
	{
		len--;
		c = str2chr(s, len);

		drawpic(pos+len * '24 0 0', q3numbers[c-'0'], '24 24 0', colours, 1, 0);
	}
};

entity hud3ditem;

void Hud_Draw3dScene(vector topleft, vector sz)
{
	clearscene();

	setviewprop(VF_DRAWWORLD, 0);
	setviewprop(VF_DRAWENGINESBAR, 0);
	setviewprop(VF_DRAWCROSSHAIR, 0);
	setviewprop(VF_MIN, topleft);
	setviewprop(VF_SIZE, sz);
	setviewprop(VF_FOV, '30 30');
	setviewprop(VF_ORIGIN, '0 0 0');
	setviewprop(VF_ANGLES, '0 0 0');

	addentity(hud3ditem);
	renderscene();
}

void Hud_Draw3dItem(vector topleft, vector sz, string modelname, vector org, vector ang)
{
	if (!hud3ditem)
		hud3ditem = spawn();

	hud3ditem.origin = org;
	hud3ditem.angles = ang;
	setmodel(hud3ditem, modelname);

	Hud_Draw3dScene(topleft, sz);
}

void Hud_DrawQ3Head(vector topleft, vector sz, string skinname, vector ang)
{
	float headmodelindex;
	vector headoffset;
	float headskinnumber;

	vector org;

	tokenize(skinname);
	skinname = argv(0);

	headmodelindex = Anim_GetHeadModelIndex(skinname);
	headskinnumber = Anim_GetHeadSkinNumber(skinname);
	headoffset = Anim_GetHeadOffset(skinname);

	if (!headmodelindex)
		return;	//eep.

	if (!hud3ditem )
		hud3ditem = spawn();

	setmodelindex(hud3ditem, headmodelindex);
	hud3ditem.skin = headskinnumber;

	org_z = -0.5 * (hud3ditem.mins_z + hud3ditem.maxs_z);
	org_y = 0.5 * (hud3ditem.mins_y + hud3ditem.maxs_y);
	org_x = (hud3ditem.maxs_z - hud3ditem.mins_z) * (0.7 / /*tan(30)*/0.268);

	org += headoffset;

	hud3ditem.origin = org;
	hud3ditem.angles = ang;

	Hud_Draw3dScene(topleft, sz);
	hud3ditem.skin = 0;
}

void Hud_Q3(vector pos)
{
	Hud_Draw3dItem(pos+'224 0', '24 24', "models/powerups/ammo/rocketam.md3", '70 0 0', '0 90 0' + ('0 1 0'*20)*sin(time));
	Hud_Draw3dItem(pos+'0 0', '24 24', "models/powerups/armor/armor_red.md3", '80 0 -10', '0 1 0'*360*(time/6));
	Hud_DrawQ3Head(pos+'112 0', '24 24', cvar_string("cg_forceskin"), '0 180 0'+'0 20 0'*sin(time));

	Hud_DrawQ3Number(pos+'24 0 0', getstatf(STAT_ARMOR), '1 1 1');
	Hud_DrawQ3Number(pos+'136 0 0', getstatf(STAT_HEALTH), '1 1 1');
	Hud_DrawQ3Number(pos+'248 0 0', getstatf(STAT_AMMO), '1 1 1');
}

nonstatic void Hud_Draw(float hudtype, float scoreboard, float width, float height)
{
	vector pos;
	screensize_x = width;
	screensize_y = height;
	screensize_z = 0;

	pos_x = (screensize_x-320)/2;
	pos_y = screensize_y;
	pos_z = 0;

	stat_items = getstatbits(STAT_ITEMS, 0, 23);
	stat_items2 = getstatbits(STAT_ITEMS, 23, 9);

	//if hudtype == 0 then the engine already drew it.
	if (hudtype == 3)
	{
		Hud_Q3(pos - '0 24 0');
		return;
	}
	else if (hudtype == 2)
	{
		if (scoreboard)
		{
			Hud_CoopScores_SBar(pos - '0 24 0');
		}
		else
		{
			Hud_DrawSBar(pos - '0 24 0');
		}
		Hud_DrawIBar(pos - '0 48 0');
	}
};
