/*
=============================================================================

START SERVER MENU *****

=============================================================================
*/

#include "ui_local.h"
#include "ui_startserver_q3.h"

//
// misc data
//

// scratchpad colours, [3] may be any value
vec4_t pulsecolor = {1.0, 1.0, 1.0, 0.0};
vec4_t fading_red = {1.0, 0.0, 0.0, 0.0};

/*
=================
StartServer_CheckFightReady
=================
*/
qboolean StartServer_CheckFightReady(commoncontrols_t* c)
{
	if (StartServer_CanFight()) {
		c->fight.generic.flags &= ~QMF_GRAYED;
		return qtrue;
	}

	c->fight.generic.flags |= QMF_GRAYED;
	return qfalse;
}

/*
=================
StartServer_BackgroundDraw
=================
*/
void StartServer_BackgroundDraw(qboolean excluded)
{
	static vec4_t dim = { 1.0, 1.0, 1.0, 0.5 };

	int y, w, h;

	// draw the frames
	y = 64 + LINE_HEIGHT;
	w = 172;
	h = 480 - 2 * y;

	trap_R_SetColor( dim );
	UI_DrawNamedPic( 32, y, w, h, FRAME_LEFT);
	UI_DrawNamedPic( 640 - 32 - w, y, w, h, FRAME_RIGHT );

	if (excluded)
	{
		dim[3] = 0.25;
		UI_DrawNamedPic( 320 - 256, 240 - 64 - 32, 512, 256, FRAME_EXCLUDED);
	}

	trap_R_SetColor( NULL );
}

/*
=================
StartServer_SelectionDraw
=================
*/
void StartServer_SelectionDraw(void* self )
{
	float	x;
	float	y;
	float	w;
	float	h;
	float offset;
	qhandle_t shader;
	menubitmap_s* b;

	b = (menubitmap_s*)self;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;

	// used to refresh shader
	if (b->generic.name && !b->shader)
	{
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if (!b->shader && b->errorpic)
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
	}

	if (b->focuspic && !b->focusshader)
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );

	if (b->generic.flags & QMF_HIGHLIGHT)
		shader = b->focusshader;
	else
		shader = b->shader;

	if (b->generic.flags & QMF_GRAYED) {
		if (shader) {
			trap_R_SetColor( colorMdGrey );
			UI_DrawHandlePic( x, y, w, h, shader );
			trap_R_SetColor( NULL );
		}
	}
	else
	{
		if ((b->generic.flags & QMF_PULSE) || (b->generic.flags & QMF_PULSEIFFOCUS) && (UI_CursorInRect(x, y, w, h)))
		{
			offset = 3*sin(uis.realtime/PULSE_DIVISOR);
			UI_DrawHandlePic( x - offset, y - offset, w + 2*offset, h + 2*offset, shader );
		}
		else
		{
			UI_DrawHandlePic( x, y, w, h, shader );
		}
	}
}

/*
=================
StartServer_CommonControls_Cache
=================
*/
void StartServer_CommonControls_Cache( void )
{
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_FIGHT0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_FIGHT1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_SERVER0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_SERVER1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_WEAPONS0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_WEAPONS1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_MAPS0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_MAPS1 );
	trap_R_RegisterShaderNoMip( GAMESERVER_BOTS0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_BOTS1 );

	trap_R_RegisterShaderNoMip( FRAME_LEFT );
	trap_R_RegisterShaderNoMip( FRAME_RIGHT );
	trap_R_RegisterShaderNoMip( FRAME_EXCLUDED );
}

/*
=================
StartServer_CommonControls_Init
=================
*/
void StartServer_CommonControls_Init(
	menuframework_s* menuptr,
	commoncontrols_t* common,
	CtrlCallback_t callback,
	int ctrlpage)
{
	int scale;
	int height;

	StartServer_CommonControls_Cache();

	common->back.generic.type     = MTYPE_BITMAP;
	common->back.generic.name     = GAMESERVER_BACK0;
	common->back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->back.generic.callback = callback;
	common->back.generic.id	    = ID_SERVERCOMMON_BACK;
	common->back.generic.x		= 0 - uis.wideoffset;
	common->back.generic.y		= 480-64;
	common->back.width  		    = 128;
	common->back.height  		    = 64;
	common->back.focuspic         = GAMESERVER_BACK1;

	common->fight.generic.type     = MTYPE_BITMAP;
	common->fight.generic.name     = GAMESERVER_FIGHT0;
	common->fight.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->fight.generic.callback = callback;
	common->fight.generic.id	    = ID_SERVERCOMMON_FIGHT;
	common->fight.generic.x		= 640-128 + uis.wideoffset;
	common->fight.generic.y		= 480-64;
	common->fight.width  		    = 128;
	common->fight.height  		    = 64;
	common->fight.focuspic         = GAMESERVER_FIGHT1;

	common->maps.generic.type     = MTYPE_BITMAP;
	common->maps.generic.name     = GAMESERVER_MAPS0;
	common->maps.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->maps.generic.callback = callback;
	common->maps.generic.id	    = ID_SERVERCOMMON_MAPS;
	common->maps.generic.x		= 0 - uis.wideoffset;
	common->maps.generic.y		= 0;
	common->maps.width  		    = 128;
	common->maps.height  		    = 64;
	common->maps.focuspic         = GAMESERVER_MAPS1;

	common->bots.generic.type     = MTYPE_BITMAP;
	common->bots.generic.name     = GAMESERVER_BOTS0;
	common->bots.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->bots.generic.callback = callback;
	common->bots.generic.id	    = ID_SERVERCOMMON_BOTS;
	common->bots.generic.x		= 128 - uis.wideoffset;
	common->bots.generic.y		= 0;
	common->bots.width  		    = 128;
	common->bots.height  		    = 64;
	common->bots.focuspic         = GAMESERVER_BOTS1;

	common->items.generic.type     = MTYPE_BITMAP;
	common->items.generic.name     = GAMESERVER_ITEMS0;
	common->items.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->items.generic.callback = callback;
	common->items.generic.id	    = ID_SERVERCOMMON_ITEMS;
	common->items.generic.x		= 256 - uis.wideoffset;
	common->items.generic.y		= 0;
	common->items.width  		    = 128;
	common->items.height  		    = 64;
	common->items.focuspic         = GAMESERVER_ITEMS1;

	common->server.generic.type     = MTYPE_BITMAP;
	common->server.generic.name     = GAMESERVER_SERVER0;
	common->server.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->server.generic.callback = callback;
	common->server.generic.id	    = ID_SERVERCOMMON_SERVER;
	common->server.generic.x		= 384 - uis.wideoffset;
	common->server.generic.y		= 0;
	common->server.width  		    = 128;
	common->server.height  		    = 64;
	common->server.focuspic         = GAMESERVER_SERVER1;
	
	common->weapon.generic.type     = MTYPE_BITMAP;
	common->weapon.generic.name     = GAMESERVER_WEAPONS0;
	common->weapon.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	common->weapon.generic.callback = callback;
	common->weapon.generic.id	    = ID_SERVERCOMMON_WEAPON;
	common->weapon.generic.x		= 512 - uis.wideoffset;
	common->weapon.generic.y		= 0;
	common->weapon.width  		    = 128;
	common->weapon.height  		    = 64;
	common->weapon.focuspic         = GAMESERVER_WEAPONS1;

	scale = UI_ProportionalSizeScale( UI_BIGFONT, 0 );
	height = (64 - PROP_HEIGHT * scale)/2;

	common->maptext.generic.type			= MTYPE_PTEXT;
	common->maptext.generic.x			= 64 - uis.wideoffset;
	common->maptext.generic.y			= height;
	common->maptext.generic.flags			= QMF_INACTIVE;
	common->maptext.color  				= color_white;
	common->maptext.style  				= UI_CENTER|UI_BIGFONT;

	common->bottext.generic.type			= MTYPE_PTEXT;
	common->bottext.generic.x			= 64 + 128 - uis.wideoffset;
	common->bottext.generic.y			= height;
	common->bottext.generic.flags			= QMF_INACTIVE;
	common->bottext.color  				= color_white;
	common->bottext.style  				= UI_CENTER|UI_BIGFONT;

	common->itemtext.generic.type			= MTYPE_PTEXT;
	common->itemtext.generic.x			= 64 + 256 - uis.wideoffset;
	common->itemtext.generic.y			= height;
	common->itemtext.generic.flags			= QMF_INACTIVE;
	common->itemtext.color  				= color_white;
	common->itemtext.style  				= UI_CENTER|UI_BIGFONT;

	common->servertext.generic.type			= MTYPE_PTEXT;
	common->servertext.generic.x			= 64 + 384 - uis.wideoffset;
	common->servertext.generic.y			= height;
	common->servertext.generic.flags			= QMF_INACTIVE;
	common->servertext.color  				= color_white;
	common->servertext.style  				= UI_CENTER|UI_BIGFONT;
	
	common->weapontext.generic.type			= MTYPE_PTEXT;
	common->weapontext.generic.x			= 64 + 512 - uis.wideoffset;
	common->weapontext.generic.y			= height;
	common->weapontext.generic.flags			= QMF_INACTIVE;
	common->weapontext.color  				= color_white;
	common->weapontext.style  				= UI_CENTER|UI_BIGFONT;
	
	if(cl_language.integer == 0){
	common->weapontext.string  				= "Weapon";
	common->servertext.string  				= "Server";
	common->itemtext.string  				= "Items";
	common->bottext.string  				= "Bots";
	common->maptext.string  				= "Maps";
	}
	if(cl_language.integer == 1){
	common->weapontext.string  				= "Оружие";
	common->servertext.string  				= "Сервер";
	common->itemtext.string  				= "Предметы";
	common->bottext.string  				= "Боты";
	common->maptext.string  				= "Карты";
	}


	// register controls
	Menu_AddItem( menuptr, &common->back);
	Menu_AddItem( menuptr, &common->fight);

	switch (ctrlpage)
	{
		case COMMONCTRL_BOTS:
			Menu_AddItem( menuptr, &common->maps);
			Menu_AddItem( menuptr, &common->items);
			Menu_AddItem( menuptr, &common->server);
			Menu_AddItem( menuptr, &common->weapon);
			Menu_AddItem( menuptr, &common->bottext);
			break;
		case COMMONCTRL_MAPS:
			Menu_AddItem( menuptr, &common->bots);
			Menu_AddItem( menuptr, &common->items);
			Menu_AddItem( menuptr, &common->server);
			Menu_AddItem( menuptr, &common->weapon);
			Menu_AddItem( menuptr, &common->maptext);
			break;
		case COMMONCTRL_SERVER:
			Menu_AddItem( menuptr, &common->bots);
			Menu_AddItem( menuptr, &common->maps);
			Menu_AddItem( menuptr, &common->items);
			Menu_AddItem( menuptr, &common->weapon);
			Menu_AddItem( menuptr, &common->servertext);
			break;
		case COMMONCTRL_ITEMS:
			Menu_AddItem( menuptr, &common->bots);
			Menu_AddItem( menuptr, &common->maps);
			Menu_AddItem( menuptr, &common->server);
			Menu_AddItem( menuptr, &common->weapon);
			Menu_AddItem( menuptr, &common->itemtext);
			break;
		case COMMONCTRL_WEAPON:
			Menu_AddItem( menuptr, &common->bots);
			Menu_AddItem( menuptr, &common->maps);
			Menu_AddItem( menuptr, &common->items);
			Menu_AddItem( menuptr, &common->server);
			Menu_AddItem( menuptr, &common->weapontext);
			break;
	}
}

/*
=================
StartServer_Cache
=================
*/
void StartServer_Cache( void )
{
	StartServer_CommonControls_Cache();
	StartServer_ServerPage_Cache();
	StartServer_MapPage_Cache();
	StartServer_BotPage_Cache();
}
