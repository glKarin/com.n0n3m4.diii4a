//Copyright (C) 1999-2005 Id Software, Inc.
//
//
// ui_sandbox.c
//

#include "../qcommon/ns_local.h"
#include "ui_local.h" //karin: add
extern void * UI_Alloc( int size ); //karin: missing decl


#define SANDBOX_FRAME	"menu/art/cut_frame"

#define ID_LIST			100
#define ID_CLASSLIST	101
#define ID_TEXTURESLIST	102
#define ID_SPAWNOBJECT	103
#define ID_PRIV			104
#define ID_SAVEMAP		105
#define ID_LOADMAP		106
#define ID_TAB			107 //107-114
#define ID_CTAB			115 //115-125

#define STAB_CREATE		1
#define STAB_ENTITIES	2
#define STAB_NPC		3
#define STAB_ITEMS		4
#define STAB_LISTS		5
#define STAB_SCRIPTS	6
#define STAB_TOOLS		7
#define STAB_ADDONS		8

#define PROPERTIES_NUM	18
#define MAX_TAB_TEXT 8

#define MODIF(i) s_sandboxmain.modif[i].field.buffer
#define MODIF_LIST MODIF(0), MODIF(1), MODIF(2), MODIF(3), MODIF(4), MODIF(5), MODIF(6), MODIF(7), MODIF(8), MODIF(9), MODIF(10), MODIF(11), MODIF(12), MODIF(13), MODIF(14), MODIF(15), MODIF(16), MODIF(17)

typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		spawnobject;
	menuradiobutton_s		priv;
	menufield_s		grid;
	menutext_s		savemap;
	menutext_s		loadmap;
	menutext_s		propstext;
	menutext_s		classtext;
	menufield_s		modif[PROPERTIES_NUM];
	menuobject_s	list;
	menuobject_s	classlist;
	menuobject_s	texturelist;
	menutext_s		tab[8];
	menutext_s		ctab[10];
	
	char			names[524288];
	char			names2[524288];
	char*			configlist[524288];
	char*			classeslist[65536];
	char*			textureslist[65536];
	char*			botclasslist[65536];
	char*			item_itemslist[524288];
} sandboxmain_t;

char* 			entity_items[] = {
	"none",
	"weapon_machinegun",
	"weapon_shotgun",
	"weapon_grenadelauncher",
	"weapon_rocketlauncher",
	"weapon_lightning",
	"weapon_railgun",
	"weapon_plasmagun",
	"weapon_bfg",
	"weapon_grapplinghook",
	"weapon_nailgun",
	"weapon_prox_launcher",
	"weapon_chaingun",
	"weapon_flamethrower",
	"weapon_antimatter",
	"weapon_physgun",
	"weapon_gravitygun",
	"weapon_toolgun",
	"weapon_thrower",
	"weapon_bouncer",
	"weapon_thunder",
	"weapon_exploder",
	"weapon_knocker",
	"weapon_propgun",
	"weapon_regenerator",
	"weapon_nuke",
	"ammo_bullets",
	"ammo_shells",
	"ammo_grenades",
	"ammo_cells",
	"ammo_lightning",
	"ammo_rockets",
	"ammo_slugs",
	"ammo_bfg",
	"ammo_nails",
	"ammo_mines",
	"ammo_belt",
	"ammo_flame",
	"item_armor_shard",
	"item_armor_vest",
	"item_armor_combat",
	"item_armor_body",
	"item_armor_full",
	"item_health_small",
	"item_health",
	"item_health_large",
	"item_health_mega",
	"item_quad",
	"item_enviro",
	"item_haste",
	"item_invis",
	"item_regen",
	"item_flight",
	"item_scout",
	"item_doubler",
	"item_ammoregen",
	"item_guard",
	"holdable_teleporter",
	"holdable_medkit",
	"holdable_kamikaze",
	"holdable_invulnerability",
	"holdable_portal",
	"holdable_key_blue",
	"holdable_key_gold",
	"holdable_key_green",
	"holdable_key_iron",
	"holdable_key_master",
	"holdable_key_red",
	"holdable_key_silver",
	"holdable_key_yellow",
	0
};

char* 			item_items[] = {
	"All",
	"Machinegun",
	"Shotgun",
	"Grenade Launcher",
	"Rocket Launcher",
	"Lightning Gun",
	"Railgun",
	"Plasma Gun",
	"BFG10K",
	"Grappling Hook",
	"Nailgun",
	"Prox Launcher",
	"Chaingun",
	"Flamethrower",
	"Dark Flare",
	"Physgun",
	"Gravitygun",
	"Toolgun",
	"Thrower",
	"Bouncer",
	"Thunder",
	"Exploder",
	"Knocker",
	"Propgun",
	"Regenerator",
	"Nuke",
	"Bullets",
	"Shells",
	"Grenades",
	"Cells",
	"Lightning",
	"Rockets",
	"Slugs",
	"Bfg Ammo",
	"Nails",
	"Proximity Mines",
	"Chaingun Belt",
	"Flame",
	"Armor Shard",
	"Light Armor",
	"Armor 50",
	"Heavy Armor",
	"Full Armor",
	"Health 5",
	"Health 25",
	"Health 50",
	"Mega Health",
	"Quad Damage",
	"Battle Suit",
	"Speed",
	"Invisibility",
	"Regeneration",
	"Flight",
	"Scout",
	"Doubler",
	"Ammo Regen",
	"Guard",
	"Personal Teleporter",
	"Medkit",
	"Kamikaze",
	"Invulnerability",
	"Portal",
	0
};

int SandbOffset = 0;

static sandboxmain_t	s_sandboxmain;

const char* tool_spawnpreset_arg(int num){
	if(num == 1){
		if(!toolgun_disabledarg1.integer){
			return va("props/%s", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]);
		} else {
			return "";
		}
	}
	if(num == 2){
		if(!toolgun_disabledarg2.integer){
			if(s_sandboxmain.priv.curvalue){
			return "1";
			} else {
			return "0";	
			}
		} else {
			return "";
		}
	}
	if(num == 3){
		if(!toolgun_disabledarg3.integer){
			return s_sandboxmain.grid.field.buffer;
		} else {
			return "";
		}
	}
	if(num == 4){
		if(!toolgun_disabledarg4.integer){
			return s_sandboxmain.texturelist.itemnames[s_sandboxmain.texturelist.curvalue];
		} else {
			return "";
		}
	}
	
return "undefined presetarg";
}

/*
=================
SandboxMain_SaveChanges
=================
*/
static void SandboxMain_SaveChanges( void ) {
	//save cvars
	trap_Cvar_Set( "sb_classnum_view", "none" );
	if(uis.sb_tab == STAB_CREATE){
	trap_Cmd_ExecuteText( EXEC_INSERT, va(tool_spawnpreset.string, tool_spawnpreset_arg(1), tool_spawnpreset_arg(2), tool_spawnpreset_arg(3), tool_spawnpreset_arg(4), MODIF_LIST) );
	trap_Cvar_Set( "toolgun_modelst", va("props/%s", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
	trap_Cvar_Set( "sb_classnum_view", s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue] );
	trap_Cvar_Set( "sb_texturename", s_sandboxmain.texturelist.itemnames[s_sandboxmain.texturelist.curvalue] );
	}
	if(uis.sb_tab == STAB_ENTITIES){
	trap_Cmd_ExecuteText( EXEC_INSERT, va(spawn_preset.string, s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue], s_sandboxmain.priv.curvalue, s_sandboxmain.grid.field.buffer, "0") );
	trap_Cvar_Set( "toolgun_modelst", va("props/%s", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
	trap_Cvar_Set( "sb_classnum_view", s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue] );
	}
	if(uis.sb_tab == STAB_NPC){
	trap_Cvar_Set( "toolcmd_spawn", va("sl npc \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue], s_sandboxmain.modif[0].field.buffer, s_sandboxmain.modif[1].field.buffer, s_sandboxmain.modif[2].field.buffer, s_sandboxmain.modif[3].field.buffer, s_sandboxmain.modif[4].field.buffer) );
	trap_Cvar_Set( "toolgun_modelst", va("props/%s", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
	}
	if(uis.sb_tab == STAB_LISTS){
	trap_Cvar_Set( "toolcmd_spawn", va("ns_openscript_ui spawnlists/%s/%s.ns\n", s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue], s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
	}
	trap_Cvar_SetValue( "sb_private", s_sandboxmain.priv.curvalue );
	trap_Cvar_Set( "sb_grid", s_sandboxmain.grid.field.buffer );
	trap_Cvar_SetValue( "sb_modelnum", s_sandboxmain.list.curvalue );
	trap_Cvar_SetValue( "sb_classnum", s_sandboxmain.classlist.curvalue );
	trap_Cvar_SetValue( "sb_texturenum", s_sandboxmain.texturelist.curvalue );
	trap_Cvar_Set( "toolgun_mod1", s_sandboxmain.modif[0].field.buffer );
	trap_Cvar_Set( "toolgun_mod2", s_sandboxmain.modif[1].field.buffer );
	trap_Cvar_Set( "toolgun_mod3", s_sandboxmain.modif[2].field.buffer );
	trap_Cvar_Set( "toolgun_mod4", s_sandboxmain.modif[3].field.buffer );
	trap_Cvar_Set( "toolgun_mod5", s_sandboxmain.modif[4].field.buffer );
	trap_Cvar_Set( "toolgun_mod6", s_sandboxmain.modif[5].field.buffer );
	trap_Cvar_Set( "toolgun_mod7", s_sandboxmain.modif[6].field.buffer );
	trap_Cvar_Set( "toolgun_mod8", s_sandboxmain.modif[7].field.buffer );
	trap_Cvar_Set( "toolgun_mod9", s_sandboxmain.modif[8].field.buffer );
	trap_Cvar_Set( "toolgun_mod10", s_sandboxmain.modif[9].field.buffer );
	trap_Cvar_Set( "toolgun_mod11", s_sandboxmain.modif[10].field.buffer );
	trap_Cvar_Set( "toolgun_mod12", s_sandboxmain.modif[11].field.buffer );
	trap_Cvar_Set( "toolgun_mod13", s_sandboxmain.modif[12].field.buffer );
	trap_Cvar_Set( "toolgun_mod14", s_sandboxmain.modif[13].field.buffer );
	trap_Cvar_Set( "toolgun_mod15", s_sandboxmain.modif[14].field.buffer );
	trap_Cvar_Set( "toolgun_mod16", s_sandboxmain.modif[15].field.buffer );
	trap_Cvar_Set( "toolgun_mod17", s_sandboxmain.modif[16].field.buffer );
	trap_Cvar_Set( "toolgun_mod18", s_sandboxmain.modif[17].field.buffer );
	if(uis.sb_tab == STAB_CREATE){
	if(trap_Cvar_VariableValue("toolgun_tool") == 1){
	trap_Cvar_Set( "toolgun_mod1", s_sandboxmain.texturelist.itemnames[s_sandboxmain.texturelist.curvalue]);
	Q_strncpyz( s_sandboxmain.modif[0].field.buffer, s_sandboxmain.texturelist.itemnames[s_sandboxmain.texturelist.curvalue], sizeof(s_sandboxmain.modif[0].field.buffer) );
	}
	if(trap_Cvar_VariableValue("toolgun_tool") == 3){
	trap_Cvar_Set( "toolgun_mod1", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]);
	Q_strncpyz( s_sandboxmain.modif[0].field.buffer, s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], sizeof(s_sandboxmain.modif[0].field.buffer) );
	}
	}
	trap_Cmd_ExecuteText( EXEC_INSERT, va(tool_modifypreset.string, MODIF_LIST) );
	trap_Cmd_ExecuteText( EXEC_INSERT, va(tool_modifypreset2.string, MODIF_LIST) );
	trap_Cmd_ExecuteText( EXEC_INSERT, va(tool_modifypreset3.string, MODIF_LIST) );
	trap_Cmd_ExecuteText( EXEC_INSERT, va(tool_modifypreset4.string, MODIF_LIST) );
}

/*
=================
SandboxMain_SpawnListUpdate
=================
*/
static void SandboxMain_SpawnListUpdate( void ) {
	int		y;
	int		gametype;
	char	info[MAX_INFO_STRING];
	int		i;
	int		len;
	char	*configname;
	
	
	uis.spawnlist_folder 				= trap_Cvar_VariableValue("sb_classnum");
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( va("spawnlists/%s", s_sandboxmain.classlist.itemnames[uis.spawnlist_folder]), "ns", s_sandboxmain.names, 524288 );

	if (!s_sandboxmain.list.numitems) {
		strcpy(s_sandboxmain.names,"No items");
		s_sandboxmain.list.numitems = 1;
	}
	else if (s_sandboxmain.list.numitems > 65536)
		s_sandboxmain.list.numitems = 65536;

	configname = s_sandboxmain.names;
	for ( i = 0; i < s_sandboxmain.list.numitems; i++ ) {
		s_sandboxmain.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 3,".ns"))
			configname[len-3] = '\0';

		configname += len + 1;
	}
	strcpy(s_sandboxmain.list.string, va("spawnlists/%s/icons", s_sandboxmain.classlist.itemnames[uis.spawnlist_folder]));
}

/*
=================
SandboxMain_MenuKey
=================
*/
static sfxHandle_t SandboxMain_MenuKey( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		SandboxMain_SaveChanges();
	}
	return Menu_DefaultKey( &s_sandboxmain.menu, key );
}

static void SandboxMain_MenuDraw( void ) {
	int i;
	vec4_t			sbcolor1 = {1.00f, 1.00f, 1.00f, 0.60f};
	vec4_t			sbcolor2 = {0.30f, 0.30f, 0.30f, 0.90f};
	vec4_t			sbcolor3 = {0.50f, 0.50f, 0.30f, 0.90f};
	float			x, y, w, h;
	
	sbcolor1[0] = sbt_color0_0.value;
	sbcolor1[1] = sbt_color0_1.value;
	sbcolor1[2] = sbt_color0_2.value;
	sbcolor1[3] = sbt_color0_3.value;
	sbcolor2[0] = sbt_color1_0.value;
	sbcolor2[1] = sbt_color1_1.value;
	sbcolor2[2] = sbt_color1_2.value;
	sbcolor2[3] = sbt_color1_3.value;
	sbcolor3[0] = sbt_color2_0.value;
	sbcolor3[1] = sbt_color2_1.value;
	sbcolor3[2] = sbt_color2_2.value;
	sbcolor3[3] = sbt_color2_3.value;
	s_sandboxmain_color1[0] = sbt_color3_0.value;
	s_sandboxmain_color1[1] = sbt_color3_1.value;
	s_sandboxmain_color1[2] = sbt_color3_2.value;
	s_sandboxmain_color1[3] = sbt_color3_3.value;

	UI_DrawHandlePic( -2.0-uis.wideoffset, 0.0, 644+uis.wideoffset*2, 480, trap_R_RegisterShaderNoMip( va( "%s", sbt_wallpaper.string ) ) );

	UI_DrawRoundedRect(20-uis.wideoffset, 40, 600+uis.wideoffset*2, 435, 10, sbcolor1);
	UI_DrawRoundedRect(372-5+uis.wideoffset, 70-25, 225+10, (160*2)+50+55, 12, sbcolor2);	//tools
	
	if(uis.sb_tab == STAB_CREATE){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), 160+50, 12, sbcolor2);//props
	UI_DrawRoundedRect(40-5-uis.wideoffset, 215+70-25, 225+100+(uis.wideoffset*2), 160+50, 12, sbcolor2);//classes
	}
	if(uis.sb_tab == STAB_ENTITIES){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), (160*2)+50+55, 12, sbcolor2);//props
	}
	if(uis.sb_tab == STAB_NPC){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), 160+50, 12, sbcolor2);//props
	UI_DrawRoundedRect(40-5-uis.wideoffset, 215+70-25, 225+100+(uis.wideoffset*2), 160+50, 12, sbcolor2);//classes
	}
	if(uis.sb_tab == STAB_ITEMS){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), (160*2)+50+55, 12, sbcolor2);//props
	}
	if(uis.sb_tab == STAB_LISTS){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), 160+50, 12, sbcolor2);//props
	UI_DrawRoundedRect(40-5-uis.wideoffset, 215+70-25, 225+100+(uis.wideoffset*2), 160+50, 12, sbcolor2);//classes
	}
	if(uis.sb_tab == STAB_SCRIPTS){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), (160*2)+50+55, 12, sbcolor2);//props
	}
	if(uis.sb_tab == STAB_TOOLS){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), (160*2)+50+55, 12, sbcolor2);//props
	}
	if(uis.sb_tab == STAB_ADDONS){
	UI_DrawRoundedRect(40-5-uis.wideoffset, 70-25, 225+100+(uis.wideoffset*2), (160*2)+50+55, 12, sbcolor2);//props
	}
	
	for (i = 1; i <= 8; i++) {
		int xOffset = (-26 + 55 * i) - uis.wideoffset;
		if (uis.sb_tab == i) {
			UI_DrawRoundedRect(xOffset, 25, 52, 15, 0, sbcolor3);
		} else {
			UI_DrawRoundedRect(xOffset, 25, 52, 15, 0, sbcolor2);
		}
	}

	for (i = 1; i <= 10; i++) {
		int xOffset = (-26 + 55 * i) - uis.wideoffset;
		if(strlen(get_cvar_char(va("sb_ctab_%i", i)))){
			UI_DrawRoundedRect(xOffset, 5, 52, 15, 8, sbcolor2);
		}
	}

	Menu_Draw( &s_sandboxmain.menu );
	
	s_sandboxmain.modif[0].generic.name           	= toolgun_toolset1.string;
	s_sandboxmain.modif[1].generic.name          	= toolgun_toolset2.string;
	s_sandboxmain.modif[2].generic.name          	= toolgun_toolset3.string;
	s_sandboxmain.modif[3].generic.name           	= toolgun_toolset4.string;
	s_sandboxmain.modif[4].generic.name           	= toolgun_toolset5.string;
	s_sandboxmain.modif[5].generic.name          	= toolgun_toolset6.string;
	s_sandboxmain.modif[6].generic.name          	= toolgun_toolset7.string;
	s_sandboxmain.modif[7].generic.name           	= toolgun_toolset8.string;
	s_sandboxmain.modif[8].generic.name          	= toolgun_toolset9.string;
	s_sandboxmain.modif[9].generic.name           	= toolgun_toolset10.string;
	s_sandboxmain.modif[10].generic.name           	= toolgun_toolset11.string;
	s_sandboxmain.modif[11].generic.name          	= toolgun_toolset12.string;
	s_sandboxmain.modif[12].generic.name          	= toolgun_toolset13.string;
	s_sandboxmain.modif[13].generic.name           	= toolgun_toolset14.string;
	s_sandboxmain.modif[14].generic.name           	= toolgun_toolset15.string;
	s_sandboxmain.modif[15].generic.name          	= toolgun_toolset16.string;
	s_sandboxmain.modif[16].generic.name          	= toolgun_toolset17.string;
	s_sandboxmain.modif[17].generic.name           	= toolgun_toolset18.string;
	
	if(uis.sb_tab == STAB_NPC){
		s_sandboxmain.modif[0].generic.name       		= "Skill:";
		s_sandboxmain.modif[1].generic.name           	= "Health:";
		s_sandboxmain.modif[2].generic.name          	= "Name:";
		s_sandboxmain.modif[3].generic.name          	= "Music:";
		s_sandboxmain.modif[4].generic.name           	= "Weapon(id):";
		s_sandboxmain.modif[5].generic.name          	= "--------:";
		s_sandboxmain.modif[6].generic.name          	= "--------:";
		s_sandboxmain.modif[7].generic.name           	= "--------:";
		s_sandboxmain.modif[8].generic.name          	= "--------:";
		s_sandboxmain.modif[9].generic.name           	= "--------:";
		s_sandboxmain.modif[10].generic.name           	= "--------:";
		s_sandboxmain.modif[11].generic.name          	= "--------:";
		s_sandboxmain.modif[12].generic.name          	= "--------:";
		s_sandboxmain.modif[13].generic.name           	= "--------:";
		s_sandboxmain.modif[14].generic.name           	= "--------:";
		s_sandboxmain.modif[15].generic.name          	= "--------:";
		s_sandboxmain.modif[16].generic.name          	= "--------:";
		s_sandboxmain.modif[17].generic.name           	= "--------:";
	}
}

/*
===============
SandboxMain_MenuEvent
===============
*/
static void SandboxMain_MenuEvent( void* ptr, int event ) {
	int i;
	if( event != QM_ACTIVATED ) {
		return;
	}
	SandboxMain_SaveChanges();

	for (i = 0; i < 8; i++) {
	    if (((menucommon_s*)ptr)->id == ID_TAB + i) {
			uis.sb_tab = i+1;
			trap_Cmd_ExecuteText( EXEC_INSERT, "menuback; wait 0; ui_sandbox\n" );
	        break;
	    }
	}

	for (i = 0; i < 10; i++) {
	    if (((menucommon_s*)ptr)->id == ID_CTAB + i) {
	        UI_PopMenu();
	        trap_Cmd_ExecuteText(EXEC_INSERT, va("nsgui %s.ns; set lastui nsgui %s.ns\n", get_cvar_char(va("sb_ctab_%i", i + 1)), get_cvar_char(va("sb_ctab_%i", i + 1))));
	        break;
	    }
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SPAWNOBJECT:
		if(uis.sb_tab == STAB_CREATE){
		Q_strncpyz( s_sandboxmain.modif[4].field.buffer, "0", sizeof(s_sandboxmain.modif[4].field.buffer) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "ns_openscript_ui tools/create.ns\n" );
		trap_Cmd_ExecuteText( EXEC_INSERT, va("weapon %i\n", WP_TOOLGUN) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "menuback\n" );	
		}
		if(uis.sb_tab == STAB_ENTITIES){
		Q_strncpyz( s_sandboxmain.modif[4].field.buffer, "0", sizeof(s_sandboxmain.modif[4].field.buffer) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "ns_openscript_ui tools/create.ns\n" );
		trap_Cmd_ExecuteText( EXEC_INSERT, va("weapon %i\n", WP_TOOLGUN) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "menuback\n" );	
		}
		if(uis.sb_tab == STAB_NPC){
		Q_strncpyz( s_sandboxmain.modif[4].field.buffer, "0", sizeof(s_sandboxmain.modif[4].field.buffer) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "ns_openscript_ui tools/create.ns\n" );
		trap_Cmd_ExecuteText( EXEC_INSERT, va("weapon %i\n", WP_TOOLGUN) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "menuback\n" );	
		}
		if(uis.sb_tab == STAB_ITEMS){
		trap_Cmd_ExecuteText( EXEC_INSERT, va("give %s\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		if(uis.sb_tab == STAB_LISTS){
		Q_strncpyz( s_sandboxmain.modif[4].field.buffer, "0", sizeof(s_sandboxmain.modif[4].field.buffer) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "ns_openscript_ui tools/create.ns\n" );
		trap_Cmd_ExecuteText( EXEC_INSERT, va("weapon %i\n", WP_TOOLGUN) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "menuback\n" );	
		}
		if(uis.sb_tab == STAB_SCRIPTS){
		trap_Cmd_ExecuteText( EXEC_INSERT, va("ns_openscript_ui dscripts/%s.ns\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		if(uis.sb_tab == STAB_TOOLS){
		trap_Cmd_ExecuteText( EXEC_INSERT, "menuback\n" );
		}
		if(uis.sb_tab == STAB_ADDONS){
		UI_PopMenu();
		trap_Cmd_ExecuteText( EXEC_INSERT, va("nsgui %s.ns; set lastui nsgui %s.ns\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		break;
	
	case ID_LIST:
		if(uis.sb_tab == STAB_CREATE){
		uis.texturelist_folder = s_sandboxmain.list.curvalue;
		trap_Cmd_ExecuteText( EXEC_INSERT, "menuback; wait 0; ui_sandbox\n" );
		}
		if(uis.sb_tab == STAB_NPC){
		trap_Cmd_ExecuteText( EXEC_NOW, va("set toolcmd_spawn sl npc %s %s %s %s %s %s %s\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue], s_sandboxmain.modif[0].field.buffer, s_sandboxmain.modif[1].field.buffer, s_sandboxmain.modif[2].field.buffer, s_sandboxmain.modif[3].field.buffer, s_sandboxmain.modif[4].field.buffer) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "vstr toolcmd_spawn\n" );
		}
		if(uis.sb_tab == STAB_ITEMS){
		trap_Cmd_ExecuteText( EXEC_INSERT, va("give %s\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		if(uis.sb_tab == STAB_LISTS){
		trap_Cmd_ExecuteText( EXEC_INSERT, va("ns_openscript_ui spawnlists/%s/%s.ns\n", s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue], s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		if(uis.sb_tab == STAB_SCRIPTS){
		trap_Cmd_ExecuteText( EXEC_INSERT, va("ns_openscript_ui dscripts/%s.ns\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		if(uis.sb_tab == STAB_TOOLS){
		Q_strncpyz( s_sandboxmain.modif[4].field.buffer, "0", sizeof(s_sandboxmain.modif[4].field.buffer) );
		trap_Cmd_ExecuteText( EXEC_INSERT, va("ns_openscript_ui tools/%s.ns\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		trap_Cmd_ExecuteText( EXEC_INSERT, va("weapon %i\n", WP_TOOLGUN) );
		}
		if(uis.sb_tab == STAB_ADDONS){
		UI_PopMenu();
		trap_Cmd_ExecuteText( EXEC_INSERT, va("nsgui %s.ns; set lastui nsgui %s.ns\n", s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]) );
		}
		break;

	case ID_TEXTURESLIST:
		trap_Cmd_ExecuteText( EXEC_NOW, va(tool_spawnpreset.string, tool_spawnpreset_arg(1), tool_spawnpreset_arg(2), tool_spawnpreset_arg(3), tool_spawnpreset_arg(4), MODIF_LIST) );
		trap_Cmd_ExecuteText( EXEC_INSERT, "vstr toolcmd_spawn\n" );
		break;
		
	case ID_CLASSLIST:
		if(uis.sb_tab == STAB_ENTITIES){
		trap_Cmd_ExecuteText( EXEC_NOW, va(spawn_preset.string, s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue], s_sandboxmain.classlist.itemnames[s_sandboxmain.classlist.curvalue], s_sandboxmain.priv.curvalue, s_sandboxmain.grid.field.buffer, "0") );
		trap_Cmd_ExecuteText( EXEC_INSERT, "vstr toolcmd_spawn\n" );
		}
		if(uis.sb_tab == STAB_LISTS){
		uis.spawnlist_folder = s_sandboxmain.classlist.curvalue;
		SandboxMain_SpawnListUpdate();
		}
		break;
		
	case ID_SAVEMAP:
		if(uis.sb_tab == STAB_NPC){
		trap_Cvar_Set("bot_pause", "1");
		return;
		}
		if(uis.sb_tab == STAB_ADDONS){
		for (i = 1; i <= 10; i++) {
		if(strlen(get_cvar_char(va("sb_ctab_%i", i))) <= 0){
			NS_setCvar(va("sb_ctab_%i", i), s_sandboxmain.list.itemnames[s_sandboxmain.list.curvalue]);
			trap_Cmd_ExecuteText( EXEC_INSERT, "menuback; wait 0; ui_sandbox\n" );
			trap_Cmd_ExecuteText( EXEC_INSERT, "menuback; wait 0; ui_sandbox\n" );
			return;
		}
		}
		return;
		}
		UI_saveMapEdMenu();
		break;

	case ID_LOADMAP:
		if(uis.sb_tab == STAB_NPC){
		trap_Cvar_Set("bot_pause", "0");
		return;
		}
		if(uis.sb_tab == STAB_ADDONS){
		for (i = 10; i >= 1; i--) {
		if(strlen(get_cvar_char(va("sb_ctab_%i", i))) > 0){
			NS_setCvar(va("sb_ctab_%i", i), "");
			trap_Cmd_ExecuteText( EXEC_INSERT, "menuback; wait 0; ui_sandbox\n" );
			trap_Cmd_ExecuteText( EXEC_INSERT, "menuback; wait 0; ui_sandbox\n" );
			return;
		}
		}
		return;
		}
		UI_loadMapEdMenu();
		break;
	}
}

vec4_t	s_sandboxmain_color1 = {1.00f, 1.00f, 1.00f, 1.00f};

/*
===============
SandboxMain_MenuInit
===============
*/
void SandboxMain_MenuInit( void ) {
	int		y;
	int		gametype;
	char	info[MAX_INFO_STRING];
	int		i;
	int		len;
	char	*configname;
    int name_length;
    const char *bot_name;

	memset( &s_sandboxmain, 0, sizeof(s_sandboxmain) );

	s_sandboxmain.menu.draw = SandboxMain_MenuDraw;
	s_sandboxmain.menu.wrapAround = qtrue;
	s_sandboxmain.menu.native = qfalse;
	s_sandboxmain.menu.fullscreen = qfalse;
	s_sandboxmain.menu.key        = SandboxMain_MenuKey;

	s_sandboxmain.tab[0].string         = "Create";
	s_sandboxmain.tab[1].string         = "Entities";
	s_sandboxmain.tab[2].string         = "NPCs";
	s_sandboxmain.tab[3].string         = "Items";
	s_sandboxmain.tab[4].string         = "Lists";
	s_sandboxmain.tab[5].string         = "Scripts";
	s_sandboxmain.tab[6].string         = "Tools";
	s_sandboxmain.tab[7].string         = "Addons";
	s_sandboxmain.savemap.string		= "Save map";
	s_sandboxmain.loadmap.string		= "Load map";
	s_sandboxmain.priv.generic.name		= "Private:";
	s_sandboxmain.grid.generic.name		= "Grid size:";
	
	for (i = 0; i < 8; i++){
	s_sandboxmain.tab[i].generic.type     	= MTYPE_PTEXT;
	s_sandboxmain.tab[i].generic.flags    	= QMF_CENTER_JUSTIFY;
	s_sandboxmain.tab[i].generic.id       	= ID_TAB+i;
	s_sandboxmain.tab[i].generic.callback 	= SandboxMain_MenuEvent;
	s_sandboxmain.tab[i].generic.x        	= (110*0.5)*(i+1) - uis.wideoffset;
	s_sandboxmain.tab[i].generic.y        	= 30;
	s_sandboxmain.tab[i].color			    = s_sandboxmain_color1;
	s_sandboxmain.tab[i].style			    = UI_CENTER;
	s_sandboxmain.tab[i].customsize			= 0.5;
	s_sandboxmain.tab[i].generic.heightmod	= 1.05;
	}

	for (i = 0; i < 10; i++){
	UI_Free(s_sandboxmain.ctab[i].string);
	s_sandboxmain.ctab[i].generic.type     	= MTYPE_PTEXT;
	s_sandboxmain.ctab[i].generic.flags    	= QMF_CENTER_JUSTIFY;
	s_sandboxmain.ctab[i].generic.id       	= ID_CTAB+i;
	s_sandboxmain.ctab[i].generic.callback 	= SandboxMain_MenuEvent;
	s_sandboxmain.ctab[i].generic.x        	= (110*0.5)*(i+1) - uis.wideoffset;
	s_sandboxmain.ctab[i].generic.y        	= 10;
    s_sandboxmain.ctab[i].string 			= (char *)UI_Alloc(MAX_TAB_TEXT + 1);
    if (s_sandboxmain.ctab[i].string) {
        strncpy(s_sandboxmain.ctab[i].string, get_cvar_char(va("sb_ctab_%i", i+1)), MAX_TAB_TEXT);
        s_sandboxmain.ctab[i].string[MAX_TAB_TEXT] = '\0';
    }
	s_sandboxmain.ctab[i].color			    = s_sandboxmain_color1;
	s_sandboxmain.ctab[i].style			    = UI_CENTER;
	s_sandboxmain.ctab[i].customsize		= 0.5;
	s_sandboxmain.ctab[i].generic.heightmod	= 1.05;
	}
	
	s_sandboxmain.savemap.generic.type     = MTYPE_PTEXT;
	s_sandboxmain.savemap.generic.flags    = QMF_CENTER_JUSTIFY;
	s_sandboxmain.savemap.generic.id       = ID_SAVEMAP;
	s_sandboxmain.savemap.generic.callback = SandboxMain_MenuEvent;
	s_sandboxmain.savemap.generic.x        = 440 + uis.wideoffset;
	s_sandboxmain.savemap.generic.y        = 420 + 5;
	s_sandboxmain.savemap.style            = UI_CENTER;
	s_sandboxmain.savemap.color            = s_sandboxmain_color1;

	s_sandboxmain.loadmap.generic.type     = MTYPE_PTEXT;
	s_sandboxmain.loadmap.generic.flags    = QMF_CENTER_JUSTIFY;
	s_sandboxmain.loadmap.generic.id       = ID_LOADMAP;
	s_sandboxmain.loadmap.generic.callback = SandboxMain_MenuEvent;
	s_sandboxmain.loadmap.generic.x        = 440 + uis.wideoffset;
	s_sandboxmain.loadmap.generic.y        = 420 + 25;
	s_sandboxmain.loadmap.style            = UI_CENTER;
	s_sandboxmain.loadmap.color            = s_sandboxmain_color1;
	
	y = 50;	
	s_sandboxmain.priv.generic.type			= MTYPE_RADIOBUTTON;
	s_sandboxmain.priv.generic.flags		= QMF_SMALLFONT;
	s_sandboxmain.priv.generic.callback		= SandboxMain_MenuEvent;
	s_sandboxmain.priv.generic.id			= ID_PRIV;
	s_sandboxmain.priv.generic.x			= 480 + uis.wideoffset;
	s_sandboxmain.priv.generic.y			= y;
	s_sandboxmain.priv.color				= s_sandboxmain_color1;
	y += 18;

	s_sandboxmain.grid.generic.type			= MTYPE_FIELD;
	s_sandboxmain.grid.generic.flags		= QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_sandboxmain.grid.field.widthInChars	= 4;
	s_sandboxmain.grid.field.maxchars		= 4;
	s_sandboxmain.grid.generic.x			= 480 + uis.wideoffset;
	s_sandboxmain.grid.generic.y			= y;
	s_sandboxmain.grid.color				= s_sandboxmain_color1;
	y += 18;
	
	for(i = 0; i < PROPERTIES_NUM; i++){
	s_sandboxmain.modif[i].generic.type			= MTYPE_FIELD;
	s_sandboxmain.modif[i].generic.flags		= QMF_SMALLFONT;
	s_sandboxmain.modif[i].field.widthInChars	= 14;
	s_sandboxmain.modif[i].field.maxchars		= 64;
	s_sandboxmain.modif[i].generic.x			= 480 + uis.wideoffset;
	s_sandboxmain.modif[i].generic.y			= y;
	s_sandboxmain.modif[i].color				= s_sandboxmain_color1;
	y += 18;
	}
	
	s_sandboxmain.spawnobject.generic.type     = MTYPE_PTEXT;
	s_sandboxmain.spawnobject.generic.flags    = QMF_CENTER_JUSTIFY;
	s_sandboxmain.spawnobject.generic.id       = ID_SPAWNOBJECT;
	s_sandboxmain.spawnobject.generic.callback = SandboxMain_MenuEvent;
	s_sandboxmain.spawnobject.generic.x        = 560 + uis.wideoffset;
	s_sandboxmain.spawnobject.generic.y        = 448;
	s_sandboxmain.spawnobject.color					= s_sandboxmain_color1;
	s_sandboxmain.spawnobject.style					= UI_CENTER;

	if(uis.sb_tab == STAB_CREATE){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 2;
	s_sandboxmain.list.columns			= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.list.string			= "props";
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.corner			= 65;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 62;
	s_sandboxmain.list.width			= 39/6;
	s_sandboxmain.list.height			= 4;
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( "props", "md3", s_sandboxmain.names, 524288 );
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.texturelist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.texturelist.type				= 5;
	s_sandboxmain.texturelist.styles			= 2;
	s_sandboxmain.texturelist.columns			= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.texturelist.fontsize			= 1;
	s_sandboxmain.texturelist.corner			= 65;
	s_sandboxmain.texturelist.generic.flags		= QMF_PULSEIFFOCUS;
	s_sandboxmain.texturelist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.texturelist.generic.id		= ID_TEXTURESLIST;
	s_sandboxmain.texturelist.generic.x			= 40 - uis.wideoffset;
	s_sandboxmain.texturelist.generic.y			= 215 + 62;
	s_sandboxmain.texturelist.width				= 39/6;
	s_sandboxmain.texturelist.height			= 4;
	s_sandboxmain.texturelist.itemnames			= (const char **)s_sandboxmain.textureslist;
	s_sandboxmain.texturelist.color				= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Props:";
	s_sandboxmain.classtext.string  				= "Textures:";
	s_sandboxmain.spawnobject.string           		= "Create";
	}
	if(uis.sb_tab == STAB_ENTITIES){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 1;
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 70;
	s_sandboxmain.list.width			= 28;
	s_sandboxmain.list.height			= 15+18;
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( "props", "md3", s_sandboxmain.names, 524288 );
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.columns			= 1;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles			= 2;
	s_sandboxmain.classlist.columns			= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.classlist.string			= "";
	s_sandboxmain.classlist.fontsize		= 1;
	s_sandboxmain.classlist.corner			= 40;
	s_sandboxmain.classlist.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 70;
	s_sandboxmain.classlist.width			= 39/6;
	s_sandboxmain.classlist.height			= 8;
	s_sandboxmain.classlist.numitems		= 70;
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.classeslist;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Props:";
	s_sandboxmain.classtext.string  				= "Entities:";
	s_sandboxmain.spawnobject.string           		= "Create";
	}
	if(uis.sb_tab == STAB_NPC){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 2;
	s_sandboxmain.list.columns			= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.list.string			= "bots";
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.corner			= 65;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 62;
	s_sandboxmain.list.width			= 39/6;
	s_sandboxmain.list.height			= 4;
	s_sandboxmain.list.numitems			= UI_GetNumBots();
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles				= 1;
	s_sandboxmain.classlist.fontsize			= 1;
	s_sandboxmain.classlist.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 215 + 70;
	s_sandboxmain.classlist.width			= 39+(2*uis.wideoffset/SMALLCHAR_WIDTH);
	s_sandboxmain.classlist.height			= 15;
	s_sandboxmain.classlist.numitems		= 5;
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.classeslist;
	s_sandboxmain.classlist.columns			= 1;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "List:";
	s_sandboxmain.classtext.string  				= "Class:";
	s_sandboxmain.savemap.string					= "Disable AI";
	s_sandboxmain.loadmap.string					= "Enable AI";
	s_sandboxmain.spawnobject.string           		= "Create";
	}
	if(uis.sb_tab == STAB_ITEMS){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 2;
	s_sandboxmain.list.columns			= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.list.string			= "";
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.corner			= 65;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 70;
	s_sandboxmain.list.width			= 39/6;
	s_sandboxmain.list.height			= 8;
	s_sandboxmain.list.numitems			= 62;
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.item_itemslist;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles				= 1;
	s_sandboxmain.classlist.fontsize			= 1;
	s_sandboxmain.classlist.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 215 + 70;
	s_sandboxmain.classlist.width			= 28;
	s_sandboxmain.classlist.height			= 15;
	s_sandboxmain.classlist.numitems		= 118;
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.botclasslist;
	s_sandboxmain.classlist.columns			= 1;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Items:";
	s_sandboxmain.classtext.string  				= "Class:";
	s_sandboxmain.spawnobject.string          		= "Give";
	}
	if(uis.sb_tab == STAB_LISTS){
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles				= 2;
	s_sandboxmain.classlist.columns				= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.classlist.string 				= "spawnlists/icons";
	s_sandboxmain.classlist.fontsize			= 1;
	s_sandboxmain.classlist.corner				= 65;
	s_sandboxmain.classlist.generic.flags		= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 215 + 62;
	s_sandboxmain.classlist.width			= 39/6;
	s_sandboxmain.classlist.height			= 4;
	s_sandboxmain.classlist.numitems		= trap_FS_GetFileList( "spawnlists", "cfg", s_sandboxmain.names2, 524288 );
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.classeslist;
	s_sandboxmain.classlist.columns			= 1;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
		
	UI_Free(s_sandboxmain.list.string);
		
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 2;
	s_sandboxmain.list.columns			= 6+((1.75*uis.wideoffset)/((39/6)*SMALLCHAR_WIDTH-7));
	s_sandboxmain.list.string 			= (char *)UI_Alloc(256);
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.corner			= 65;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 62;
	s_sandboxmain.list.width			= 39/6;
	s_sandboxmain.list.height			= 4;
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Items:";
	s_sandboxmain.classtext.string  				= "Lists:";
	s_sandboxmain.spawnobject.string           		= "Create";
	}
	if(uis.sb_tab == STAB_SCRIPTS){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 0;
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 70;
	s_sandboxmain.list.width			= 39+(2*uis.wideoffset/SMALLCHAR_WIDTH);
	s_sandboxmain.list.height			= 15+18;
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( "dscripts", "ns", s_sandboxmain.names, 524288 );
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.columns			= 1;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles				= 1;
	s_sandboxmain.classlist.fontsize			= 1;
	s_sandboxmain.classlist.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 215 + 70;
	s_sandboxmain.classlist.width			= 28;
	s_sandboxmain.classlist.height			= 15;
	s_sandboxmain.classlist.numitems		= 118;
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.botclasslist;
	s_sandboxmain.classlist.columns			= 1;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Scripts:";
	s_sandboxmain.classtext.string  				= "Class:";
	s_sandboxmain.spawnobject.string          		= "Execute";
	}
	if(uis.sb_tab == STAB_TOOLS){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 0;
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 70;
	s_sandboxmain.list.width			= 39+(2*uis.wideoffset/SMALLCHAR_WIDTH);
	s_sandboxmain.list.height			= 15+18;
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( "tools", "ns", s_sandboxmain.names, 524288 );
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.columns			= 1;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles				= 1;
	s_sandboxmain.classlist.fontsize			= 1;
	s_sandboxmain.classlist.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 215 + 70;
	s_sandboxmain.classlist.width			= 28;
	s_sandboxmain.classlist.height			= 15;
	s_sandboxmain.classlist.numitems		= 118;
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.botclasslist;
	s_sandboxmain.classlist.columns			= 1;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Tools:";
	s_sandboxmain.classtext.string  				= "Class:";
	s_sandboxmain.spawnobject.string          		= "Select";
	}
	if(uis.sb_tab == STAB_ADDONS){
	s_sandboxmain.list.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.list.type				= 5;
	s_sandboxmain.list.styles			= 1;
	s_sandboxmain.list.fontsize			= 1;
	s_sandboxmain.list.string			= "nsgui/icons";
	s_sandboxmain.list.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.list.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.list.generic.id		= ID_LIST;
	s_sandboxmain.list.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.list.generic.y		= 70;
	s_sandboxmain.list.width			= 39+(2*uis.wideoffset/SMALLCHAR_WIDTH);
	s_sandboxmain.list.height			= 15+18;
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( "nsgui", "ns", s_sandboxmain.names, 524288 );
	s_sandboxmain.list.itemnames		= (const char **)s_sandboxmain.configlist;
	s_sandboxmain.list.columns			= 1;
	s_sandboxmain.list.color			= s_sandboxmain_color1;
	
	s_sandboxmain.classlist.generic.type		= MTYPE_UIOBJECT;
	s_sandboxmain.classlist.type				= 5;
	s_sandboxmain.classlist.styles				= 1;
	s_sandboxmain.classlist.fontsize			= 1;
	s_sandboxmain.classlist.generic.flags	= QMF_PULSEIFFOCUS;
	s_sandboxmain.classlist.generic.callback	= SandboxMain_MenuEvent;
	s_sandboxmain.classlist.generic.id		= ID_CLASSLIST;
	s_sandboxmain.classlist.generic.x		= 40 - uis.wideoffset;
	s_sandboxmain.classlist.generic.y		= 215 + 70;
	s_sandboxmain.classlist.width			= 28;
	s_sandboxmain.classlist.height			= 15;
	s_sandboxmain.classlist.numitems		= 118;
	s_sandboxmain.classlist.itemnames		= (const char **)s_sandboxmain.botclasslist;
	s_sandboxmain.classlist.columns			= 1;
	s_sandboxmain.classlist.color			= s_sandboxmain_color1;
	//y += 20;
	
	s_sandboxmain.propstext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.propstext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.propstext.generic.y				= 48;
	s_sandboxmain.propstext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.propstext.color  					= s_sandboxmain_color1;
	s_sandboxmain.propstext.style  					= UI_BIGFONT;
	
	s_sandboxmain.classtext.generic.type			= MTYPE_PTEXT;
	s_sandboxmain.classtext.generic.x				= 40 - uis.wideoffset;
	s_sandboxmain.classtext.generic.y				= 215 + 48;
	s_sandboxmain.classtext.generic.flags			= QMF_INACTIVE;
	s_sandboxmain.classtext.color  					= s_sandboxmain_color1;
	s_sandboxmain.classtext.style  					= UI_BIGFONT;
	
	s_sandboxmain.propstext.string  				= "Addons:";
	s_sandboxmain.classtext.string  				= "";
	s_sandboxmain.savemap.string					= "Add as tab";
	s_sandboxmain.loadmap.string					= "Delete tab";
	s_sandboxmain.spawnobject.string           		= "Open";
	}

	y = 480+240;

if(uis.sb_tab == STAB_CREATE){
	if (!s_sandboxmain.list.numitems) {
		strcpy(s_sandboxmain.names,"No models");
		s_sandboxmain.list.numitems = 1;
	}
	else if (s_sandboxmain.list.numitems > 65536)
		s_sandboxmain.list.numitems = 65536;

	configname = s_sandboxmain.names;
	for ( i = 0; i < s_sandboxmain.list.numitems; i++ ) {
		s_sandboxmain.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 4,".md3"))
			configname[len-4] = '\0';

		configname += len + 1;
	}
	
	s_sandboxmain.texturelist.string			= sb_texture.string;
	s_sandboxmain.texturelist.numitems			= trap_FS_GetFileList( va("ptex/%s", s_sandboxmain.list.itemnames[uis.texturelist_folder]), "png", s_sandboxmain.names2, 524288 );
	
	if (!s_sandboxmain.texturelist.numitems) {
		strcpy(s_sandboxmain.names2,"0");
		s_sandboxmain.texturelist.numitems = 1;
	}
	else if (s_sandboxmain.texturelist.numitems > 65536)
		s_sandboxmain.texturelist.numitems = 65536;

	configname = s_sandboxmain.names2;
	for ( i = 0; i < s_sandboxmain.texturelist.numitems; i++ ) {
		s_sandboxmain.texturelist.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 4,".png"))
			configname[len-4] = '\0';

		configname += len + 1;
	}
}
if (uis.sb_tab == STAB_NPC) {
    if (!s_sandboxmain.list.numitems) {
        strcpy(s_sandboxmain.names, "No NPC");
        s_sandboxmain.list.numitems = 1;
    } else if (s_sandboxmain.list.numitems > 65536) {
        s_sandboxmain.list.numitems = 65536;
    }

    configname = s_sandboxmain.names;
    
    for (i = 0; i < s_sandboxmain.list.numitems; i++) {
        // Получение имени из информации о боте
        bot_name = Info_ValueForKey(UI_GetBotInfoByNumber(i), "name");

        // Проверка длины строки, чтобы избежать переполнения буфера
        name_length = strlen(bot_name);

        // Копирование имени в массив имен
        strcpy(configname, bot_name);

        // Установка имени в массив itemnames
        s_sandboxmain.list.itemnames[i] = configname;

        // Переход к следующему элементу
        configname += name_length + 1;
    }
}

if(uis.sb_tab == STAB_LISTS){	
	if (!s_sandboxmain.classlist.numitems) {
		strcpy(s_sandboxmain.names2,"No lists");
		s_sandboxmain.classlist.numitems = 1;
	}
	else if (s_sandboxmain.classlist.numitems > 65536)
		s_sandboxmain.classlist.numitems = 65536;

	configname = s_sandboxmain.names2;
	for ( i = 0; i < s_sandboxmain.classlist.numitems; i++ ) {
		s_sandboxmain.classlist.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 4,".cfg"))
			configname[len-4] = '\0';

		configname += len + 1;
	}
	
	s_sandboxmain.list.numitems			= trap_FS_GetFileList( va("spawnlists/%s", s_sandboxmain.classlist.itemnames[uis.spawnlist_folder]), "ns", s_sandboxmain.names, 524288 );
	
	if (!s_sandboxmain.list.numitems) {
		strcpy(s_sandboxmain.names,"No items");
		s_sandboxmain.list.numitems = 1;
	}
	else if (s_sandboxmain.list.numitems > 65536)
		s_sandboxmain.list.numitems = 65536;

	configname = s_sandboxmain.names;
	for ( i = 0; i < s_sandboxmain.list.numitems; i++ ) {
		s_sandboxmain.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 3,".ns"))
			configname[len-3] = '\0';

		configname += len + 1;
	}
	
	strcpy(s_sandboxmain.list.string, va("spawnlists/%s/icons", s_sandboxmain.classlist.itemnames[uis.spawnlist_folder]));
}
if(uis.sb_tab == STAB_SCRIPTS){
	if (!s_sandboxmain.list.numitems) {
		strcpy(s_sandboxmain.names,"No scripts");
		s_sandboxmain.list.numitems = 1;
	}
	else if (s_sandboxmain.list.numitems > 65536)
		s_sandboxmain.list.numitems = 65536;

	configname = s_sandboxmain.names;
	for ( i = 0; i < s_sandboxmain.list.numitems; i++ ) {
		s_sandboxmain.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 3,".ns"))
			configname[len-3] = '\0';

		configname += len + 1;
	}
}
if(uis.sb_tab == STAB_TOOLS){
	if (!s_sandboxmain.list.numitems) {
		strcpy(s_sandboxmain.names,"No tools");
		s_sandboxmain.list.numitems = 1;
	}
	else if (s_sandboxmain.list.numitems > 65536)
		s_sandboxmain.list.numitems = 65536;

	configname = s_sandboxmain.names;
	for ( i = 0; i < s_sandboxmain.list.numitems; i++ ) {
		s_sandboxmain.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 3,".ns"))
			configname[len-3] = '\0';

		configname += len + 1;
	}
}
if(uis.sb_tab == STAB_ADDONS){	
	if (!s_sandboxmain.list.numitems) {
		strcpy(s_sandboxmain.names,"No addons");
		s_sandboxmain.list.numitems = 1;
	}
	else if (s_sandboxmain.list.numitems > 65536)
		s_sandboxmain.list.numitems = 65536;

	configname = s_sandboxmain.names;
	for ( i = 0; i < s_sandboxmain.list.numitems; i++ ) {
		s_sandboxmain.list.itemnames[i] = configname;

		// strip extension
		len = strlen( configname );
		if (!Q_stricmp(configname +  len - 3,".ns"))
			configname[len-3] = '\0';


		configname += len + 1;
	}
}

	if(uis.sb_tab == STAB_ITEMS){
		for (i = 0; i < 62; i++) {
			s_sandboxmain.list.itemnames[i] = item_items[i];
		}
	}
	if(uis.sb_tab == STAB_ENTITIES){
		for (i = 0; i < 70; i++) {
			s_sandboxmain.classlist.itemnames[i] = entity_items[i];
		}
	}
	if(uis.sb_tab == STAB_NPC){
		s_sandboxmain.classlist.itemnames[0] = "NPC_Citizen";
		s_sandboxmain.classlist.itemnames[1] = "NPC_Enemy";
		s_sandboxmain.classlist.itemnames[2] = "NPC_Guard";
		s_sandboxmain.classlist.itemnames[3] = "NPC_Partner";
		s_sandboxmain.classlist.itemnames[4] = "NPC_PartnerEnemy";
	}
	
	for (i = 0; i < 8; i++){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.tab[i] );
	}
	for (i = 0; i < 10; i++){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.ctab[i] );
	}
	Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.spawnobject );
	Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.priv );
	Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.grid );
	Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.savemap );
	Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.loadmap );
	if(uis.sb_tab == STAB_CREATE){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classtext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.texturelist );
	}
	if(uis.sb_tab == STAB_ENTITIES){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classtext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classlist );
	}
	if(uis.sb_tab == STAB_NPC){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classtext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classlist );
	}
	if(uis.sb_tab == STAB_ITEMS){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
	}
	if(uis.sb_tab == STAB_LISTS){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classtext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.classlist );
	}
	if(uis.sb_tab == STAB_SCRIPTS){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
	}
	if(uis.sb_tab == STAB_TOOLS){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
	}
	if(uis.sb_tab == STAB_ADDONS){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.propstext );
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.list );
	}
	
	for(i = 0; i < PROPERTIES_NUM; i++){
		Menu_AddItem( &s_sandboxmain.menu, (void*) &s_sandboxmain.modif[i] );
	}
	
	s_sandboxmain.priv.curvalue = trap_Cvar_VariableValue("sb_private");
	Q_strncpyz( s_sandboxmain.grid.field.buffer, UI_Cvar_VariableString("sb_grid"), sizeof(s_sandboxmain.grid.field.buffer) );
	s_sandboxmain.list.curvalue = trap_Cvar_VariableValue("sb_modelnum");
	s_sandboxmain.classlist.curvalue = trap_Cvar_VariableValue("sb_classnum");
	s_sandboxmain.texturelist.curvalue = trap_Cvar_VariableValue("sb_texturenum");
	Q_strncpyz( s_sandboxmain.modif[0].field.buffer, UI_Cvar_VariableString("toolgun_mod1"), sizeof(s_sandboxmain.modif[0].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[1].field.buffer, UI_Cvar_VariableString("toolgun_mod2"), sizeof(s_sandboxmain.modif[1].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[2].field.buffer, UI_Cvar_VariableString("toolgun_mod3"), sizeof(s_sandboxmain.modif[2].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[3].field.buffer, UI_Cvar_VariableString("toolgun_mod4"), sizeof(s_sandboxmain.modif[3].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[4].field.buffer, UI_Cvar_VariableString("toolgun_mod5"), sizeof(s_sandboxmain.modif[4].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[5].field.buffer, UI_Cvar_VariableString("toolgun_mod6"), sizeof(s_sandboxmain.modif[5].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[6].field.buffer, UI_Cvar_VariableString("toolgun_mod7"), sizeof(s_sandboxmain.modif[6].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[7].field.buffer, UI_Cvar_VariableString("toolgun_mod8"), sizeof(s_sandboxmain.modif[7].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[8].field.buffer, UI_Cvar_VariableString("toolgun_mod9"), sizeof(s_sandboxmain.modif[8].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[9].field.buffer, UI_Cvar_VariableString("toolgun_mod10"), sizeof(s_sandboxmain.modif[9].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[10].field.buffer, UI_Cvar_VariableString("toolgun_mod11"), sizeof(s_sandboxmain.modif[10].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[11].field.buffer, UI_Cvar_VariableString("toolgun_mod12"), sizeof(s_sandboxmain.modif[11].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[12].field.buffer, UI_Cvar_VariableString("toolgun_mod13"), sizeof(s_sandboxmain.modif[12].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[13].field.buffer, UI_Cvar_VariableString("toolgun_mod14"), sizeof(s_sandboxmain.modif[13].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[14].field.buffer, UI_Cvar_VariableString("toolgun_mod15"), sizeof(s_sandboxmain.modif[14].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[15].field.buffer, UI_Cvar_VariableString("toolgun_mod16"), sizeof(s_sandboxmain.modif[15].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[16].field.buffer, UI_Cvar_VariableString("toolgun_mod17"), sizeof(s_sandboxmain.modif[16].field.buffer) );
	Q_strncpyz( s_sandboxmain.modif[17].field.buffer, UI_Cvar_VariableString("toolgun_mod18"), sizeof(s_sandboxmain.modif[17].field.buffer) );
	trap_Cvar_Set( "sb_texture", va("ptex/%s", s_sandboxmain.list.itemnames[uis.texturelist_folder]) );
	trap_Cvar_Set( "sb_texture_view", va("ptex/props/%s", s_sandboxmain.list.itemnames[uis.texturelist_folder]) );
	
	if(uis.sb_tab == STAB_LISTS){
		SandboxMain_SpawnListUpdate();
	}
	
	if(uis.sb_tab == STAB_CREATE){
		s_sandboxmain.classlist.curvalue = 0;
	} else {
		s_sandboxmain.texturelist.curvalue = 0;
	}
}

/*
===============
UI_SandboxMainMenu
===============
*/
void UI_SandboxMainMenu( void ) {
	if(DynamicMenu_ServerGametype() == GT_SANDBOX){
	if(!uis.sb_tab){ uis.sb_tab = 1;}
	SandboxMain_MenuInit();
	UI_PushMenu ( &s_sandboxmain.menu );
	}
}
