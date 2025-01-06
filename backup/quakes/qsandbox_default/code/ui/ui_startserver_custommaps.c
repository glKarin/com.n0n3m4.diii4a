/*
=============================================================================

MAP SELECT MENU *****

=============================================================================
*/


#include "ui_local.h"
#include "ui_startserver.h"



#define MAX_MAPSLIST 2048
#define MAP_ERRORPIC "menu/art/unknownmap"


#define MAPTYPE_MASTER_BEGIN 	0
#define MAPTYPE_MASTER_END 		1
#define MAPTYPE_CUSTOM_BEGIN 	2
#define MAPTYPE_CUSTOM_END 		3

#define MAPTYPE_ICONX	20
#define MAPTYPE_ICONY	20

#define TMP_BUFSIZE 64

#define GROUP_INDEX "[Index]"



const char* mapfilter_items[MAPFILTER_MAX + MAX_MAPTYPES + 1] = {
	"Off",	// MAPFILTER_OFF
	"QS",	// MAPFILTER_DMod
	"Others",	// MAPFILTER_NONID
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0
};

const char* mapfilter_itemsru[MAPFILTER_MAX + MAX_MAPTYPES + 1] = {
	"Откл",	// MAPFILTER_OFF
	"QS",	// MAPFILTER_DMod
	"Другие",	// MAPFILTER_NONID
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0
};

typedef struct mapTypeList_s {
	int num_maptypes;
	int num_maps;
	int noBotsIndex;

	int type_offset[MAX_MAPTYPES][4];

	char mapTypeName[MAX_MAPTYPES][MAPNAME_BUFFER + 2];
	char mapGraphic[MAX_MAPTYPES][MAX_QPATH];

	char mapName[MAX_MAPSLIST][MAPNAME_BUFFER];
} mapTypeList_t;


static mapTypeList_t s_mapList;
static qboolean maplist_loaded = qfalse;
static vec4_t faded_color = {1.0, 1.0, 1.0, 1.0 };
static vec4_t shadow_color = {0.0, 0.0, 0.0, 1.0 };



static const char* maptype_icon[NUM_GAMETYPES] = {
	"menu/uie_art/gt_sandbox",		// GT_SANDBOX
	"menu/uie_art/gt_ffa",		// GT_FFA
	"menu/uie_art/gt_single",		// GT_SINGLE
	"menu/uie_art/gt_tourney",		// GT_TOURNAMENT
	"menu/uie_art/gt_team",	// GT_TEAM
	"menu/uie_art/gt_ctf",		// GT_CTF
	"menu/uie_art/gt_oneflag",		// GT_1FCTF
	"menu/uie_art/gt_obelisk",		// GT_OBELISK
	"menu/uie_art/gt_harvester",		// GT_HARVESTER
	"menu/uie_art/gt_elimination",		// GT_ELIMINATION
	"menu/uie_art/gt_eliminationctf",		// GT_CTF_ELIMINATION
	"menu/uie_art/gt_lms",		// GT_LMS
	"menu/uie_art/gt_doubledom",		// GT_DOUBLE_D
	"menu/uie_art/gt_dom"		// GT_DOMINATION
};





/*
=================
GametypeBits
=================
*/
int GametypeBits( char *string ) {
	int		bits;
	char	*p;
	char	*token;

	bits = 0;
	p = string;
	while( 1 ) {
		token = COM_ParseExt( &p, qfalse );
		if( token[0] == 0 ) {
			break;
		}
		
		if( Q_stricmp( token, "sandbox" ) == 0 ) {
			bits |= 1 << GT_SANDBOX;
			continue;
		}

		if( Q_stricmp( token, "ffa" ) == 0 ) {
			bits |= 1 << GT_FFA;
			bits |= 1 << GT_LMS;
			bits |= 1 << GT_ELIMINATION;
			bits |= 1 << GT_DOMINATION;
			continue;
		}
		
		if( Q_stricmp( token, "entityplus" ) == 0 ) { //entityplus support
			bits |= 1 << GT_SINGLE;
			continue;
		}
		
		if( Q_stricmp( token, "singleplayer" ) == 0 ) {	//Quake Sandbox single mode
			bits |= 1 << GT_SINGLE;
			continue;
		}

		if( Q_stricmp( token, "tourney" ) == 0 ) {
			bits |= 1 << GT_TOURNAMENT;
			continue;
		}

		if( Q_stricmp( token, "team" ) == 0 ) {
			bits |= 1 << GT_TEAM;
			bits |= 1 << GT_ELIMINATION;
			continue;
		}

		if( Q_stricmp( token, "ctf" ) == 0 ) {
			bits |= 1 << GT_CTF;
			bits |= 1 << GT_DOUBLE_D;
			bits |= 1 << GT_CTF_ELIMINATION;
			bits |= 1 << GT_ELIMINATION;
			continue;
		}
		
		if( Q_stricmp( token, "oneflag" ) == 0 ) {
			bits |= 1 << GT_1FCTF;
			continue;
		}
                
		if( Q_stricmp( token, "overload" ) == 0 ) {
			bits |= 1 << GT_OBELISK;
			continue;
		}
                
		if( Q_stricmp( token, "harvester" ) == 0 ) {
			bits |= 1 << GT_HARVESTER;
			continue;
		}

		if( Q_stricmp( token, "elimination" ) == 0 ) {
			bits |= 1 << GT_ELIMINATION;
			continue;
		}

		if( Q_stricmp( token, "ctfelimination" ) == 0 ) {
			bits |= 1 << GT_CTF_ELIMINATION;
			continue;
	}

		if( Q_stricmp( token, "lms" ) == 0 ) {
			bits |= 1 << GT_LMS;
			continue;
		}
		if( Q_stricmp( token, "dd" ) == 0 ) {
			bits |= 1 << GT_DOUBLE_D;
			continue;
		}
                
		if( Q_stricmp( token, "dom" ) == 0 ) {
			bits |= 1 << GT_DOMINATION;
			continue;
		}
	}

	return bits;
}




/*
=================
UI_BuildMapListByType

If list is NULL then we're just counting the
number of maps that match gametype

gametype == -1

Can be used externally to the ui_startserver.c subsystem
=================
*/
int UI_BuildMapListByType(int* list, int listmax, int gametype,
	callbackMapList callback)
{
	int count;
	int i, nummaps;
	int matchbits, gamebits;
	const char* info;
	const char* mapname;
	qboolean idmap;

	count = 0;
	if (gametype == -1) {
		matchbits = (1 << GT_MAX_GAME_TYPE) - 1;
	}
	else {
		matchbits = 1 << gametype;
	}

	nummaps = UI_GetNumArenas();
	for (i = 0; i < nummaps; i++)
	{
		info = UI_GetArenaInfoByNumber(i);

		gamebits = GametypeBits( Info_ValueForKey( info, "type") );
		if( !( gamebits & matchbits )) {
			continue;
		}

		if (callback && !callback(info))
			continue;

		// add to list, terminate if full
		count++;
		if (!list)
			continue;

		list[count - 1] = i;
		if (count == listmax)
			break;
	}

	return count;
}






/*
=================
UIE_DefaultIconFromGameType
=================
*/
const char* UIE_DefaultIconFromGameType(int gametype)
{
	if (gametype < 0 || gametype > NUM_GAMETYPES) {
		trap_Print(va(S_COLOR_RED"Unknown gametype icon: %i\n", gametype));
		return NULL;
	}

	return maptype_icon[ gametype ];
}




/*
=================
StartServer_SetIconFromGameType

gametype < 0 clears the bitmap so nothing is drawn

Modifies path to icon to get high quality Id version
if needed. Note: these Id icons don't have the
proper transparency behaviour
=================
*/
void StartServer_SetIconFromGameType(menubitmap_s* b, int gametype, qboolean custom)
{
	const char* new_icon;

	if (!b)
		return;

	if (gametype < 0)
	{
		b->generic.name = 0;
		b->shader = 0;
		return;
	}

	new_icon = StartServer_MapIconFromType(gametype, custom);

	if (new_icon != b->generic.name)
	{
		b->generic.name = new_icon;
		b->shader = 0;
	}
}




/*
=================
StartServer_CreateMapType
=================
*/
static qboolean StartServer_CreateMapType(const char* name, const char* graphic)
{
	int i;
	qboolean duplicated;
	char* ptr;

	ptr = va("[%s]",name);
	duplicated = qfalse;
	for (i = 0; i < s_mapList.num_maptypes; i++)
	{
		if (!Q_stricmp(ptr, s_mapList.mapTypeName[i]))
		{
			duplicated = qtrue;
			break;
		}
	}

	if (duplicated || !Q_stricmp(ptr, GROUP_INDEX))
	{
		return qfalse;
	}

	strcpy(s_mapList.mapTypeName[s_mapList.num_maptypes], ptr);
	strcpy(s_mapList.mapGraphic[s_mapList.num_maptypes], graphic);
	s_mapList.num_maptypes++;

	return qtrue;
}



/*
=================
StartServer_LoadCustomMapData
=================
*/
static void StartServer_LoadCustomMapData(const char* filename, qboolean merge)
{
	static char data[65000];
	char buf[TMP_BUFSIZE];
	fileHandle_t file;
	int len;

	char *text_p, *prev_p;
	char *token, *token2;
	char *ptr;
	int i;
	int index;
	int first, last;
	qboolean indexgroup, groupfound, indexdone;
	qboolean groupused[MAX_MAPTYPES];

	// setup parameters
	for (i = 0; i < MAX_MAPTYPES; i++)
		groupused[i] = qfalse;

	if (merge)
	{
		first = MAPTYPE_CUSTOM_BEGIN;
		last = MAPTYPE_CUSTOM_END;
	}
	else
	{
		first = MAPTYPE_MASTER_BEGIN;
		last = MAPTYPE_MASTER_END;
	}


	//
	// read the file
	//
	len = trap_FS_FOpenFile(filename, &file, FS_READ);
	if (len <= 0 )
	{
		return;
	}

	if ( len >= (sizeof(data) - 1))
	{
		Com_Printf( "UI_LoadCustomMapData: %s larger than buffer\n", filename );
		return;
	}

	trap_FS_Read(data, len, file);
	trap_FS_FCloseFile(file);
	data[len] = 0;

	// parse the data file
	groupfound = qfalse;
	indexgroup = qfalse;
	indexdone = qfalse;
	index = -1;
	text_p = data;
	while (1)
	{
		prev_p = text_p;	// for backup
		token = COM_Parse(&text_p);
		if (!text_p)
			break;

		if (token[0] == '[')
		{
			// close previous group
			if (groupfound)
			{
				s_mapList.type_offset[index][last] = s_mapList.num_maps;
				groupused[index] = qtrue;
				groupfound = qfalse;
			}

			// check for "[name]" format
			ptr = strchr(token, ']');
			if (strchr(&token[1],'[') || !ptr || ptr[1])
			{
				Com_Printf( "(%s): has malformed group (%s)\n", filename, token);
				break;
			}

			// we have a valid "[name]" tokem
			if (indexgroup)
			{
				indexgroup = qfalse;
				indexdone = qtrue;
			}

			if (!indexgroup && !indexdone)
			{
				if (Q_stricmp(token, GROUP_INDEX))
				{
					Com_Printf( "(%s): must have %s group first\n", filename, GROUP_INDEX);
					break;
				}
				indexgroup = qtrue;
				continue;
			}

			// locate
			index = -1;
			for (i = 0; i < MAX_MAPTYPES; i++)
			{
				if (!Q_stricmp(token, s_mapList.mapTypeName[i]))
				{
					index = i;
					break;
				}
			}

			// failed to locate, or duplicated
			if ( index == -1 || groupused[index] || !Q_stricmp(token, GROUP_INDEX) )
			{
				Com_Printf("(%s): %s ignored\n", filename, token);
				continue;
			}

			// have a valid value of index at this point
			groupfound = qtrue;
			s_mapList.type_offset[index][first] = s_mapList.num_maps;
			continue;
		}

		//
		// not a new index header, so we treat token as a map name or description of index
		//
		if (indexgroup)
		{
			//
			// "name=graphic file" format
			//
			ptr = strchr(token, '=');
			buf[0] = 0;

			if (ptr)
			{
				Q_strncpyz(buf, token, ptr - token + 1);
				if (!ptr[1])
				{
					token = COM_Parse(&text_p);
					if (!text_p)
						break;
				}
				else
				{
					token = &ptr[1];
				}
			}
			else	// token is separated from equals by whitespace
			{
				Q_strncpyz(buf, token, TMP_BUFSIZE);

				token = COM_Parse(&text_p);
				if (!text_p)
					break;

				if (token[0] != '=')
				{
					Com_Printf( "(%s): (%s) requires = separator\n", filename, buf);
					break;
				}

				if (!token[1])
				{
					token = COM_Parse(&text_p);
					if (!text_p)
						break;
				}
				else
				{
					token = &token[1];
				}
			}

			// found a type of map, with associated graphic
			if (!StartServer_CreateMapType(buf, token))
			{
				Com_Printf( "(%s): (%s) duplicated, ignored\n", filename, buf);
			}
			continue;
		}
		else
		{
			if (!groupfound)
			{
				continue;
			}

			// map name, mustn't be too long
			if (strlen(token) >= MAPNAME_BUFFER - 1)
			{
				Com_Printf( "(%s): mapname too long (%s)\n", filename, token);
				break;
			}

			strcpy(s_mapList.mapName[s_mapList.num_maps++ ], token);
		}
	}

	if (index != -1)
		s_mapList.type_offset[index][last] = s_mapList.num_maps;
}




/*
=================
StartServer_MapSupportsBots

=================
*/
qboolean StartServer_MapSupportsBots(const char* mapname)
{
	int i;
	int index, start, end;

	index = s_mapList.noBotsIndex;
	if (index < 0)
		return qtrue;

	start = s_mapList.type_offset[index][MAPTYPE_MASTER_BEGIN];
	end = s_mapList.type_offset[index][MAPTYPE_MASTER_END];
	for (i = start; i < end; i++)
	{
		if (!Q_stricmp(mapname, s_mapList.mapName[i]))
			return qfalse;
	}

	return qtrue;
}




/*
=================
StartServer_AddMapType
=================
*/
static void StartServer_AddMapType(mappic_t* mappic, int type)
{
	int i;

	if (mappic->num_types == MAX_MAPTYPES)
		return;

	// prevent duplication
	for (i = 0; i < mappic->num_types; i++)
	{
		if (mappic->type[i] == type)
			return;
	}

	mappic->type[ mappic->num_types++ ] = type;
}




/*
=================
StartServer_InitMapPictureFromIndex
=================
*/
void StartServer_InitMapPictureFromIndex(mappic_t* mappic, int index)
{
	int i, j, tmp;
	const char* info;
	char mapname[MAPNAME_BUFFER];

	memset(mappic, 0, sizeof(mappic_t));

	if (!maplist_loaded)
		UI_LoadMapTypeInfo();

	info = UI_GetArenaInfoByNumber( index );
	mappic->gamebits = GametypeBits( Info_ValueForKey( info, "type") );

	Q_strncpyz(mapname, Info_ValueForKey( info, "map"), MAPNAME_BUFFER);

	// find map on master and custom lists
	for (i = 0; i < s_mapList.num_maps; i++)
	{
		if (Q_stricmp(mapname, s_mapList.mapName[i]))
			continue;

		// find map in list
		for (j = 0; j < s_mapList.num_maptypes; j++)
		{
			if ( i >= s_mapList.type_offset[j][MAPTYPE_MASTER_BEGIN] &&
				i < s_mapList.type_offset[j][MAPTYPE_MASTER_END])
			{
				StartServer_AddMapType(mappic, j);
				break;
			}
			if ( i >= s_mapList.type_offset[j][MAPTYPE_CUSTOM_BEGIN] &&
				i < s_mapList.type_offset[j][MAPTYPE_CUSTOM_END])
			{
				StartServer_AddMapType(mappic, j);
				break;;
			}
		}
	}

	// sort the icons, so they're always displayed in the same order
	for (i = 0; i < mappic->num_types - 1; i++)
		for (j = i + 1; j < mappic->num_types; j++)
		{
			if (mappic->type[j] < mappic->type[i])
			{
				tmp = mappic->type[j];
				mappic->type[j] = mappic->type[i];
				mappic->type[i] = tmp;
			}
		}

	// set the picture name, cache it
	strcpy(mappic->mapname, mapname);
	trap_R_RegisterShaderNoMip( va("levelshots/%s.tga", mapname) );
}



/*
=================
StartServer_InitMapPicture
=================
*/
void StartServer_InitMapPicture(mappic_t* mappic, const char* mapname)
{
	int i;
	const char* info;
	int nummaps;

	if (!mapname || !*mapname)
		return;

	// verify existance of map
	nummaps = UI_GetNumArenas();

	for( i = 0; i < nummaps; i++ ) {
		info = UI_GetArenaInfoByNumber( i );

		if (!info) {
			continue;
		}

		if (Q_stricmp(Info_ValueForKey( info, "map"), mapname))
			continue;

		StartServer_InitMapPictureFromIndex(mappic, i);
		return;
	}
}





/*
=================
StartServer_DrawMapPicture
=================
*/
void StartServer_DrawMapPicture(int x, int y, int w, int h, mappic_t* mappic, vec4_t color)
{
	qhandle_t hpic;
	int i;
	int mapbits;
	int colx, coly;
	int icons;

	// load and draw map picture
	hpic = trap_R_RegisterShaderNoMip( va("levelshots/%s.tga", mappic->mapname) );
	if (!hpic)
		hpic = trap_R_RegisterShaderNoMip( MAP_ERRORPIC );

	if (color)
		trap_R_SetColor(color);

	UI_DrawHandlePic(x, y, w, h, hpic);

	if (color)
	{
		faded_color[0] = color[0];
		faded_color[1] = color[1];
		faded_color[2] = color[2];
		faded_color[3] = color[3] * 0.75;	// slight transparency
	}
	else {
		faded_color[0] = 1.0;
		faded_color[1] = 1.0;
		faded_color[2] = 1.0;
		faded_color[3] = 0.75;	// slight transparency
	}
	shadow_color[3] = faded_color[3];

	//
	// overlay the icons
	//

	colx = 0;
	coly = 0;

	// built in game types first
	icons = uie_mapicons.integer;
	if (icons == MAPICONS_ALL)
	{
		for (i = 0; i < NUM_GAMETYPES; i++)
		{
			mapbits = 1 << i;

			if (!(mapbits & mappic->gamebits))
				continue;

			hpic = trap_R_RegisterShaderNoMip( maptype_icon[i] );
			if (!hpic)
				continue;

			colx += MAPTYPE_ICONX;
			if (colx > w)
			{
				colx = MAPTYPE_ICONX;
				coly += MAPTYPE_ICONY;
			}

			trap_R_SetColor(shadow_color);
			UI_DrawHandlePic(x + w - colx + 1, y + coly + 1, MAPTYPE_ICONX, MAPTYPE_ICONY, hpic);

			trap_R_SetColor(faded_color);
			UI_DrawHandlePic(x + w - colx, y + coly, MAPTYPE_ICONX, MAPTYPE_ICONY, hpic);
		}
	}

	if (icons != MAPICONS_NONE)
	{
		for (i = 0; i < mappic->num_types; i++)
		{
			hpic = trap_R_RegisterShaderNoMip( s_mapList.mapGraphic[mappic->type[i]] );
			if (!hpic)
				continue;

			colx += MAPTYPE_ICONX;
			if (colx > w)
			{
				colx = MAPTYPE_ICONX;
				coly += MAPTYPE_ICONY;
			}

			trap_R_SetColor(shadow_color);
			UI_DrawHandlePic(x + w - colx + 1, y + coly + 1, MAPTYPE_ICONX, MAPTYPE_ICONY, hpic);

			trap_R_SetColor(faded_color);
			UI_DrawHandlePic(x + w - colx, y + coly, MAPTYPE_ICONX, MAPTYPE_ICONY, hpic);
		}
	}

	trap_R_SetColor(NULL);
}




/*
=================
StartServer_LoadBotlessMaps

Creates a special group of maps that don't have
.aas bot support files
=================
*/
void StartServer_LoadBotlessMaps(void)
{
	int i, nummaps;
	const char* info;
	char mapname[MAPNAME_BUFFER];
	fileHandle_t file;
	int len, index;

	StartServer_CreateMapType("NoBots","uie_icons/noammo");
	index = s_mapList.num_maptypes - 1;
	s_mapList.noBotsIndex = index;
	 
	s_mapList.type_offset[index][MAPTYPE_MASTER_BEGIN] = s_mapList.num_maps;

	nummaps = UI_GetNumArenas();
	for (i = 0; i < nummaps; i++)
	{
		info = UI_GetArenaInfoByNumber(i);
		if (!info)
			continue;

		Q_strncpyz(mapname, Info_ValueForKey( info, "map"), MAPNAME_BUFFER) ;
		len = trap_FS_FOpenFile(va("maps/%s.aas", mapname), &file, FS_READ);

		if (len >= 0) {
			trap_FS_FCloseFile(file);
			continue;
		}

		// no .aas file, so we can't play bots on this map
		strcpy(s_mapList.mapName[s_mapList.num_maps], mapname);
		s_mapList.num_maps++;
	}

	// update last map index
	s_mapList.type_offset[index][MAPTYPE_MASTER_END] = s_mapList.num_maps;
}




/*
=================
UI_LoadMapTypeInfo
=================
*/
void UI_LoadMapTypeInfo(void)
{
	int i;
	int arenas;

	if (maplist_loaded)
		return;

	// reset data
	memset(&s_mapList, 0, sizeof(mapTypeList_t));

	s_mapList.num_maptypes = 0;
	s_mapList.num_maps = 0;
	s_mapList.noBotsIndex = -1;

	// load all the maps that don't have bot route files
	StartServer_LoadBotlessMaps();

	// load data files
	StartServer_LoadCustomMapData("mapdata.txt", qfalse);
	StartServer_LoadCustomMapData("usermaps.txt", qtrue);

	// update map selection list so we can put custom maps on screen
	for (i = 0; i < s_mapList.num_maptypes; i++)
	{
		mapfilter_items[MAPFILTER_MAX + i] = s_mapList.mapTypeName[i];
		randommaptype_items[MAP_RND_MAX + i] = s_mapList.mapTypeName[i];
	}

	maplist_loaded = qtrue;
}




/*
=================
StartServer_NumCustomMapTypes
=================
*/
int StartServer_NumCustomMapTypes(void)
{
	if (!maplist_loaded)
		UI_LoadMapTypeInfo();

	return s_mapList.num_maptypes;
}


/*
=================
StartServer_MapIconFromType
=================
*/
const char* StartServer_MapIconFromType(int gametype, qboolean isCustomMap)
{
	if (!maplist_loaded)
		UI_LoadMapTypeInfo();

	if (isCustomMap)
	{
		if (gametype >= s_mapList.num_maptypes || gametype < 0)
			return NULL;

		return s_mapList.mapGraphic[gametype];
	}
	else
	{
		return UIE_DefaultIconFromGameType(gametype);
	}
}




/*
=================
StartServer_IsCustomMapType
=================
*/
qboolean StartServer_IsCustomMapType(const char* mapname, int type)
{
	int i;
	int end;

	if (!maplist_loaded)
		UI_LoadMapTypeInfo();

	if (type >= s_mapList.num_maptypes || type < 0)
		return qfalse;

	end = s_mapList.type_offset[type][MAPTYPE_MASTER_END];
	for (i = s_mapList.type_offset[type][MAPTYPE_MASTER_BEGIN]; i < end; i++)
		if (!Q_stricmp(mapname, s_mapList.mapName[i]))
			return qtrue;

	end = s_mapList.type_offset[type][MAPTYPE_CUSTOM_END];
	for (i = s_mapList.type_offset[type][MAPTYPE_CUSTOM_BEGIN]; i < end; i++)
		if (!Q_stricmp(mapname, s_mapList.mapName[i]))
			return qtrue;

	return qfalse;
}
