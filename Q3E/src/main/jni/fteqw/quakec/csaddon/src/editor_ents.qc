static int selectedent;
static entity tempent;
static string editkey;
static int editfieldtype;	//0 = not editing. 1 = editing the fieldname. 2 = editing the value

typedef struct
{
	//quick access for rendering.
	float modelindex;
	float isbsp;
	float alpha;
	vector colourmod;
	vector org;
	vector ang;
	vector mins;
	vector maxs;
	float scale;
	
	trisoup_simple_vert_t bboxverts[8];

	//full data
	hashtable fields;
} entedit_t;

int bbox_line_idxs[] =
{
	0,2, 2,3, 3,1, 1,0,
	4,6, 6,7, 7,5, 5,4,
	0,4, 3,7, 2,6, 1,5
};

static entedit_t *editents;
static int numents;

struct
{	//FIXME: should probably parse quakeed comments or something.
	//FIXME: bbox colours.
	string classn;
	string model;
	vector colour;
	vector mins, maxs;
	float spawnflags_match;
	float spawnflags_mask;
} entclasses[] =
{
	{"info_player_start", 			"progs/player.mdl",	'0 1 0',	'-16 -16 -24',	'16 16 32'},
	{"info_player_start", 			"progs/player.mdl",	'1 0 0',	'-16 -16 -24',	'16 16 32'},
	{"info_player_start2",			"progs/player.mdl",	'1 0 0',	'-16 -16 -24',	'16 16 32'},
	{"info_player_deathmatch",		"progs/player.mdl",	'1 0 1',	'-16 -16 -24',	'16 16 32'},
	{"info_player_coop", 			"progs/player.mdl",	'1 0 1',	'-16 -16 -24',	'16 16 32'},

	{"item_armor1", 			"progs/armor.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"item_armor2", 			"progs/armor.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"item_armorInv", 			"progs/armor.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},

	{"weapon_supershotgun", 		"progs/g_shot.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"weapon_nailgun", 			"progs/g_nail.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"weapon_supernailgun", 		"progs/g_nail2.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"weapon_grenadelauncher",		"progs/g_rock.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"weapon_rocketlauncher", 		"progs/g_rock2.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},
	{"weapon_lightning", 			"progs/g_light.mdl",	'0 0.5 0.8',	'-16 -16 0',	'16 16 56'},

	{"item_key1", 				"progs/w_s_key.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},
	{"item_key2", 				"progs/w_g_key.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},
	{"item_sigil", 				"progs/end1.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},

	{"item_artifact_invulnerability", 	"progs/invulner.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},
	{"item_artifact_envirosuit", 		"progs/suit.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},
	{"item_artifact_invisibility", 		"progs/invisibl.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},
	{"item_artifact_super_damage", 		"progs/quaddama.mdl",	'0 0.5 0.8',	'-16 -16 -24',	'16 16 32'},



//	{"func_button",				"*",			'0 .5 .8', 	?
//	{"trigger_changelevel",			"*",			'0.5 0.5 0.5'	? //NO_INTERMISSION
//	{"func_door",				"*",			'0 .5 .8,'		? //START_OPEN x DOOR_DONT_LINK GOLD_KEY SILVER_KEY TOGGLE
//	{"func_door_secret",			"*",			'0 .5 .8',		? //open_once 1st_left 1st_down no_shoot always_shoot
//	{"trigger_portaldespawn",		"*",			'.5 .5 .5',	?
//	{"func_wall",				"*",			'0 .5 .8', 	?
//	{"func_illusionary",			"*",			'0 .5 .8',		?
//	{"func_episodegate",			"*",			'0 .5 .8',		? //E1 E2 E3 E4
//	{"func_bossgate",			"*",			'0 .5 .8',		?
//	{"func_plat",				"*",			'0 .5 .8'		? PLAT_LOW_TRIGGER
//	{"func_train",				"*",			'0 .5 .8'		?
//	{"trigger_multiple",			"*",			'.5 .5 .5' 	? notouch
//	{"trigger_once",			"*",			'.5 .5 .5' 	? notouch
//	{"trigger_secret",			"*",			'.5 .5 .5' 	?
//	{"trigger_counter",			"*",			'.5 .5 .5' 	? nomessage
//	{"trigger_teleport",			"*",			'.5 .5 .5' 	? PLAYER_ONLY SILENT
//	{"trigger_setskill",			"*",			'.5 .5 .5' 	?
//	{"trigger_onlyregistered",		"*",			'.5 .5 .5' 	?
//	{"trigger_hurt",			"*",			'.5 .5 .5' 	?
//	{"trigger_push",			"*",			'.5 .5 .5' 	? PUSH_ONCE
//	{"trigger_monsterjump",			"*",			'.5 .5 .5', 	?
//	{"trigger_motionsickness",		"*",			'.5 .5 .5', 	?
//	{"worldspawn",				"*",			'0 0 0) ?

	{"path_corner",				0,			'0.5 0.3 0', 	'-8 -8 -8',	'8 8 8'},
	{"event_lightning",			0,			'0 1 1', 	'-16 -16 -16',	'16 16 16'},
	{"info_intermission",			0,			'1 0.5 0.5', 	'-16 -16 -16',	'16 16 16'},
	{"misc_portalspawn",			0,			'.5 .5 .5', 	'-8 -8 -8',	'8 8 8'},
	{"noclass",				0,			'0 0 0', 	'-8 -8 -8',	'8 8 8'},
	{"item_health",				"maps/b_bh25.bsp",	'.3 .3 1', 	'0 0 0',	'32 32 32'}, //rotten megahealth
	{"item_shells",				"maps/b_shell0.bsp",	'0 .5 .8', 	'0 0 0',	'32 32 32'}, //big
	{"item_spikes",				"maps/b_nail0.bsp",	'0 .5 .8', 	'0 0 0',	'32 32 32'}, //big
	{"item_rockets",			"maps/b_rock0.bsp",	'0 .5 .8', 	'0 0 0',	'32 32 32'}, //big
	{"item_cells",				"maps/b_batt0.bsp",	'0 .5 .8', 	'0 0 0',	'32 32 32'}, //big
	{"item_weapon",				0,			'0 .5 .8', 	'0 0 0',	'32 32 32'}, //shotgun rocket spikes big
	{"info_null",				0,			'0 0.5 0', 	'-4 -4 -4',	'4 4 4'},
	{"info_notnull",			0,			'0 0.5 0', 	'-4 -4 -4',	'4 4 4'},
	{"light",				0,			'0 1 0', 	'-8 -8 -8',	'8 8 8'}, //START_OFF
	{"light_fluoro",			0,			'0 1 0', 	'-8 -8 -8',	'8 8 8'},//START_OFF
	{"light_fluorospark",			0,			'0 1 0', 	'-8 -8 -8',	'8 8 8'},
	{"light_globe",				0,			'0 1 0', 	'-8 -8 -8',	'8 8 8'},
	{"light_torch_small_walltorch",		0,			'0 .5 0', 	'-10 -10 -20',	'10 10 20'},
	{"light_flame_large_yellow",		0,			'0 1 0', 	'-10 -10 -12',	'12 12 18'},
	{"light_flame_small_yellow",		0,			'0 1 0', 	'-8 -8 -8',	'8 8 8'}, //START_OFF
	{"light_flame_small_white",		0,			'0 1 0', 	'-10 -10 -40',	'10 10 40'}, //START_OFF
	{"misc_fireball",			0,			'0 .5 .8', 	'-8 -8 -8',	'8 8 8'},
	{"misc_explobox",			0,			'0 .5 .8', 	'0 0 0',	'32 32 64'},
	{"misc_explobox2",			0,			'0 .5 .8', 	'0 0 0',	'32 32 64'},
	{"trap_spikeshooter",			0,			'0 .5 .8', 	'-8 -8 -8',	'8 8 8'}, //superspike laser
	{"trap_shooter",			0,			'0 .5 .8', 	'-8 -8 -8',	'8 8 8'}, //superspike laser
	{"air_bubbles",				0,			'0 .5 .8', 	'-8 -8 -8',	'8 8 8'},
	{"viewthing",				0,			'0 .5 .8', 	'-8 -8 -8',	'8 8 8'},
	{"ambient_suck_wind",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_drone",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_flouro_buzz",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_drip",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_comp_hum",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_thunder",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_light_buzz",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_swamp1",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"ambient_swamp2",			0,			'0.3 0.1 0.6', 	'-10 -10 -8',	'10 10 8'},
	{"misc_noisemaker",			0,			'1 0.5 0', 	'-10 -10 -10',	'10 10 10'},
	{"misc_teleporttrain",			0,			'0 .5 .8', 	'-8 -8 -8',	'8 8 8'},
	{"trigger_relay",			0,			'.5 .5 .5', 	'-8 -8 -8', 	'8 8 8'},
	{"info_teleport_destination",		0,			'.5 .5 .5', 	'-8 -8 -8', 	'8 8 32'},

	{"monster_boss",			"progs/boss.mdl",	'1 0 0', 	'-128 -128 -24','128 128 256'},
	{"monster_hell_knight",			"progs/hknight.mdl",	'1 0 0', 	'-16 -16 -24',	'16 16 40'}, //Ambush
	{"monster_demon1",			"progs/demon.mdl",	'1 0 0', 	'-32 -32 -24',	'32 32 64'}, //Ambush
	{"monster_dog",				"progs/dog.mdl",	'1 0 0', 	'-32 -32 -24',	'32 32 40'}, //Ambush
	{"monster_enforcer",			"progs/enforcer.mdl",	'1 0 0', 	'-16 -16 -24',	'16 16 40'}, //Ambush
	{"monster_fish",			"progs/fish.mdl",	'1 0 0', 	'-16 -16 -24',	'16 16 24'}, //Ambush
	{"monster_ogre",			"progs/ogre.mdl",	'1 0 0', 	'-32 -32 -24', 	'32 32 64'}, //Ambush
	{"monster_oldone",			"progs/oldone.mdl",	'1 0 0', 	'-16 -16 -24', 	'16 16 32'},
	{"monster_knight",			"progs/knight.mdl",	'1 0 0', 	'-16 -16 -24',	'16 16 40'}, //Ambush
	{"monster_shalrath",			"progs/shalrath.mdl",	'1 0 0', 	'-32 -32 -24',	'32 32 48'}, //Ambush
	{"monster_shambler",			"progs/shambler.mdl",	'1 0 0', 	'-32 -32 -24',	'32 32 64'}, //Ambush
	{"monster_army",			"progs/soldier.mdl",	'1 0 0', 	'-16 -16 -24',	'16 16 40'}, //Ambush
	{"monster_tarbaby",			"progs/tarbaby.mdl",	'1 0 0', 	'-16 -16 -24',	'16 16 24'}, //Ambush
	{"monster_wizard",			"progs/wizard.mdl",	'1 0 0', 	'-16 -16 -24', 	'16 16 40'}, // Ambush
	{"monster_zombie",			"progs/zombie.mdl",	'1 0 0', 	'-16 -16 -24', 	'16 16 32'} //Crucified ambush
};

entedit_t*() editor_ents_new =
{
	local int nent;
	local entedit_t *newents;
	nent = terrain_edit(TEREDIT_ENT_ADD, "");
	if (nent >= numents)
	{
		//extend the list
		newents = memalloc(sizeof(entedit_t)*(nent+1));
		memcpy((__variant*)newents, (__variant*)editents, sizeof(entedit_t)*numents);
		memfree((__variant*)editents);
		editents = newents;
		numents = nent+1;
	}

	editents[nent].fields = hash_createtab(12, EV_STRING);

	return &editents[nent];
};

void(float num) editor_ents_delete =
{
	//num 0 *could* be deleted, but we really don't want to allow that...
	if (num > 0 && num < numents)
	{
		hash_destroytab(editents[num].fields);
		editents[num].fields = 0i;
		terrain_edit(TEREDIT_ENT_SET, num, __NULL__);
	}
};

string(entedit_t *ent) reforment =
{
	string n = "";
	for (int i = 0; ; i++)
	{
		string key, value;
		key = hash_getkey(ent->fields, i);
		if isnull(key)
			break;
		if (key == "{")
			continue;
		value = ent->fields[key];
		//inject markup into the value so that it doesn't get too corrupted
		value = strreplace("\\", "\\\\", value);
		value = strreplace("\"", "\\\"", value);
		value = strreplace("\n", "\\n", value);
		//these are more optional
		value = strreplace("\t", "\\t", value);
		value = strreplace("\r", "\\r", value);
		n = strcat(n, key, " \"", value, "\"\n");
	}
	return n;
};

void(entedit_t *ent) editor_ents_edited =
{
	string n = reforment(ent);
	terrain_edit(TEREDIT_ENT_SET, ent-editents, n);
}

inline float(string model) modelindexforname =
{
//	print("precaching \"");
//	print(model);
//	print("\"\n");
	precache_model(model);
	setmodel(tempent, model);
	return tempent.modelindex;
};
void(entedit_t *nent) editor_ents_updated =
{
	float ang;
	int i;

	nent->alpha = stof(nent->fields["alpha"]);
	nent->scale = stof(nent->fields["scale"]);
	nent->modelindex = modelindexforname(nent->fields["model"]);
	nent->mins = '-8 -8 -8';
	nent->maxs = '8 8 8';
	nent->org = stov(nent->fields["origin"]);
	nent->isbsp = FALSE;
	nent->colourmod = '1 1 1';

	ang = stof(nent->fields["angle"]);
	if (ang == -1)
		nent->ang = [90, 0, 0];
	else if (ang == -2)
		nent->ang = [-90, 0, 0];
	else
		nent->ang = [0, ang, 0];

	if (!nent->modelindex)
	{
		string classn;
		classn = nent->fields["classname"];
		for (i = 0; i < entclasses.length; i++)
		{
			if (classn == entclasses[i].classn)
			{
				nent->modelindex = modelindexforname(entclasses[i].model);
				nent->alpha = 0.7;
				nent->colourmod = entclasses[i].colour;
				nent->mins = entclasses[i].mins;
				nent->maxs = entclasses[i].maxs;
				break;
			}
		}
	}
	else
		nent->isbsp = TRUE;

	if (nent->isbsp)
		nent->ang = '0 0 0';	//bsp entities should not be displayed rotated, because that just messes everything up.

	for (i = 0; i < 8; i++)
	{
		nent->bboxverts[i].st = (vec2){0,0};
		nent->bboxverts[i].rgba = (vec4){nent->colourmod[0], nent->colourmod[1], nent->colourmod[2], nent->alpha?nent->alpha:1};
		nent->bboxverts[i].xyz[0] = nent->org[0] + ((i&1i)?nent->maxs[0]:nent->mins[0]);
		nent->bboxverts[i].xyz[1] = nent->org[1] + (((i&2i)?nent->maxs[1]:nent->mins[1]));
		nent->bboxverts[i].xyz[2] = nent->org[2] + ((i&4)?nent->maxs[2]:nent->mins[2]);
	}
		/*void(string shadername, vector min, vector max, vector col) editor_ents_drawbbox =
{
	if (min == max)
		return;
	R_BeginPolygon(shadername);
#define line(x,y) 			R_PolygonVertex(x, '0 0', col, 1);	R_PolygonVertex(y, '0 0', col, 1);	R_EndPolygon()
	line(min, ([max[0], min[1], min[2]]));
	line(min, ([min[0], max[1], min[2]]));
	line(min, ([min[0], min[1], max[2]]));
	line(max, ([min[0], max[1], max[2]]));
	line(max, ([max[0], min[1], max[2]]));
	line(max, ([max[0], max[1], min[2]]));

	line(([max[0], min[1], min[2]]), ([max[0], max[1], min[2]]));
	line(([max[0], min[1], min[2]]), ([max[0], min[1], max[2]]));
	line(([min[0], max[1], min[2]]), ([min[0], max[1], max[2]]));
	line(([min[0], max[1], min[2]]), ([max[0], max[1], min[2]]));
	line(([min[0], min[1], max[2]]), ([min[0], max[1], max[2]]));
	line(([min[0], min[1], max[2]]), ([max[0], min[1], max[2]]));
#undef line
};*/
};

entedit_t*(entedit_t *o) editor_ents_clone =
{
	int i;
	entedit_t *n = editor_ents_new();

	for (i = 0; ; i++)
	{
		string key, value;
		key = hash_getkey(o->fields, i);
		if isnull(key)
			break;
		value = o->fields[key];
		n->fields[key] = value;
	}

	editor_ents_updated(n);
	return n;
};

void() editor_ents_reload =
{
	local entedit_t *nent;
	local string field, value;

	int id;
	int f, fcount;

	numents = terrain_edit(TEREDIT_ENT_COUNT);
	editents = memalloc(sizeof(entedit_t)*numents);
	for (id = 0; id < numents; id++)
	{
		field = terrain_edit(TEREDIT_ENT_GET, id);
		nent = &editents[id];
		if (nent->fields)
			hash_destroytab(nent->fields);
		nent->fields = __NULL__;
		if (field == __NULL__)	//empty entity slot.
			continue;
		nent->fields = hash_createtab(12, EV_STRING);
		fcount = tokenize(field);
		for (f = 0; f < fcount; f+=2)
		{
			field = argv(f);
			value = argv(f+1);
			nent->fields[field] = value;
		}
		editor_ents_updated(nent);
	}
};

//called when another client has edited an entity.
void(int idx, string new) CSQC_MapEntityEdited =
{
	if (idx >= numents)
	{
		if not (new)
			return; //deleting an ent that doesn't already exist? :o
	}
	local entedit_t *ent = &editents[idx];

	if (ent->fields)
		hash_destroytab(ent->fields);
	ent->fields = hash_createtab(12, EV_STRING);
	int fcount = tokenize(new);
	for (int f = 0; f < fcount; f+=2)
	{
		string field = argv(f);
		string value = argv(f+1);
		ent->fields[field] = value;
	}

	editor_ents_updated(ent);
//	editor_ents_edited(ent);
};

void() editor_ents_addentities =
{
	int e;
	string shadername;
	if (!tempent)
	{
		tempent = spawn();
		editor_ents_reload();
	}

	//make sure this shader was generated already.
	shaderforname("entbox",
		"{"
			"cull disable\n"
			"{\n"
				"map $whiteimage\n"
				"blendfunc add\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}");
	//make sure this shader was generated already.
	shaderforname("entboxsel",
		"{"
			"cull disable\n"
			"{\n"
				"map $whiteimage\n"
				"blendfunc add\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"sort nearest\n"
				"nodepthtest\n"
			"}\n"
		"}");

	for (e = 1; e < numents; e++)
	{
		if (e == selectedent)
		{
			if ((gettime(0)*5f) & 1)
				continue;
			tempent.effects |= EF_NODEPTHTEST;
			shadername = "entboxsel";
		}
		else
		{
			tempent.effects &= ~EF_NODEPTHTEST;
			shadername = "entbox";
		}

		if (editents[e].modelindex)
		{
			tempent.modelindex = editents[e].modelindex;
			tempent.alpha = editents[e].alpha;
			tempent.scale = editents[e].scale;
			tempent.angles = editents[e].ang;
			tempent.colormod = editents[e].colourmod;

			setorigin(tempent, editents[e].org);
			addentity(tempent);
		}
		else
		{
			addtrisoup_simple("entbox", 0x800, &editents[e].bboxverts[0], bbox_line_idxs, bbox_line_idxs.length);
		}
	}
};

float(float key, float unic, vector mousepos) editor_ents_key =
{
	vector t = mousefar;
	vector o = mousenear;
	entedit_t *ent;

	if (key == K_MOUSE1)
	{
		float bestfrac = 1;
		int bestent = selectedent, e;
		
		//FIXME: we need some click+drag moving like the brush editor has
		
		if (mousepos_x < 128)
		{
			editfieldtype = isnull(editkey)?1:2;
			return TRUE;
		}
		editfieldtype = 0;

		if (vlen(o - t) > 8192)
			t = o + normalize(t)*8192;

		other = tempent;
		for (e = 1; e < numents; e++)
		{
			if (editents[e].isbsp)
				tempent.solid = SOLID_BSP;
			else
				tempent.solid = SOLID_BBOX;
			tempent.modelindex = editents[e].modelindex;
			tempent.alpha = editents[e].alpha;
			tempent.scale = editents[e].scale;
			tempent.angles = editents[e].ang;
			tempent.colormod = editents[e].colourmod;
			tempent.mins = editents[e].mins;
			tempent.maxs = editents[e].maxs;
			setorigin(tempent, editents[e].org);

			traceline(o, t, 256, tempent);

			if (bestfrac > trace_fraction)
			{
				bestfrac = trace_fraction;
				bestent = e;
			}
		}
		tempent.modelindex = 0;

		selectedent = bestent;
	}
	else if (editfieldtype == 1)
	{
		if (selectedent >= numents)
			return FALSE;

		if (key == K_ESCAPE)
		{
			editkey = "";
			editfieldtype = 0;
		}
		else if (key == K_BACKSPACE || key == K_DEL)
		{
			if (!editkey)
				editfieldtype = 0;
			else
				editkey = substring(editkey, 0, -2);
		}
		else if (key == K_ENTER && !shiftdown)
		{
			if (editkey != "")
			{
				editents[selectedent].fields[editkey] = "";
				editfieldtype = 2;
			}
		}
		else if (unic)
			editkey = strcat(editkey, chr2str(unic));
		else
			return FALSE;
	}
	else if (editfieldtype == 2)
	{
		if (selectedent >= numents)
			return FALSE;
		ent = &editents[selectedent];
		string value = ent->fields[editkey];

		if (key == K_ESCAPE)
		{
			editkey = "";
			editfieldtype = 0;
		}
		else if (key == K_BACKSPACE || key == K_DEL)
		{
			if (!value)
			{
				hash_delete(ent->fields, editkey);
				editfieldtype = 0;
			}
			else
				value = substring(value, 0, -2);
		}
		else if (key == K_ENTER && !shiftdown)
			editfieldtype = 0;
		else if (key == K_ENTER)
			value = strcat(value, chr2str('\n'));
		else if (unic)
			value = strcat(value, chr2str(unic));
		else
			return FALSE;

		if (editfieldtype)
		{
			ent->fields[editkey] = value;
			editor_ents_edited(ent);
			editor_ents_updated(ent);
		}
	}
	else if (key == K_ESCAPE && selectedent)
		selectedent = 0;
	else if (key == K_BACKSPACE || key == K_DEL)
		editor_ents_delete(selectedent);
	else if (unic == 'm' || unic == 'M' || unic == 'i' || unic == 'I')
	{
		traceline(o, t, TRUE, world);
		
		ent = &editents[selectedent];
		if (unic == 'i' || unic == 'I')
			ent = editor_ents_clone(ent);

		//figure out how far along the plane normal to push the entity in order to ensure that its mins/maxs is on the floor/slope/ceiling/wall/etc
		//yay dotproducts
		float ext = 	trace_plane_normal * [
					(trace_plane_normal[0] < 0)?ent->mins[0]:ent->maxs[0],
					(trace_plane_normal[1] < 0)?ent->mins[1]:ent->maxs[1],
					(trace_plane_normal[2] < 0)?ent->mins[2]:ent->maxs[2]
					];
		//update the all important origin
		string str = sprintf("%v", trace_endpos + trace_plane_normal * ext);
		ent->fields["origin"] = str;
		//and fix up the quick-access stuff
		ent->org = stov(ent->fields["origin"]);

		editor_ents_edited(ent);
	}
	else
		return FALSE;
	return TRUE;
};

void(vector mousepos) editor_ents_overlay =
{
	int i;
	vector pos;
	vector col;
	float foundedit = FALSE;
	float pickedit;
	if (selectedent >= numents)
		return;
	entedit_t *ent = &editents[selectedent];
	
	pos = '128 0 0';
	pos_y = vidsize_y - 32;
	drawfill('0 16 0', pos, '0 0 0', 0.3);
	pos = '0 32 0';
	
	pickedit = !editfieldtype && mousepos_x < 128;
	if (pickedit)
		editkey = 0;

	for (i = 0; ; i++)
	{
		string key, value;
		key = hash_getkey(ent->fields, i);
		if isnull(key)
			break;
		value = ent->fields[key];
		col = '1 1 1';
		if (pickedit && mousepos_y >= pos_y && mousepos_y < pos_y + 8 && mousepos_x < 128)
		{
			col_y = 0;
			editkey = key;
		}
		if (key == editkey && editfieldtype)
		{
			col = '0 0 1';
			foundedit = TRUE;
		}
		
		//provide some markup so that its actually readable.
		value = strreplace("^", "^^", value);
		value = strreplace("\\", "^5\\\\^7", value);
		value = strreplace("\"", "^5\\\"^7", value);
		value = strreplace("\n", "^5\\n^7", value);
		value = strreplace("\t", "^5\\t^7", value);
		value = strreplace("\r", "^5\\r^7", value);
				
		drawstring(pos, sprintf("%s: %s", key, value), '8 8 0', col, 1, 0);
		pos_y += 8;
	}
	if (editfieldtype && !foundedit)
	{
		drawrawstring(pos, sprintf("%s: <UNDEFINED>", editkey==""?"<UNMNAMED>":editkey), '8 8 0', '0 0 1', 1);
		pos_y += 8;
	}
};
