/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

static float CG_DrawRallyPowerups( float y );

#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*s & 255];
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*s & 255];
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[*s & 255];
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

/*
==============================================================
CG_DrawField - Draws large numbers for status bar and powerups
==============================================================
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	// center x, move y to bottom.
	x += (1.0f-cg_statusScale.value)*l*CHAR_WIDTH*0.5f;
	y += (1.0f-cg_statusScale.value)*CHAR_HEIGHT;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH*cg_statusScale.value, CHAR_HEIGHT*cg_statusScale.value, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH*cg_statusScale.value;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK

/*
==============
CG_Draw3DModel
==============
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
==================
CG_DrawHealthModel
==================
*/
void CG_DrawHealthModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, qhandle_t model2, vec3_t origin, vec3_t angles, float yaw2 ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );

	if ( model2 ) {
		ent.hModel = model2;
		angles[YAW] = yaw2;
		AnglesToAxis( angles, ent.axis );
		trap_R_AddRefEntityToScene( &ent );
	}

	trap_R_RenderScene( &refdef );
}

/*
==================================================================
CG_DrawFlagModel - Used for both the status bar and the scoreboard
==================================================================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
====================
CG_DrawStatusBarHead
====================
*/
#ifndef MISSIONPACK
#if 0 // ZTM: Not used by Q3Rally
static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

}
#endif
#endif // MISSIONPACK

/*
====================
CG_DrawStatusBarFlag
====================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	int iconSize = ICON_SIZE*cg_statusScale.value;
	CG_DrawFlagModel( x+(1.0f-cg_statusScale.value)*ICON_SIZE*0.5f, 480 - iconSize, iconSize, iconSize, team, qfalse );
}
#endif // MISSIONPACK

/*
=====================
CG_DrawTeamBackground
=====================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	}
// Q3Rally Code Start
	else if ( team == TEAM_GREEN ) {
		hcolor[0] = 0;
		hcolor[1] = 1;
		hcolor[2] = 0;
	}
	else if ( team == TEAM_YELLOW ) {
		hcolor[0] = 1;
		hcolor[1] = 1;
		hcolor[2] = 0;
	}
// END
	else {
		return;
	}
	trap_R_SetColor( hcolor );
    CG_SetScreenPlacement(PLACE_STRETCH, CG_GetScreenVerticalPlacement());
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
    CG_PopScreenPlacement();
	trap_R_SetColor( NULL );
}

/*
===============
CG_DrawSigilHUD
===============
*/

void CG_DrawSigilHUD( void ) {

                      int i, x=440, y=0;
                      for (i=0; i<MAX_SIGILS; i++) {
                      switch ( cgs.sigil[i] )
                      
                {

                case SIGIL_ISRED:
                    CG_DrawPic( x, y, 18, 18, cgs.media.redsigilShader );
                    break;
        
                case SIGIL_ISBLUE:
                    CG_DrawPic( x, y, 18, 18, cgs.media.bluesigilShader );
                    break;
                  
                case SIGIL_ISGREEN:
                    CG_DrawPic( x, y, 18, 18, cgs.media.greensigilShader );
                    break;
                    
                case SIGIL_ISYELLOW:
                    CG_DrawPic( x, y, 18, 18, cgs.media.yellowsigilShader );
                    break;
                
                case SIGIL_ISWHITE:
                    CG_DrawPic( x, y, 18, 18, cgs.media.sigilShader );
                    break;
                    
                case SIGIL_NONE:
                    break;
                }
                
              x+= 19;
            }
}

/*
================
CG_DrawStatusBar
================
*/
#ifndef MISSIONPACK
#if 0
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
    float		scale, iconSize;

	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

	scale = Com_Clamp( 0.1f, 2, cg_statusScale.value);

	iconSize = scale * ICON_SIZE;

	// draw the team background
	CG_DrawTeamBackground( 0, 480 - 60*scale, 640, 60*scale, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 480-iconSize, iconSize, iconSize,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

		if ( cg_drawStatusHead.integer == 2 ) {
		origin[0] = 60;
		origin[1] = 0;
		origin[2] = -5;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 4096.0;
		CG_DrawHealthModel( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 480-iconSize, iconSize, iconSize,
					   cg_items[ 6 /*item_health_large*/].models[0], 0, cg_items[ 6 /*item_health_large*/].models[1], origin, angles, 0 );

		// if we didn't draw a 3D icon, draw a 2D icon for health
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 480 - iconSize, iconSize, iconSize, cg_items[6/*item_health_large*/].icon );
		}
	}
	else if ( cg_drawStatusHead.integer == 1 )
		CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
        CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 480 - iconSize, iconSize, iconSize,
					   cgs.media.armorModel, 0, origin, angles );
	}
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING

				&& (cg.predictedPlayerState.weaponTime & NORMAL_WEAPON_TIME_MASK) > 100 ) {

				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (0, 432, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 480 - iconSize, iconSize, iconSize, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawField ( 185, 432, 3, value);
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 480 - iconSize, iconSize, iconSize, cgs.media.armorIcon );
		}

	}
}
#endif


/*
=====================
CG_DrawRallyStatusBar
=====================
*/
static void CG_DrawRallyStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
	qhandle_t	healthModel;
	vec4_t		bg_color;
	int			weapon, i;

	if ( !cg_drawStatus.integer ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

// draw the dtf sigils

    if ( cgs.gametype == GT_DOMINATION )

    CG_DrawSigilHUD();

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	switch (cgs.clientinfo[cg.snap->ps.clientNum].team){
	case TEAM_RED:
		Vector4Copy(colorRed, bg_color);
		bg_color[3] = 0.5;
		break;

	case TEAM_BLUE:
		Vector4Copy(colorBlue, bg_color);
		bg_color[3] = 0.5;
		break;

	case TEAM_GREEN:
		Vector4Copy(colorGreen, bg_color);
		bg_color[3] = 0.5;
		break;

	case TEAM_YELLOW:
		Vector4Copy(colorYellow, bg_color);
		bg_color[3] = 0.5;
		break;
	
	default:
		Vector4Copy(bgColor, bg_color);
	}

	// draw ammo background
	value = ps->ammo[cent->currentState.weapon];
	if ( cent->currentState.weapon && value > -1 )
		CG_FillRect( 20, 476 - 30, 90, 24, bg_color );

	// health background
	   CG_FillRect( 210, 476 - 30, 90, 24, bg_color );

	// armor background
	if ( ps->stats[ STAT_ARMOR ] )

        CG_FillRect( 305, 476 - 30, 90, 24, bg_color );

	// rearammo background
	weapon = 0;
	for (i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++){
		if (ps->stats[STAT_WEAPONS] & ( 1 << i )){
			if (ps->ammo[ i ]){
				weapon = i;
				break;
			}
		}
	}

	if ( weapon )
		CG_FillRect( 115, 476 - 30, 90, 24, bg_color );

	// draw any 3D icons now, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 270 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( 26, 476 - 28, 19, 19,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	// Replace head with '+' sign.
	VectorClear( angles );
	if (ps->stats[STAT_HEALTH] < 25)
		healthModel = trap_R_RegisterModel( "models/powerups/health/large_cross.md3" );
	else if (ps->stats[STAT_HEALTH] < 50)
		healthModel = trap_R_RegisterModel( "models/powerups/health/medium_cross.md3" );
	else
		healthModel = trap_R_RegisterModel( "models/powerups/health/small_cross.md3" );

	if ( healthModel ) {
		origin[0] = 68;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 270 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( 212, 476 - 26, 19, 19,
					   healthModel, 0, origin, angles );
	}

	CG_DrawRallyPowerups( 476 );

	if (cg.predictedPlayerState.powerups[PW_REDFLAG])
		CG_DrawStatusBarFlag( 495, TEAM_RED);
	else if (cg.predictedPlayerState.powerups[PW_BLUEFLAG])
		CG_DrawStatusBarFlag( 495, TEAM_BLUE);
	else if (cg_entities[cg.snap->ps.clientNum].finishRaceTime &&
		cg_entities[cg.snap->ps.clientNum].currentPosition == 1){

		CG_DrawStatusBarFlag( 495, 15);
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 45;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = 270 * sin( cg.time / 1000.0 );
//		CG_Draw3DModel( 196, 476 - 64, 26, 26,
        CG_Draw3DModel( 311, 476 - 28, 19, 19,
					   cgs.media.armorModel, 0, origin, angles );
	}


	//
	// Draw numbers and 2D icons now
	//

	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& (cg.predictedPlayerState.weaponTime & NORMAL_WEAPON_TIME_MASK) > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (42, 476 - 28, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( 10, 476 - 28, 26, 26, icon );
				}
			}
		}
	}


	//
	// rear ammo
	//
	if ( weapon && cg_weapons[ weapon ].weaponModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 270 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( 120, 476 - 28, 19, 19,
					   cg_weapons[ weapon ].weaponModel, 0, origin, angles );
	}

	if ( weapon ) {
		value = ps->ammo[weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_REARFIRING
				&& ((cg.predictedPlayerState.weaponTime & REAR_WEAPON_TIME_MASK) >> 16) > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (158, 476 - 28, 2, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ weapon ].weaponIcon;
				if ( icon ) {
					CG_DrawPic( 6, 480 - 67, 26, 26, icon );
				}
			}
		}
	}


	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawField ( 232, 476 - 28, 3, value);
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
//		CG_DrawField ( 242, 476 - 64, 3, value);
        CG_DrawField ( 327, 476 - 28, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 196, 476 - 64, 26, 26, cgs.media.armorIcon );
		}
	}
}
#endif

/*
================
CG_DrawPowerups
================

#ifndef MISSIONPACK
#if 0 // ZTM: Not used by Q3Rally
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}

		// ZOID--don't draw if the power up has unlimited time
		// This is true of the CTF flags
		if ( ps->powerups[ i ] == INT_MAX ) {
			continue;
		}

		t = ps->powerups[ i ] - cg.time;
		if ( t <= 0 ) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

    if (item) {

		  color = 1;

		  y -= ICON_SIZE;

		  trap_R_SetColor( colors[color] );
		  CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

		  t = ps->powerups[ sorted[i] ];
		  if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			  trap_R_SetColor( NULL );
		  } else {
			  vec4_t	modulate;

			  f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
			  f -= (int)f;
			  modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			  trap_R_SetColor( modulate );
		  }

		  if ( cg.powerupActive == sorted[i] && 
			  cg.time - cg.powerupTime < PULSE_TIME ) {
			  f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
			  size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		  } else {
			  size = ICON_SIZE;
		  }

		  CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2, 
			  size, size, trap_R_RegisterShader( item->icon ) );
    }
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif

*/

/*
====================
CG_DrawRallyPowerups
====================
*/
static float CG_DrawRallyPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		color;
	float	size;
	float	f;
    float	x;
	vec3_t	origin;
	vec3_t	angles;
	vec4_t	bg_color;

	static float colors[2][4] = { 
		{ 0.2f, 1.0f, 0.2f, 1.0f } ,
		{ 1.0f, 0.2f, 0.2f, 1.0f }
	};

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	switch (cgs.clientinfo[cg.snap->ps.clientNum].team){
	case TEAM_RED:
		Vector4Copy(colorRed, bg_color);
		bg_color[3] = 0.5f;
		break;

	case TEAM_BLUE:
		Vector4Copy(colorBlue, bg_color);
		bg_color[3] = 0.5f;
		break;

	case TEAM_GREEN:
		Vector4Copy(colorGreen, bg_color);
		bg_color[3] = 0.5f;
		break;

	case TEAM_YELLOW:
		Vector4Copy(colorYellow, bg_color);
		bg_color[3] = 0.5f;
		break;
	
	default:
		Vector4Copy(bgColor, bg_color);
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;

		if (i == PW_TURBO && ps->powerups[ i ] < 0)
			t = -ps->powerups[ i ];

		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000 ) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] <= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
    
    x = 402;  // Start X position for horizontal layout
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		color = 1;

		CG_FillRect( x, y - 30, 90, 24, bg_color );

		trap_R_SetColor( colors[color] );
		CG_DrawField( x + 22 + CHAR_WIDTH, y - 28, 2, sortedTime[ i ] / 1000 );

		t = ps->powerups[ sorted[i] ];
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			trap_R_SetColor( NULL );
		}
		// dont blink turbo when not on
		else if (item->giTag == PW_TURBO && ps->powerups[ item->giTag ] < 0){
			trap_R_SetColor( NULL );
		}
		else {
			vec4_t	modulate;

			f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
			f -= (int)f;
			modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			trap_R_SetColor( modulate );
		}

		if ( cg.powerupActive == sorted[i] && 
			cg.time - cg.powerupTime < PULSE_TIME ) {
			f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
			size = 19 * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		} else {
			size = 19;
		}

		// Setup 3D model positioning and rotation
		origin[0] = 68;
		origin[1] = 0;
		origin[2] = 0;
		VectorClear( angles );
		angles[YAW] = 270 * sin( cg.time / 1000.0 );
		
		CG_Draw3DModel( x + 6, y - 27, size, size, trap_R_RegisterModel( item->world_model[0] ), 0, origin, angles );
		x += 96;  // Move to next horizontal position (90 width + 6 spacing)
	}
	trap_R_SetColor( NULL );
    return y;
}


/*
===========================================================================================

 UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker
================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
// Q3Rally Code
	//vec3_t		angles;
// Q3Rally Code END
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	if ( !cgs.clientinfo[clientNum].infoValid ) {
		cg.attackerTime = 0;
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

// Q3Rally Code ( removed function )
//	angles[PITCH] = 0;
//	angles[YAW] = 180;
//	angles[ROLL] = 0;
//	CG_DrawHead( 640 - size, y, size, size, clientNum, angles );
// Q3Rally Code END

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
===============
CG_DrawSnapshot
===============
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==========
CG_DrawFPS
==========
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString( 621 - w, 355 + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
============
CG_DrawTimer
============
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
==================
CG_DrawTeamOverlay
==================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


/*
=================
CG_DrawUpperRight
=================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame)
{
	float	y;

	y = 0;
    
    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);

	y = CG_DrawUpperRightHUD( y );

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		CG_DrawAttacker( y );
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
================================================
CG_DrawScores - Draw the small two score display
================================================
*/
#ifndef MISSIONPACK

float CG_DrawScores( float x, float y ) {

	const char	*s;
	int			s1, s2, score;
	int			s3, s4, w;
	int			v;
	vec4_t		color;
	float		y1;
	gitem_t		*item;

	if ( !cg_drawScores.integer ) {
		return y;
	}

	s1 = cgs.scores1;
	s2 = cgs.scores2;
	s3 = cgs.scores3;
	s4 = cgs.scores4;

	y1 = y + 18;


	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM ) {

		CG_FillRect( x - 80, y, 96, 18, bgColor );

        if (cgs.gametype >= GT_TEAM){
			// draw yellow
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 0.0f;
			color[3] = 0.33f;
			s = va( "%2i", s4 );
			w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
			x -= w;
			CG_FillRect( x+1, y+1, w-2, 16, color );
			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_YELLOW ) {
				CG_DrawPic( x+1, y+1, w-2, 16, cgs.media.selectShader );
			}
			CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

			// draw green
			color[0] = 0.0f;
			color[1] = 1.0f;
			color[2] = 0.0f;
			color[3] = 0.33f;
			s = va( "%2i", s3 );
			w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
			x -= w;
			CG_FillRect( x+1, y+1, w-2, 16, color );
			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_GREEN ) {
				CG_DrawPic( x+1, y+1, w-2, 16, cgs.media.selectShader );
			}
			CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);
		}

		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
		x -= w;
        
		CG_FillRect( x+1, y+1, w-2, 16, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x+1, y+1, w-2, 16, cgs.media.selectShader );

		}

		CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item) {

				y1 = y + TINYCHAR_HEIGHT + 8;		
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {

					CG_DrawPic( x+1, y1+1, w-2, 16, cgs.media.blueFlagShader[cgs.blueflag] );

				}
			}
		}

		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
		x -= w;

		CG_FillRect( x+1, y+1, w-2, 16, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {

			CG_DrawPic( x+1, y+1, w-2, 16, cgs.media.selectShader );

		}

		CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item) {

				y1 = y + TINYCHAR_HEIGHT + 8;

				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {


				}
			}
		}

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.flagStatus >= 0 && cgs.flagStatus <= 3 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.flagShader[cgs.flagStatus] );
				}
			}
		}
#endif
		if ( cgs.gametype >= GT_CTF ) {
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}

		if ( v && !isRallyRace()) {

			s = va( "%2i", v );
			w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
			x -= w;

			CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

		}

	} else {
		qboolean	spectator;

		CG_FillRect( x - 80, y, 96, 18, bgColor );

		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
			w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;

				CG_FillRect( x+1, y+1, w-2, 16, color );
				CG_DrawPic( x+1, y+1, w-2, 16, cgs.media.selectShader );

			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;

				CG_FillRect( x+1, y+1, w-2, 16, color );

			}	

			CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
			w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;

				CG_FillRect( x+1, y+1, w-2, 16, color );
				CG_DrawPic( x+1, y+1, w-2, 16, cgs.media.selectShader );

			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;

				CG_FillRect( x+1, y+1, w-2, 16, color );

			}	

			CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			w = CG_DrawStrlen( s ) * TINYCHAR_WIDTH + 8;
			x -= w;

			CG_DrawTinyDigitalString( x + 4, y+4, s, 1.0F);

		}

	}

	return y1;

}
#endif // MISSIONPACK

/*
=================
CG_DrawLowerRight
=================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 470;

	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

	if ( isRaceObserver( cg.snap->ps.clientNum ) )
		return;

	y = CG_DrawLowerRightHUD( y );

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

}
#endif // MISSIONPACK

/*
=================
CG_DrawPickupItem
=================
*/
#ifndef MISSIONPACK
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;
    float	iconSize, charWidth, charHeight;

	if ( cg_drawPickups.value <= 0 ) {
		return y;
	}

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	iconSize = ICON_SIZE*cg_drawPickups.value;
	charWidth = BIGCHAR_WIDTH*cg_drawPickups.value;
	charHeight = BIGCHAR_HEIGHT*cg_drawPickups.value;

	y -= iconSize;

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 8, y, iconSize, iconSize, cg_items[ value ].icon );
			CG_DrawStringExt( iconSize + 16, y + (iconSize/2 - charHeight/2), bg_itemlist[ value ].pickup_name, fadeColor, qfalse, qtrue, charWidth, charHeight, 0 );
			trap_R_SetColor( NULL );
		}
	}
	
	return y;
}
#endif // MISSIONPACK

/*
================
CG_DrawLowerLeft
================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480;
    
    CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);

	if (y > 404)
		y = 404;

	y = CG_DrawLowerLeftHUD( y );

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	} 


	CG_DrawPickupItem( y );
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	int h;
	int i;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

    CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}
#endif // MISSIONPACK

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
static void CG_DrawHoldableItem( void ) { 
	int		value;

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}

}
#endif // MISSIONPACK

#ifdef MISSIONPACK
/*
========================
CG_DrawPersistantPowerup
========================
*/
#if 0 // sos001208 - DEAD
static void CG_DrawPersistantPowerup( void ) { 
	int		value;

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);

	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif
#endif // MISSIONPACK


/*
=============
CG_DrawReward
=============
*/
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 112;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 112;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}


/*
=========
LAGOMETER
=========
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
========================================================================================
CG_AddLagometerFrameInfo - Adds the current interpolate / extrapolate bar for this frame
========================================================================================
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
=======================================================
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
=======================================================
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==================================================================================
CG_DrawDisconnect - Should we draw something differnet for long lag vs no packets?
==================================================================================
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
================
CG_DrawLagometer
================
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

	//
	// draw the graph
	//
#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
#ifdef MISSIONPACK
	int h;
#endif
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
			cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );

		y += cg.centerPrintCharWidth * 1.5;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================

static void CG_DrawCrosshair(void)
{
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	CG_DrawPic( ((SCREEN_WIDTH-w)*0.5f)+x, ((SCREEN_HEIGHT-h)*0.5f)+y, w, h, hShader );

	trap_R_SetColor( NULL );
}
*/
/*
==================
CG_DrawCrosshair3D
==================

static void CG_DrawCrosshair3D(void)
{
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	
	VectorCopy(trace.endpos, ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}

*/

/*
=========================
CG_ScanForCrosshairEntity
=========================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );
#endif
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
    CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
// Q3Rally Code Start - removed gametype
/*
	if ( cgs.gametype == GT_TOURNAMENT ) {
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	}
	else
*/
// Q3Rally Code END
	if ( cgs.gametype >= GT_TEAM ) {
		CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the START menu to play", 1.0F);
	}
}

/*
===========
CG_DrawVote
===========
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
#ifdef MISSIONPACK
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
#else
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_DrawSmallString( 0, 58, s, 1.0F );
#endif
}

/*
===============
CG_DrawTeamVote
===============
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}


static qboolean CG_DrawScoreboard( void ) {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
	} else {
		if ( !CG_FadeColor( cg.scoreFadeTime, FADE_TIME ) ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
	}

	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else
	return CG_DrawOldScoreboard();
#endif
}

/*
===================
CG_DrawIntermission
===================
*/
static void CG_DrawIntermission( void ) {
//	int key;
#ifdef MISSIONPACK
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		return;
	}
#endif
	cg.scoreFadeTime = cg.time;

// Q3Rally Code Start
	cg.scoreBoardShowing = CG_DrawHUD();

	if (!cg.scoreBoardShowing)
// Q3Rally Code END
		cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=============
CG_DrawFollow
=============
*/
static qboolean CG_DrawFollow( void ) {
	float		x;
	vec4_t		color;
	const char	*name;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
    
    CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);
    
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;


	CG_DrawBigString( 320 - 9 * BIGCHAR_WIDTH/2, 24, "following", 1.0F );

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( name ) );

	CG_DrawStringExt( x, 40, name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

	return qtrue;
}



/*
==================
CG_DrawAmmoWarning
==================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

// Q3Rally Code Start
	if (isRallyNonDMRace() || cgs.gametype == GT_DERBY){
		return;
	}
// Q3Rally Code END

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 98, s, 1.0F);
}


#ifdef MISSIONPACK
/*
==================
CG_DrawProxWarning
==================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	int			w;
  static int proxTime;
  int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

  if (proxTime == 0) {
    proxTime = cg.time;
  }

  proxTick = 10 - ((cg.time - proxTime) / 1000);

  if (proxTick > 0 && proxTick <= 5) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
#endif

//========================
//CG_DrawSigilLocationInfo
//========================

void CG_DrawSigilLocationInfo( vec3_t origin, vec3_t target, qhandle_t shader, vec4_t color )    {

          int x = 320, y = 240;
          int w = 320, h = 240;
          float angle, distance;
          vec3_t temp, angles;
          VectorSubtract(origin, target, temp);
          distance=VectorLength(temp);
          VectorNormalize(temp);
          vectoangles(temp,angles);
          
          angles[YAW]=AngleSubtract(cg.refdefViewAngles[YAW],angles[YAW]);
          angle=(angles[YAW] + 180.0f)/360.0f;
          angle -=0.25;
          angle *= (2*M_PI);
          w=sqrt((w*w)+(h*h));
          x +=cos(angle)*w;
          y +=sin(angle)*w;
          
          if (x<15)
                  x=15;
          else {
        
          if (x>605)
                  x=605;
        }
        if (y<20)
                y=20;
        else 
          {
            if (y>440)
                    y=440;
        }
        CG_DrawPic( x, y, 20, 20, shader );
        CG_DrawStringExt( x-50, y+20, va("%10.2f",distance/100.0), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
    }

//=====================
//CG_DrawSigilLocations
//=====================

static void CG_DrawSigilLocations( void ) {
          snapshot_t *snap;
          int i;
          vec3_t origin, end;
          int redSigil, blueSigil, greenSigil, yellowSigil, whiteSigil;

          if ( cgs.gametype != GT_DOMINATION)
            return;
          if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR )
            return; 
      
          if ( cg.nextSnap && (!cg.nextFrameTeleport && !cg.thisFrameTeleport))
            
            snap = cg.nextSnap;
          else
            snap = cg.snap;
      
      VectorCopy(cg.snap->ps.origin,origin);
      redSigil = ITEM_INDEX( BG_FindItemForPowerup( PW_SIGILRED ) );
      blueSigil = ITEM_INDEX( BG_FindItemForPowerup( PW_SIGILBLUE ) );
      greenSigil = ITEM_INDEX( BG_FindItemForPowerup( PW_SIGILGREEN ) );
      yellowSigil = ITEM_INDEX( BG_FindItemForPowerup( PW_SIGILYELLOW ) );
      whiteSigil = ITEM_INDEX( BG_FindItemForPowerup( PW_SIGILWHITE ) );
      
      for ( i = 0; i < snap->numEntities; i++ )
    {
    
    centity_t *target = &cg_entities[snap->entities[i].number];
    if (target->currentState.eType != ET_ITEM)
    continue;

    if ( target->currentState.modelindex != redSigil && target->currentState.modelindex != blueSigil && target->currentState.modelindex != greenSigil && target->currentState.modelindex != yellowSigil && target->currentState.modelindex != whiteSigil )
    continue;
    
    VectorCopy(target->lerpOrigin,end);
    
    if (target->currentState.modelindex == redSigil)
        CG_DrawSigilLocationInfo(origin, end, cgs.media.redsigilShader, colorRed);
    
    else if (target->currentState.modelindex == blueSigil)
        CG_DrawSigilLocationInfo(origin, end, cgs.media.bluesigilShader, colorBlue);
    
    else if (target->currentState.modelindex == greenSigil)
        CG_DrawSigilLocationInfo(origin, end, cgs.media.greensigilShader, colorGreen);
        
    else if (target->currentState.modelindex == yellowSigil)
        CG_DrawSigilLocationInfo(origin, end, cgs.media.yellowsigilShader, colorYellow);
        
    else if (target->currentState.modelindex == whiteSigil)
        CG_DrawSigilLocationInfo(origin, end, cgs.media.sigilShader, colorWhite);
    }
  }
//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif
/*
=========
CG_Draw2D
=========
*/
static void CG_Draw2D(stereoFrame_t stereoFrame)
{
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR 
		|| isRaceObserver( cg.snap->ps.clientNum ) ) {

		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)

		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown

		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

#ifdef MISSIONPACK
			if ( cg_drawStatus.integer ) {
            
            CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
            
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#else

			CG_DrawRallyStatusBar();

#endif
      
			CG_DrawAmmoWarning();

#ifdef MISSIONPACK
			CG_DrawProxWarning();
#endif      
			if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

#ifndef MISSIONPACK
			CG_DrawHoldableItem();
#else
	
#endif
			CG_DrawReward();
		}
	}

	if ( cgs.gametype >= GT_TEAM ) {
#ifndef MISSIONPACK
		CG_DrawTeamInfo();
#endif
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();
	
	if (cg_sigilLocator.integer == 1)
      CG_DrawSigilLocations();
      
#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight(stereoFrame);
	}
#else
	CG_DrawUpperRight(stereoFrame);
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	CG_DrawFollow();

	cg.scoreBoardShowing = CG_DrawHUD();


	// don't draw center string if scoreboard is up

	if (!cg.scoreBoardShowing)

		cg.scoreBoardShowing = CG_DrawScoreboard();

	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();

		CG_DrawRaceCountDown();

	}
}


/*
========================================================================
CG_DrawActive - Perform all drawing needed to completely fill the screen
========================================================================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
//		CG_DrawCrosshair3D();
	// apply earthquake effect
	CG_Earthquake();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

// Q3Rally Code Start
	if ( !cg.scoreBoardShowing )
		CG_DrawRearviewMirror( 170, 10, 300, 75);
	
	CG_DrawMMap( 0, 10, 160, 120); //TBB draw minimap function
		
// Q3Rally Code END

	// draw status bar and other floating elements
 	CG_Draw2D(stereoView);
}



