/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "../ui/ui_shared.h"

//----(SA) added to make it easier to raise/lower our statsubar by only changing one thing
#define STATUSBARHEIGHT 452
//----(SA) end

extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

////////////////////////
////////////////////////
////// new hud stuff
///////////////////////
///////////////////////

int CG_Text_Width( const char *text, int font, float scale, int limit ) {
	int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *fnt = &cgDC.Assets.textFont;

	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= cg_smallFont.value ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( scale > cg_bigFont.value ) {
			fnt = &cgDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &cgDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &cgDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &cgDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;
	out = 0;
	if ( text ) {
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = &fnt->glyphs[*s & 255];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

int CG_Text_Height( const char *text, int font, float scale, int limit ) {
	int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *fnt = &cgDC.Assets.textFont;

	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= cg_smallFont.value ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( scale > cg_bigFont.value ) {
			fnt = &cgDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &cgDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &cgDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &cgDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;
	max = 0;
	if ( text ) {
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = &fnt->glyphs[*s & 255];
				if ( max < glyph->height ) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void CG_Text_PaintChar( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader ) {
	float w, h;
	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust, int limit, int style ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *fnt = &cgDC.Assets.textFont;

	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= cg_smallFont.value ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( scale > cg_bigFont.value ) {
			fnt = &cgDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &cgDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &cgDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &cgDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;

	color[3] *= cg_hudAlpha.value;  // (SA) adjust for cg_hudalpha

	if ( text ) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			glyph = &fnt->glyphs[*s & 255];
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar( x + ofs, y - yadj + ofs,
									   glyph->imageWidth,
									   glyph->imageHeight,
									   useScale,
									   glyph->s,
									   glyph->t,
									   glyph->s2,
									   glyph->t2,
									   glyph->glyph );
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar( x, y - yadj,
								   glyph->imageWidth,
								   glyph->imageHeight,
								   useScale,
								   glyph->s,
								   glyph->t,
								   glyph->s2,
								   glyph->t2,
								   glyph->glyph );
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += ( glyph->xSkip * useScale ) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}











static void CG_DrawField( int x, int y, int width, int value ) {
	char num[16], *ptr;
	int l;
	int frame;

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

	Com_sprintf( num, sizeof( num ), "%i", value );
	l = strlen( num );
	if ( l > width ) {
		l = width;
	}
	x += 2 + CHAR_WIDTH * ( width - l );

	ptr = num;
	while ( *ptr && l )
	{
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else {
			frame = *ptr - '0';
		}

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t refdef;
	refEntity_t ent;

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
	ent.renderfx = RF_NOSHADOW;     // no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	refdef.rdflags |= RDF_DRAWSKYBOX;
	if ( !cg_skybox.integer ) {
		refdef.rdflags &= ~RDF_DRAWSKYBOX;
	}

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t cm;
	clientInfo_t    *ci;
	float len;
	vec3_t origin;
	vec3_t mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;    // len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->modelInfo->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles );
//	} else if ( cg_drawIcons.integer ) {
//		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team ) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;

	VectorClear( angles );

	cm = cgs.media.redFlagModel;

	// offset the origin y and z to center the flag
	trap_R_ModelBounds( cm, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the flag nearly fills the box
	// assume heads are taller than wide
	len = 0.5 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;    // len / tan( fov/2 )

	angles[YAW] = 60 * sin( cg.time / 2000.0 );;

	CG_Draw3DModel( x, y, w, h,
					team == TEAM_RED ? cgs.media.redFlagModel : cgs.media.blueFlagModel,
					0, origin, angles );
}


/*
==============
CG_DrawKeyModel
==============
*/
void CG_DrawKeyModel( int keynum, float x, float y, float w, float h, int fadetime ) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;

	VectorClear( angles );

	cm = cg_items[keynum].models[0];

	// offset the origin y and z to center the model
	trap_R_ModelBounds( cm, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

//	len = 0.5 * ( maxs[2] - mins[2] );
	len = 0.75 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;    // len / tan( fov/2 )

	angles[YAW] = 30 * sin( cg.time / 2000.0 );;

	CG_Draw3DModel( x, y, w, h, cg_items[keynum].models[0], 0, origin, angles );
}

/*
================
CG_DrawStatusBarHead

================
*/
static void CG_DrawStatusBarHead( float x ) {
	vec3_t angles;
	float size, stretch;
	float frac;

	VectorClear( angles );

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)( cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.viewDamage[cg.damageIndex].damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.viewDamage[cg.damageIndex].damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom() * M_PI );
		cg.headEndPitch = 5 * cos( crandom() * M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom() * M_PI );
			cg.headEndPitch = 5 * cos( crandom() * M_PI );
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

	CG_DrawHead( x, 480 - size, size, size,
			cg.snap->ps.clientNum, angles );
}

/*
==============
CG_DrawStatusBarKeys
IT_KEY (this makes this routine easier to find in files...) (SA)
==============
*/
static void CG_DrawStatusBarKeys() {
	int i;
	float y = 0;    // start height is
	gitem_t *gi;
	int itemnum;
//	int		fadetime = 0;
	float   *fadeColor;


//----(SA)	added
	if ( cg.showItems ) {
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.itemFadeTime, 1000 );
	}

	if ( !fadeColor ) {
		return;
	}


	// (SA) just don't draw this stuff for now.  It's got fog issues I need to clean up

//	return;



	for ( i = 1; i < KEY_NUM_KEYS; i++ )
	{
		gi = BG_FindItemForKey( i, &itemnum );
		// if i've got the key...

		if ( cg.snap->ps.stats[STAT_KEYS] & ( 1 << gi->giTag ) ) {
			y += ICON_SIZE + 5;
			CG_DrawKeyModel( itemnum, 640 - ( 1.5 * ICON_SIZE ), y, ICON_SIZE, ICON_SIZE, cg.time + fadeColor[0] * 1000 );
		}
	}
}

/*
================
CG_DrawStatusBarFlag

================
*/
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team );
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team ) {
	vec4_t hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

//////////////////////
////// end new hud stuff
//////////////////////





// JOSEPH 4-25-00
/*
================
CG_DrawStatusBar

================
*/
static void CG_DrawStatusBar( void ) {
	int color;
	centity_t   *cent;
	playerState_t   *ps;
	int value, inclip;
	vec4_t hcolor;
	vec3_t angles;
//	vec3_t		origin;


	static float colors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1, 0.69, 0, 1.0 },        // normal
		{ 1.0, 0.2, 0.2, 1.0 },     // low health
		{0.5, 0.5, 0.5, 1},         // weapon firing
		{ 1, 1, 1, 1 }
	};                              // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
		hcolor[3] = 0.33;
		trap_R_SetColor( hcolor );
		CG_DrawPic( 0, 420, 640, 60, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );
	} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
		hcolor[3] = 0.33;
		trap_R_SetColor( hcolor );
		CG_DrawPic( 0, 420, 640, 60, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );
	}

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized

	//----(SA) further change... we don't need to draw the ammo 3d model do we?
/*
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );;
//----(SA) Wolf statusbar change
//			CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, STATUSBARHEIGHT -20, ICON_SIZE, ICON_SIZE,
//					cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
//----(SA) end
	}
*/

	
	if ( cg_drawStatusHead.integer ) {
		CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );
	}

	CG_DrawStatusBarKeys();

	if ( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if ( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	}

	//----(SA) further change... we don't need to draw the armor do we?
/*
	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
//----(SA) Wolf statusbar change
//			CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, STATUSBARHEIGHT -20, ICON_SIZE, ICON_SIZE,
//					cgs.media.armorModel, 0, origin, angles );
//----(SA) end
	}
*/

	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		qhandle_t icon;
		float scale,halfScale;
		float wideOffset;

		if ( cg_fixedAspect.integer == 2 ) {
			CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
		}

		value = ps->ammo[BG_FindAmmoForWeapon( cent->currentState.weapon )];
		inclip = ps->ammoclip[BG_FindClipForWeapon( cent->currentState.weapon )];

		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weapon == WP_KNIFE || cg.predictedPlayerState.weapon == WP_DAGGER ) {
				color = 3; 
			} else if ( ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING || cg.predictedPlayerState.weaponstate == WEAPON_FIRINGALT )
					&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;  // dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;  // green
				} else {
					color = 1;  // red
				}
			}
			trap_R_SetColor( colors[color] );

			// pulsing grenade icon to help the player 'count' in their head
			if ( ps->grenadeTimeLeft ) {
				if ( ps->weapon == WP_DYNAMITE ) {

				} else {
					if ( ( ( cg.grenLastTime ) % 1000 ) < ( ( ps->grenadeTimeLeft ) % 1000 ) ) {
						switch ( ps->grenadeTimeLeft / 1000 ) {
						case 3:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
							break;
						case 2:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound3, CHAN_LOCAL_SOUND );
							break;
						case 1:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound2, CHAN_LOCAL_SOUND );
							break;
						case 0:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound1, CHAN_LOCAL_SOUND );
							break;
						}
					}
				}

				scale = (float)( ( ps->grenadeTimeLeft ) % 1000 ) / 100.0f;
				halfScale = scale * 0.5f;

				cg.grenLastTime = ps->grenadeTimeLeft;
			} else {
				scale = halfScale = 0;
			}


			switch ( cg.predictedPlayerState.weapon ) {
			case WP_THOMPSON:
			case WP_MP40:
			case WP_STEN:
			case WP_MAUSER:
			case WP_GARAND:
			case WP_VENOM:
			case WP_TESLA:
			case WP_PANZERFAUST:
			case WP_FLAMETHROWER:
				wideOffset = -38;
				break;
			default:
				wideOffset = 0;
				break;
			}

			// don't draw ammo value for knife
			if ( cg.predictedPlayerState.weapon != WP_DAGGER ) {
				if ( cgs.dmflags & DF_NO_WEAPRELOAD ) {
					CG_DrawBigString2( ( 580 - 23 + 35 ) + wideOffset, STATUSBARHEIGHT, va( "%d.", value ), cg_hudAlpha.value );
				} else if ( value ) {
					CG_DrawBigString2( ( 580 - 23 + 35 ) + wideOffset, STATUSBARHEIGHT, va( "%d/%d", inclip, value ), cg_hudAlpha.value );
				} else {
					CG_DrawBigString2( ( 580 - 23 + 35 ) + wideOffset, STATUSBARHEIGHT, va( "%d", inclip ), cg_hudAlpha.value );
				}
			}
            
			icon = cg_weapons[ cg.predictedPlayerState.weapon ].weaponIcon[0];
			if ( icon ) {
				CG_DrawPic( ( ( 530 + 68 ) - halfScale ) + wideOffset,  ( 446 - 10 ) - halfScale, ( 38 + scale ) - wideOffset, 38 + scale, icon );
			}
			

			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer ) {
				qhandle_t icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH * 3 + TEXT_ICON_SPACE, STATUSBARHEIGHT, ICON_SIZE, ICON_SIZE, icon );
				}
			}
			
		}
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );       // white
	} else if ( value > 25 ) {
		trap_R_SetColor( colors[0] );   // green
	} else if ( value > 0 ) {
		color = ( cg.time >> 8 ) & 1; // flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );   // red
	}

	// stretch the health up when taking damage
//----(SA) Wolf statusbar change
//	CG_DrawField ( 185, STATUSBARHEIGHT, 3, value);
	{
		char printme[16];
		Com_sprintf( printme, sizeof( printme ), "%d", value );
		//CG_DrawBigString( 185, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
		CG_DrawBigString2( 16 + 23 + 43, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
	}
//----(SA) end
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if ( value > 0 ) {
		trap_R_SetColor( colors[0] );
//----(SA) Wolf statusbar change
//		CG_DrawField (370, STATUSBARHEIGHT, 3, value);
		{
			char printme[16];
			Com_sprintf( printme, sizeof( printme ), "%d", value );
			//CG_DrawBigString( 370, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
			CG_DrawBigString2( 200, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
		}
//----(SA) end
		trap_R_SetColor( NULL );
//----(SA) Wolf statusbar change
//		CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, STATUSBARHEIGHT, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
//----(SA) end
	}
}
// END JOSEPH

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

#define UPPERRIGHT_X 500

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int t;
	float size;
	vec3_t angles;
	const char  *info;
	const char  *name;
	int clientNum;

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

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( UPPERRIGHT_X - size, y, size, size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( UPPERRIGHT_X - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH ), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char        *s;
	int w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
			cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES  4
static float CG_DrawFPS( float y ) {
	char        *s;
	int w;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frameTime;

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

		CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char        *s;
	int w;
	int mins, seconds, tens;
	int msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

// set in CG_ParseTeamInfo
int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

#define TEAM_OVERLAY_MAXNAME_WIDTH  16
#define TEAM_OVERLAY_MAXLOCATION_WIDTH  20

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame) {
	float y;

	y = 0;

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);
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
// (SA) disabling drawattacker for the time being
	if ( cg_oldWolfUI.integer ) {
		if ( cg_drawAttacker.integer ) {
			CG_DrawAttacker( y );
		}
	}
//----(SA)	end
}

/*
=====================
1NTERRUPTOR

CG_DrawScriptLabel

fasty but hacky: we use player's playerstate fields because it can be directly passed by game to cgame
=====================
*/
#define SCRIPTLABEL_PULSEAMP		0.7f		//1 - mean 0...1; 0.7 mean 0.15...0.85

static void CG_DrawScriptLabel() {
	char *s1, *s2, *s3;
	int w1, w2, w3;
	int mins, seconds, tens;
	int msec;
	float a;
	int offset = 0;
	printLabel_t *lbl = &cg.predictedPlayerState.scriptAccumLabel;

	if (!lbl->state) {
		return;
	}

	msec = lbl->value;

	if (lbl->flags & SCRIPT_ACCUMPRINT_ACCUM) {
		if (lbl->flags & SCRIPT_ACCUMPRINT_TIMER) {
			seconds = msec / 1000;
			mins = seconds / 60;
			seconds -= mins * 60;
			tens = seconds / 10;
			seconds -= tens * 10;

			s1 = va("%i:%i%i", mins, tens, seconds);
		}
		else {
			s1 = va("%d", msec);
		}
	}
	else {
		s1 = va("");
	}
	w1 = CG_DrawStrlen(s1) * BIGCHAR_WIDTH;

	if (lbl->flags & SCRIPT_ACCUMPRINT_STRING) {
		s2 = va("%s", lbl->label);
	}
	else {
		s2 = va("");
	}
	w2 = CG_DrawStrlen(s2) * BIGCHAR_WIDTH;

	//nothing to draw
	if (!w1 && !w2) {
		return;
	}

	if (lbl->flags & SCRIPT_ACCUMPRINT_PULSE) {
		a = 0.5f + 0.5f * sinf((float)cg.time * 0.005f) / (1.f / SCRIPTLABEL_PULSEAMP);
	}
	else {
		a = 1.f;
	}

	if (lbl->flags & SCRIPT_ACCUMPRINT_INLINE) {
		s3 = va("%s %s", s2, s1);
		w3 = CG_DrawStrlen(s3) * BIGCHAR_WIDTH;
		CG_DrawBigString(lbl->pos[0] - (int)roundf((float)w3 * 0.5f), lbl->pos[1], s3, a);
	}
	else {
		if (w2) {
			offset = BIGCHAR_HEIGHT;
			CG_DrawBigString(lbl->pos[0] - (int)roundf((float)w2 * 0.5f), lbl->pos[1], s2, a);
		}
		if (w1) {
			CG_DrawBigString(lbl->pos[0] - (int)roundf((float)w1 * 0.5f), lbl->pos[1] + offset, s1, a);
		}
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
static float CG_DrawScores( float y ) {

	y -=  BIGCHAR_HEIGHT + 8;

	return y - 8;
}

/*
================
CG_DrawPowerups
================
*/
static float CG_DrawPowerups( float y ) {
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t   *ps;
	int t;
	gitem_t *item;
	int x;
	int color;
	float size;
	float f;
	static float colors[2][4] = {
		{ 0.2, 1.0, 0.2, 1.0 }, { 1.0, 0.2, 0.2, 1.0 }
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
					sorted[k + 1] = sorted[k];
					sortedTime[k + 1] = sortedTime[k];
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

//		continue;   // (SA) FIXME: TEMP: as I'm getting powerup business going

		item = BG_FindItemForPowerup( sorted[i] );

		color = 1;

		y -= ICON_SIZE;

		trap_R_SetColor( colors[color] );
		CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

		t = ps->powerups[ sorted[i] ];
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			trap_R_SetColor( NULL );
		} else {
			vec4_t modulate;

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
	trap_R_SetColor( NULL );

	return y;
}


/*
=====================
CG_DrawLowerRight

=====================
*/
static void CG_DrawLowerRight( void ) {
	float y;

	y = 480 - ICON_SIZE;

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	y = CG_DrawScores( y );
	CG_DrawPowerups( y );
}

//===========================================================================================

//----(SA)	modified
//----(SA)	modified

/*
===================
CG_DrawCheckpointString
===================
*/
static void CG_DrawCheckpointString ( void ) {
	float    *color;

	color = CG_FadeColor( cg.checkpointTime, CHECKPOINT_PASSED_TIME );

	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	if ( cg_drawCheckpoint.integer == 1 ) 
	{
	CG_DrawStringExt2( -25, 100, CG_translateString( "checkpointsaved" ), color, qfalse, qtrue, 10, 10, 0 );
	}

}

/*
===================
CG_DrawGameSavedString
===================
*/
static void CG_DrawGameSavedString ( void ) {
	float    *color;

	color = CG_FadeColor( cg.gameSavedTime, GAME_SAVED_TIME );

	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	CG_DrawStringExt2( -25, 115, CG_translateString( "gamesaved" ), color, qfalse, qtrue, 10, 10, 0 );

}


/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int value;
	float   *fadeColor;
	char pickupText[256];
	float color[4];
	int w = 0;

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );

			//----(SA)	so we don't pick up all sorts of items and have it print "0 <itemname>"
			if ( bg_itemlist[ value ].giType == IT_AMMO || bg_itemlist[ value ].giType == IT_HEALTH || bg_itemlist[value].giType == IT_POWERUP ) {
				if ( bg_itemlist[ value ].world_model[2] ) {   // this is a multi-stage item
					// FIXME: print the correct amount for multi-stage
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", cgs.itemPrintNames[ value ] );
				} else {
					// removing print of the hardcoded values for pickups since there is a dropammo command with an arbitrary amount.
					//if ( bg_itemlist[ value ].gameskillnumber[cg_gameSkill.integer] > 1 ) {
					//	Com_sprintf( pickupText, sizeof( pickupText ), "%i  %s", bg_itemlist[ value ].gameskillnumber[cg_gameSkill.integer], cgs.itemPrintNames[ value ] );
					//} else {
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", cgs.itemPrintNames[value] );
					//}
				}
			} else {
				Com_sprintf( pickupText, sizeof( pickupText ), "%s", cgs.itemPrintNames[value] );
			}

			color[0] = color[1] = color[2] = 1.0;
			color[3] = fadeColor[0];
			w = CG_DrawStrlen( pickupText ) * 10;
#ifdef LOCALISATION
			CG_DrawStringExt2( 640 - ( w / 2 ), 375, CG_TranslateString( pickupText ), color, qfalse, qtrue, 10, 10, 0 );
#else
			CG_DrawStringExt2( 640 - ( w / 2 ), 375, pickupText, color, qfalse, qtrue, 10, 10, 0 );
#endif

			trap_R_SetColor( NULL );
		}
	}
}
//----(SA)	end

/*
===================
CG_DrawHoldableItem
===================
*/
void CG_DrawHoldableItem_old( void ) {
	int value;
	gitem_t *item;

	if ( !cg.holdableSelect ) {
		return;
	}

	item    = BG_FindItemForHoldable( cg.holdableSelect );

	if ( !item ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
	}

	value   = cg.predictedPlayerState.holdable[cg.holdableSelect];

	if ( value ) {

		trap_R_SetColor( NULL );

		CG_RegisterItemVisuals( item - bg_itemlist );

		if ( cg.holdableSelect == HI_WINE ) {
			if ( value > 3 ) {
				value = 3;  // 3 stages to icon, just draw full if beyond 'full'
			}
			CG_DrawPic( 606, 366, 24, 24, cg_items[item - bg_itemlist].icons[2 - ( value - 1 )] );
		} else {
			CG_DrawPic( 606, 366, 24, 24, cg_items[item - bg_itemlist].icons[0] );

		}

		// draw the selection box so it's not just floating in space
		CG_DrawPic( 606 - 4, 366 - 4, 32, 32, cgs.media.selectShader );
	}
}

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) {
	float   *color;
	int i;
	float x, y;

	if ( !cg_drawRewards.integer ) {
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );
	y = 56;
	x = 320 - cg.rewardCount * ICON_SIZE / 2;
	for ( i = 0 ; i < cg.rewardCount ; i++ ) {
		CG_DrawPic( x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader );
		x += ICON_SIZE;
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES     128


typedef struct {
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float x, y;
	int cmdNum;
	usercmd_t cmd;
	const char      *s;
	int w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		 || cmd.serverTime > cg.time ) { // special check for map_restart
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 100, s, 1.0F );

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	x = 640 - 52;
	y = 480 - 240;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/2d/net.tga" ) );
}


#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
//	if(0) {
		CG_DrawDisconnect();
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	//
	// draw the graph
	//
	x = 640 - 52;
	y = 480 - 240;

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
		i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_BLUE )] );
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
		i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_GREEN )] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_RED )] );
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
	char   *s;

//----(SA)	added translation lookup
	Q_strncpyz( cg.centerPrint, CG_translateString( (char*)str ), sizeof( cg.centerPrint ) );
//----(SA)	end


	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.centerPrintLines++;
		}
		if ( !Q_strncmp( s, "\\n", 1 ) ) {
			cg.centerPrintLines++;
			s++;
		}
		s++;
	}
}

/*
==============
CG_BonusCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_BonusCenterPrint( const char *str, int y, int charWidth ) {
	char   *s;
    int len;
//----(SA)	added translation lookup
	Q_strncpyz( cg.centerPrint, CG_bonusString( (char*)str ), sizeof( cg.centerPrint ) );
//----(SA)	end


	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.centerPrintLines++;
		}
		if ( !Q_strncmp( s, "\\n", 1 ) ) {
			cg.centerPrintLines++;
			s++;
		}
		s++;
	}

	len = CG_DrawStrlen(cg.centerPrint);
	cg.centerPrintTime = cg.time + len * 200;
}

/*
==============
CG_SubtitlePrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_SubtitlePrint( const char *str, int y, int charWidth ) {
	char   *s;
	int len;


    // Translate the input string
    const char* translated = CG_translateTextString(str);

    // Check if the translated string is "IGNORED_SUBTITLE", indicating an ignored subtitle
    if (strcmp(translated, "IGNORED_SUBTITLE") == 0) {
        // If it is, return immediately to prevent the subtitle from being displayed
        return;
    }

    // Copy the translated string to cg.subtitlePrint
    Q_strncpyz(cg.subtitlePrint, translated, sizeof(cg.subtitlePrint));


	
	cg.subtitlePrintY = y;
	cg.subtitlePrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.subtitlePrintLines = 1;
	s = cg.subtitlePrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.subtitlePrintLines++;
		}
		if ( !Q_strncmp( s, "\\n", 1 ) ) {
			cg.subtitlePrintLines++;
			s++;
		}
		s++;
	}
    // Calculate the number of characters in the message
    len = CG_DrawStrlen(cg.subtitlePrint);
	// Calculate the display time based on an average reading speed of 17 characters per second
    int displayTime = (len / 17.0) * 1000; // Convert to milliseconds

	// Ensure the display time is at least a certain minimum value to prevent very short messages from disappearing too quickly
    int minDisplayTime = 2000; // 2 seconds
    if (displayTime < minDisplayTime) {
       displayTime = minDisplayTime;
    }

	// Set the time at which the message should disappear
    cg.subtitlePrintTime = cg.time + displayTime;
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char    *start;
	int l;
	int x, y, w;
	float   *color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' || !Q_strncmp( &start[l], "\\n", 1 ) ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue, cg.centerPrintCharWidth, (int)( cg.centerPrintCharWidth * 1.5 ), 0 );

		y += cg.centerPrintCharWidth * 2;

		while ( *start && ( *start != '\n' ) ) {
			if ( !Q_strncmp( start, "\\n", 1 ) ) {
				start++;
				break;
			}
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
===================
CG_DrawSubtitleString
===================
*/
static void CG_DrawSubtitleString( void ) {
	char    *start;
	int l;
	int x, y, w;
	float   *color;

	if ( !cg.subtitlePrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.subtitlePrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = cg.subtitlePrint;

	y = cg.subtitlePrintY - cg.subtitlePrintLines * BIGCHAR_HEIGHT / 2;

while ( 1 ) {
    char linebuffer[1024]; // Buffer size

    for ( l = 0; l < 50; l++ ) { // Line length limit
        if ( !start[l] || start[l] == '\n' || !Q_strncmp( &start[l], "\\n", 1 ) ) {
            break;
        }
        linebuffer[l] = start[l];
        if (l >= 49 && start[l+1] != ' ' && start[l+1] != '\0') { // Check if the next character is a space or end of string
            while(l > 0 && linebuffer[l] != ' ') { // Move back to the last space
                l--;
            }
            break;
        }
    }
    linebuffer[l] = 0;

    w = cg.subtitlePrintCharWidth * CG_DrawStrlen( linebuffer );

    x = ( SCREEN_WIDTH - w ) / 2;


    if ( cg_subtitleShadow.integer ) {
       CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue, cg.subtitlePrintCharWidth, (int)( cg.subtitlePrintCharWidth * 1.5 ), 0 );
	} else {
       CG_DrawStringExt( x, y, linebuffer, color, qfalse, qfalse, cg.subtitlePrintCharWidth, (int)( cg.subtitlePrintCharWidth * 1.5 ), 0 );
	}

    y += cg.subtitlePrintCharWidth * 2;

    // Skip processed characters and newline characters
    start += l;
    while ( *start && ( *start == '\n' || !Q_strncmp( start, "\\n", 1 ) ) ) {
        start++;
    }
    if ( !*start ) {
        break;
    }
}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIRS

================================================================================
*/


/*
==============
CG_DrawWeapReticle
==============
*/
static void CG_DrawWeapReticle( void ) {
	int weap;
	vec4_t color = {0, 0, 0, 1};
	vec4_t snoopercolor = {0.7, .8, 0.7, 0};    // greenish
	float snooperBrightness;
	float x = 80, y, w = 240, h = 240;
	float mask = 0, lb = 0;

	CG_AdjustFrom640( &x, &y, &w, &h );

	weap = cg.weaponSelect;

	if ( weap == WP_SNIPERRIFLE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		if ( cgs.media.reticleShaderSimpleQ ) {
			if ( cg_fixedAspect.integer ) {
				trap_R_DrawStretchPic( x, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );         // tl
				trap_R_DrawStretchPic( x + w, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );     // tr
				trap_R_DrawStretchPic( x, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			} else {
				trap_R_DrawStretchPic( x, 0, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );      // tl
				trap_R_DrawStretchPic( x + w, 0, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );  // tr
				trap_R_DrawStretchPic( x, h, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );      // bl
				trap_R_DrawStretchPic( x + w, h, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ );  // br
			}
		}

		if ( cg_drawCrosshairReticle.integer ) {
			CG_FillRect( 80, 239, 480, 1, color );	// horiz
			CG_FillRect( 319, 0, 1, 480, color );   // vert
		}

		// hairs
		CG_FillRect( 84, 239, 177, 2, color );   // left
		CG_FillRect( 320, 242, 1, 58, color );   // center top
		CG_FillRect( 319, 300, 2, 178, color );  // center bot
		CG_FillRect( 380, 239, 177, 2, color );  // right

	} else if ( weap == WP_DELISLESCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		if ( cgs.media.reticleShaderSimpleQ ) {
			if ( cg_fixedAspect.integer ) {
				trap_R_DrawStretchPic( x, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );         // tl
				trap_R_DrawStretchPic( x + w, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );     // tr
				trap_R_DrawStretchPic( x, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			} else {
				trap_R_DrawStretchPic( x, 0, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );      // tl
				trap_R_DrawStretchPic( x + w, 0, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );  // tr
				trap_R_DrawStretchPic( x, h, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );      // bl
				trap_R_DrawStretchPic( x + w, h, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ );  // br
			}
		}

		if ( cg_drawCrosshairReticle.integer ) {
			CG_FillRect( 80, 239, 480, 1, color );	// horiz
			CG_FillRect( 319, 0, 1, 480, color );   // vert
		}

		// hairs
		CG_FillRect( 84, 239, 177, 2, color );   // left
		CG_FillRect( 320, 242, 1, 58, color );   // center top
		CG_FillRect( 319, 300, 2, 178, color );  // center bot
		CG_FillRect( 380, 239, 177, 2, color );  // right

	} else if ( weap == WP_M1941SCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		if ( cgs.media.reticleShaderSimpleQ ) {
			if ( cg_fixedAspect.integer ) {
				trap_R_DrawStretchPic( x, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );         // tl
				trap_R_DrawStretchPic( x + w, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );     // tr
				trap_R_DrawStretchPic( x, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			} else {
				trap_R_DrawStretchPic( x, 0, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );      // tl
				trap_R_DrawStretchPic( x + w, 0, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );  // tr
				trap_R_DrawStretchPic( x, h, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );      // bl
				trap_R_DrawStretchPic( x + w, h, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ );  // br
			}
		}

		if ( cg_drawCrosshairReticle.integer ) {
			CG_FillRect( 80, 239, 480, 1, color );	// horiz
			CG_FillRect( 319, 0, 1, 480, color );   // vert
		}

		// hairs
		CG_FillRect( 84, 239, 177, 2, color );   // left
		CG_FillRect( 320, 242, 1, 58, color );   // center top
		CG_FillRect( 319, 300, 2, 178, color );  // center bot
		CG_FillRect( 380, 239, 177, 2, color );  // right

	}  else if ( weap == WP_SNOOPERSCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

//----(SA)	added
		// DM didn't like how bright it gets
		snooperBrightness = Com_Clamp( 0.0f, 1.0f, cg_reticleBrightness.value );
		snoopercolor[0] *= snooperBrightness;
		snoopercolor[1] *= snooperBrightness;
		snoopercolor[2] *= snooperBrightness;
		trap_R_SetColor( snoopercolor );
//----(SA)	end

		if ( cgs.media.snooperShaderSimple ) {
			CG_DrawPic( 80, 0, 480, 480, cgs.media.snooperShaderSimple );
		}

		// hairs

		CG_FillRect( 310, 120, 20, 1, color );   //					-----
		CG_FillRect( 300, 160, 40, 1, color );   //				-------------
		CG_FillRect( 310, 200, 20, 1, color );   //					-----

		CG_FillRect( 140, 239, 360, 1, color );  // horiz ---------------------------

		CG_FillRect( 310, 280, 20, 1, color );   //					-----
		CG_FillRect( 300, 320, 40, 1, color );   //				-------------
		CG_FillRect( 310, 360, 20, 1, color );   //					-----



		CG_FillRect( 400, 220, 1, 40, color );   // l

		CG_FillRect( 319, 60, 1, 360, color );   // vert

		CG_FillRect( 240, 220, 1, 40, color );   // r
	} else if ( weap == WP_FG42SCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		if ( cgs.media.reticleShaderSimpleQ ) {
			if ( cg_fixedAspect.integer ) {
				trap_R_DrawStretchPic( x, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );         // tl
				trap_R_DrawStretchPic( x + w, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );     // tr
				trap_R_DrawStretchPic( x, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			} else {
				trap_R_DrawStretchPic( x, 0, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );     // tl
				trap_R_DrawStretchPic( x + w, 0, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ ); // tr
				trap_R_DrawStretchPic( x, h, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			}
		}

		// hairs
		CG_FillRect( 84, 239, 150, 3, color );   // left
		CG_FillRect( 234, 240, 173, 1, color );  // horiz center
		CG_FillRect( 407, 239, 150, 3, color );  // right


		CG_FillRect( 319, 2,   3, 151, color );  // top center top
		CG_FillRect( 320, 153, 1, 114, color );  // top center bot

		CG_FillRect( 320, 241, 1, 87, color );   // bot center top
		CG_FillRect( 319, 327, 3, 151, color );  // bot center bot
	}
}

/*
==============
CG_DrawBinocReticle
==============
*/
static void CG_DrawBinocReticle( void ) {
	// an alternative.  This gives nice sharp lines at the expense of a few extra polys
	vec4_t color = {0, 0, 0, 1};
	float x, y, w = 320, h = 240;
	float mask = 0, lb = 0;

	if ( cg_fixedAspect.integer ) {
		if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
			mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 640.0 ) ) / cgs.screenXScale );

			CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
			CG_FillRect( 0, 0, mask, 480, color );
			CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
			CG_FillRect( 640 - mask, 0, mask, 480, color );
		} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
			lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

			CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
			CG_FillRect( 0, 480 - lb, 640, lb, color );
			CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
			CG_FillRect( 0, 0, 640, lb, color );
		}
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	if ( cgs.media.binocShaderSimpleQ ) {
		CG_AdjustFrom640( &x, &y, &w, &h );
		if ( cg_fixedAspect.integer ) {
			trap_R_DrawStretchPic( mask * cgs.screenXScale, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.binocShaderSimpleQ );         // tl
			trap_R_DrawStretchPic( w + mask * cgs.screenXScale, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.binocShaderSimpleQ );     // tr
			trap_R_DrawStretchPic( mask * cgs.screenXScale, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.binocShaderSimpleQ );     // bl
			trap_R_DrawStretchPic( w + mask * cgs.screenXScale, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.binocShaderSimpleQ ); // br
		} else {
			trap_R_DrawStretchPic( 0, 0, w, h, 0, 0, 1, 1, cgs.media.binocShaderSimpleQ );  // tl
			trap_R_DrawStretchPic( w, 0, w, h, 1, 0, 0, 1, cgs.media.binocShaderSimpleQ );  // tr
			trap_R_DrawStretchPic( 0, h, w, h, 0, 1, 1, 0, cgs.media.binocShaderSimpleQ );  // bl
			trap_R_DrawStretchPic( w, h, w, h, 1, 1, 0, 0, cgs.media.binocShaderSimpleQ );  // br
		}
	}

	if ( cg_drawCrosshairBinoc.integer ) {
		CG_FillRect( 0, 239, 640, 1, color );	// horiz
		CG_FillRect( 320, 0, 1, 480, color );   // vert
	}
			

	CG_FillRect( 146, 239, 348, 1, color );

	CG_FillRect( 188, 234, 1, 13, color );   // ll
	CG_FillRect( 234, 226, 1, 29, color );   // l
	CG_FillRect( 274, 234, 1, 13, color );   // lr
	CG_FillRect( 320, 213, 1, 55, color );   // center
	CG_FillRect( 360, 234, 1, 13, color );   // rl
	CG_FillRect( 406, 226, 1, 29, color );   // r
	CG_FillRect( 452, 234, 1, 13, color );   // rr
}

void CG_FinishWeaponChange( int lastweap, int newweap ); // JPW NERVE


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( void ) {
	float w, h;
	qhandle_t hShader;
	float f;
	float x, y;
	int weapnum;                // DHM - Nerve
	vec4_t hcolor = {1, 1, 1, 1};
	qboolean friendInSights = qfalse;

	/*if ( cg.renderingThirdPerson ) {
		return;
	}*/

	hcolor[3] = cg_crosshairAlpha.value;    //----(SA)	added

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// on mg42
	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {
		hcolor[0] = hcolor[1] = hcolor[2] = 0.0f;
		hcolor[3] = 0.6f;
		// option 1
//		CG_FillRect (300, 240, 40, 2, hcolor);	// horizontal
//		CG_FillRect (319, 242, 2, 16, hcolor);	// vertical

		// option 2
		CG_FillRect( 305, 240, 30, 2, hcolor );  // horizontal
		CG_FillRect( 314, 256, 12, 2, hcolor );  // horizontal2
		CG_FillRect( 319, 242, 2, 32, hcolor );  // vertical

		return;
	}

	friendInSights = (qboolean)( cg.snap->ps.serverCursorHint == HINT_PLYR_FRIEND );  //----(SA)	added

	weapnum = cg.weaponSelect;

	switch ( weapnum ) {

		// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
	    return;
	case WP_GARAND:
		if ( cg.zoomedBinoc ) 
		{
			CG_DrawBinocReticle();
		}
	if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		break;

		// special reticle for weapon
	case WP_KNIFE:
	case WP_DAGGER:
		if ( cg.zoomedBinoc ) {
			CG_DrawBinocReticle();
			return;
		}

	    if ( !cg_drawCrosshair.integer ) {	
		    return;
	    }

		// no crosshair when looking at exits
		if ( cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT_FAR ) {
			return;
		}

		if ( !friendInSights ) {
			if ( !cg.snap->ps.leanf ) {     // no crosshair while leaning
				CG_FillRect( 319, 239, 2, 2, hcolor );      // dot
			}
			return;
		}
		break;

	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
	case WP_FG42SCOPE:
	case WP_DELISLESCOPE:
	case WP_M1941SCOPE:
		if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {
			CG_DrawWeapReticle();
			return;
		}
		break;
	default:
		break;
	}

	// using binoculars
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}

	// mauser only gets crosshair if you don't have the scope (I don't like this, but it's a test)
	if ( cg.weaponSelect == WP_MAUSER ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_SNIPERRIFLE ) ) {
		if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_DELISLE ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_DELISLESCOPE ) ) {
		if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_M1941 ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_M1941SCOPE ) ) {
		if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		}
	}

	if ( !cg_drawCrosshair.integer ) {	//----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if ( cg.snap->ps.leanf ) {
		return;
	}

	// no crosshair when looking at exits
	if ( cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT_FAR ) {
		return;
	}

	if ( cg_paused.integer ) {
		// no draw if any menu's are up	 (or otherwise paused)
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	if ( !cg_solidCrosshair.integer ) {
		f = (float)cg.snap->ps.aimSpreadScale / 255.0;
		w *= ( 1 + f * 2.0 );
		h *= ( 1 + f * 2.0 );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	if ( !cg_fixedAspect.integer ) {
		CG_AdjustFrom640( &x, &y, &w, &h );
	}

//----(SA)	modified
	if ( friendInSights ) {
		hShader = cgs.media.crosshairFriendly;
	} else {
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	}
//----(SA)	end

	// NERVE - SMF - modified, fixes crosshair offset in shifted/scaled 3d views
	// (SA) also breaks scaled view...
	if ( cg_fixedAspect.integer ) {
		CG_DrawPic( ((SCREEN_WIDTH-w)*0.5f)+x, ((SCREEN_HEIGHT-h)*0.5f)+y, w, h, hShader );
	} else {
		trap_R_DrawStretchPic(  x + cg.refdef.x + 0.5 * ( cg.refdef.width - w ),
								y + cg.refdef.y + 0.5 * ( cg.refdef.height - h ),
								w, h, 0, 0, 1, 1, hShader );
	}

	trap_R_SetColor( NULL );
}


/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D( void ) {
	float w;
	qhandle_t hShader;
	float f;
	int weapnum;                // DHM - Nerve
	vec4_t hcolor = {1, 1, 1, 1};
	qboolean friendInSights = qfalse;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	/*if ( cg.renderingThirdPerson ) {
		return;
	}*/

	hcolor[3] = cg_crosshairAlpha.value;    //----(SA)	added


	// on mg42
	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {
		hcolor[0] = hcolor[1] = hcolor[2] = 0.0f;
		hcolor[3] = 0.6f;
		// option 1
//		CG_FillRect (300, 240, 40, 2, hcolor);	// horizontal
//		CG_FillRect (319, 242, 2, 16, hcolor);	// vertical

		// option 2
		CG_FillRect( 305, 240, 30, 2, hcolor );  // horizontal
		CG_FillRect( 314, 256, 12, 2, hcolor );  // horizontal2
		CG_FillRect( 319, 242, 2, 32, hcolor );  // vertical

		return;
	}

	friendInSights = (qboolean)( cg.snap->ps.serverCursorHint == HINT_PLYR_FRIEND );  //----(SA)	added

	weapnum = cg.weaponSelect;

	switch ( weapnum ) {

		// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
	    return;
	case WP_GARAND:
		if ( cg.zoomedBinoc ) 
		{
			CG_DrawBinocReticle();
		}
	if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		break;
		// special reticle for weapon
	case WP_KNIFE:
	case WP_DAGGER:
		if ( cg.zoomedBinoc ) {
			CG_DrawBinocReticle();
			return;
		}

		// no crosshair when looking at exits
		if ( cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT_FAR ) {
			return;
		}

		if ( !friendInSights ) {
			if ( !cg.snap->ps.leanf ) {     // no crosshair while leaning
				CG_FillRect( 319, 239, 2, 2, hcolor );      // dot
			}
			return;
		}
		break;

	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
	case WP_FG42SCOPE:
	case WP_DELISLESCOPE:
	case WP_M1941SCOPE:
		if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {
			CG_DrawWeapReticle();
			return;
		}
		break;
	default:
		break;
	}

	// using binoculars
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}

	// mauser only gets crosshair if you don't have the scope (I don't like this, but it's a test)
	if ( cg.weaponSelect == WP_MAUSER ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_SNIPERRIFLE ) ) {
		if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_DELISLE ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_DELISLESCOPE ) ) {
		if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		}
	}

		if ( cg.weaponSelect == WP_M1941) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_M1941SCOPE ) ) {
		if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		}
	}


	if ( !cg_drawCrosshair.integer ) {	//----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if ( cg.snap->ps.leanf ) {
		return;
	}

	// no crosshair when looking at exits
	if ( cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT_FAR ) {
		return;
	}

	if ( cg_paused.integer ) {
		// no draw if any menu's are up	 (or otherwise paused)
		return;
	}

	w = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	f = (float)cg.snap->ps.aimSpreadScale / 255.0;
	w *= ( 1 + f * 2.0 );

//----(SA)	modified
	if ( friendInSights ) {
		hShader = cgs.media.crosshairFriendly;
	} else {
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	}
//----(SA)	end

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

	ent.shaderRGBA[0]=255;
	ent.shaderRGBA[1]=255;
	ent.shaderRGBA[2]=255;
	ent.shaderRGBA[3]=(byte)(hcolor[3]*255.f);

	trap_R_AddRefEntityToScene(&ent);
}

/*
==============
CG_DrawDynamiteStatus
==============
*/
static void CG_DrawDynamiteStatus( void ) {
	float color[4];
	char        *name;
	int timeleft;
	float w;

	if ( cg.snap->ps.weapon != WP_DYNAMITE ) {
		return;
	}

	if ( cg.snap->ps.grenadeTimeLeft <= 0 ) {
		return;
	}

	timeleft = cg.snap->ps.grenadeTimeLeft;

//	color = g_color_table[ColorIndex(COLOR_RED)];
	color[0] = color[3] = 1.0f;

	// fade red as it pulses past seconds
	color[1] = color[2] = 1.0f - ( (float)( timeleft % 1000 ) * 0.001f );

	if ( timeleft < 300 ) {        // fade up the text
		color[3] = (float)timeleft / 300.0f;
	}

	trap_R_SetColor( color );

	timeleft *= 5;
	timeleft -= ( timeleft % 5000 );
	timeleft += 5000;
	timeleft /= 1000;

	name = va( "Timer: %d", timeleft );
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;

	color[3] *= cg_hudAlpha.value;
	CG_DrawBigStringColor( 300 - w / 2, 170, name, color );

	trap_R_SetColor( NULL );
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t trace;
	vec3_t start, end;
	//int content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 4096, cg.refdef.viewaxis[0], end );  

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
			  cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM );

	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	/*content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}*/

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
	if ( cg.crosshairClientNum != cg.identifyClientNum && cg.crosshairClientNum != ENTITYNUM_WORLD ) {
		cg.identifyClientRequest = cg.crosshairClientNum;
	}
}



/*
==============
CG_CheckForCursorHints
==============
*/
void CG_CheckForCursorHints( void ) {

/*	if ( cg.renderingThirdPerson ) {
		return;
	}*/

	if ( cg.snap->ps.serverCursorHint != HINT_NONE ) { // let the client remember what was last looked at (for fading out)
		cg.cursorHintTime = cg.time;
		cg.cursorHintFade = cg_hintFadeTime.integer;    // fade out time
		cg.cursorHintIcon = cg.snap->ps.serverCursorHint;
		cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
	}

	// (SA) (8/14/01) removed all the client-side stuff.  don't think it's really necessary anymore
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float       *color;
	char        *name;
	float w;

	const char  *s;

	if ( cg_drawCrosshair.integer < 0 ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 100 );

	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	if ( cg.crosshairClientNum > MAX_CLIENTS ) {
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].translation;

	s = va( "%s", name );
	if ( !s ) {
		return;
	}
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;

	// draw the name and class
	CG_DrawSmallStringColor( 370 - w / 2, 190, s, color );

	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator( void ) {
	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}
	CG_DrawBigString( 320 - 9 * 8, 440, "SPECTATOR", 1.0F );
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote( void ) {
	char    *s;
	int sec;

	if ( !cgs.voteTime ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va( "VOTE(%i):%s yes(F1):%i no(F2):%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_DrawSmallString( 0, 58, s, 1.0F );
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {

	CG_DrawCenterString();
	CG_DrawSubtitleString();
	return;

	//cg.scoreFadeTime = cg.time;
	//CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	float x;
	vec4_t color;
	const char  *name;
	char deploytime[128];        // JPW NERVE

	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {
		return qfalse;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

// JPW NERVE -- if in limbo, show different follow message
	if ( cg.snap->ps.pm_flags & PMF_LIMBO ) {
//		CG_Printf("following %s\n",cgs.clientinfo[ cg.snap->ps.clientNum ].name);
		color[1] = 0;
		color[2] = 0;
		if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ) {
			Com_sprintf( deploytime, sizeof(deploytime), "Deploying in %d seconds", (int)( (float)( cg_redlimbotime.integer - ( cg.time % cg_redlimbotime.integer ) ) * 0.001f ) );
		} else {
			Com_sprintf( deploytime, sizeof(deploytime), "Deploying in %d seconds", (int)( (float)( cg_bluelimbotime.integer - ( cg.time % cg_bluelimbotime.integer ) ) * 0.001f ) );
		}

		x = 0.5 * ( 640 - BIGCHAR_WIDTH * strlen( deploytime ) ); //CG_DrawStrlen( deploytime ) );
		CG_DrawStringExt( x, 24, deploytime, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
		Com_sprintf( deploytime, sizeof(deploytime), "(Following %s)",cgs.clientinfo[ cg.snap->ps.clientNum ].name );
		x = 0.5 * ( 640 - BIGCHAR_WIDTH * strlen( deploytime ) ); //CG_DrawStrlen( deploytime ) );
		CG_DrawStringExt( x, 48, deploytime, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );

	} else {
// jpw
		CG_DrawBigString( 320 - 9 * 8, 24, "following", 1.0F );

		name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

		x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( name ) );

		CG_DrawStringExt( x, 40, name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
	} // JPW NERVE
	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
//----(SA)	forcing return for now
//			if we have messages to show here, comment back in
#if 0
	const char  *s;
	int w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 64, s, 1.0F );
#endif
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	return;     // (SA) don't bother with this stuff in sp
}

//==================================================================================

/*
=================
CG_DrawFlashFade
=================
*/
static void CG_DrawFlashFade( void ) {
	static int lastTime;
	int elapsed, time;
	vec4_t col;

	if ( cgs.scrFadeStartTime + cgs.scrFadeDuration < cg.time ) {
		cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
	} else if ( cgs.scrFadeAlphaCurrent != cgs.scrFadeAlpha ) {
		elapsed = ( time = trap_Milliseconds() ) - lastTime;  // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading
		lastTime = time;
		if ( elapsed < 500 && elapsed > 0 ) {
			if ( cgs.scrFadeAlphaCurrent > cgs.scrFadeAlpha ) {
				cgs.scrFadeAlphaCurrent -= ( (float)elapsed / (float)cgs.scrFadeDuration );
				if ( cgs.scrFadeAlphaCurrent < cgs.scrFadeAlpha ) {
					cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
				}
			} else {
				cgs.scrFadeAlphaCurrent += ( (float)elapsed / (float)cgs.scrFadeDuration );
				if ( cgs.scrFadeAlphaCurrent > cgs.scrFadeAlpha ) {
					cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
				}
			}
		}
	}
	// now draw the fade
	if ( cgs.scrFadeAlphaCurrent > 0.0 ) {
		VectorClear( col );
		col[3] = cgs.scrFadeAlphaCurrent;
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		 	CG_FillRect( 0, 0, 640, 480, col );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {	
			CG_FillRect( 0, 0, 640, 480, col );
		}
	}
}



/*
==============
CG_DrawFlashZoomTransition
	hide the snap transition from regular view to/from zoomed

  FIXME: TODO: use cg_fade?
==============
*/
static void CG_DrawFlashZoomTransition( void ) {
	vec4_t color;
	float frac;
	int fadeTime;

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {   // don't draw when on mg_42
		// keep the timer fresh so when you remove yourself from the mg42, it'll fade
		cg.zoomTime = cg.time;
		return;
	}

		if ( cg.zoomedScope ) {
			fadeTime = cg.zoomedScope;  //----(SA)
		} else {
			fadeTime = 300;
		}


	frac = cg.time - cg.zoomTime;

	if ( frac < fadeTime ) {
		frac = frac / (float)fadeTime;

		if ( cg.weaponSelect == WP_SNOOPERSCOPE ) {
//			Vector4Set( color, 0.7f, 0.3f, 0.7f, 1.0f - frac );
//			Vector4Set( color, 1, 0.5, 1, 1.0f - frac );
//			Vector4Set( color, 0.5f, 0.3f, 0.5f, 1.0f - frac );
			Vector4Set( color, 0.7f, 0.6f, 0.7f, 1.0f - frac );
		} else {
			Vector4Set( color, 0, 0, 0, 1.0f - frac );
		}

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( -10, -10, 650, 490, color );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( -10, -10, 650, 490, color );
		}
	}
}



/*
=================
CG_DrawFlashDamage
=================
*/
static void CG_DrawFlashDamage( void ) {
	vec4_t col;
	float redFlash;

	if ( !cg.snap ) {
		return;
	}

	if (!cg_blood.integer) {
		return;
	}

		// Blood blending
	if ( !cg_bloodBlend.integer ) {
		return;
	}
	// end

	if ( cg.v_dmg_time > cg.time ) {
		redFlash = fabs( cg.v_dmg_pitch * ( ( cg.v_dmg_time - cg.time ) / DAMAGE_TIME ) );

		// blend the entire screen red
		if ( redFlash > 5 ) {
			redFlash = 5;
		}

		VectorSet( col, 0.2, 0, 0 );
		col[3] =  0.7 * ( redFlash / 5.0 );

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( -10, -10, 650, 490, col );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( -10, -10, 650, 490, col );
		}
	}
}


/*
=================
CG_DrawFlashFire
=================
*/
static void CG_DrawFlashFire( void ) {
	vec4_t col = {1,1,1,1};
	float alpha, max, f;

	if ( !cg.snap ) {
		return;
	}

	if ( cg_thirdPerson.integer ) {
		return;
	}

	if ( cg.cameraMode ) { // don't draw flames on camera screen.  will still do damage though, so not a potential cheat
		return;
	}

	if ( !cg.snap->ps.onFireStart ) {
		cg.v_noFireTime = cg.time;
		return;
	}

	alpha = (float)( ( FIRE_FLASH_TIME - 1000 ) - ( cg.time - cg.snap->ps.onFireStart ) ) / ( FIRE_FLASH_TIME - 1000 );
	if ( alpha > 0 ) {
		if ( alpha >= 1.0 ) {
			alpha = 1.0;
		}

		// fade in?
		f = (float)( cg.time - cg.v_noFireTime ) / FIRE_FLASH_FADEIN_TIME;
		if ( f >= 0.0 && f < 1.0 ) {
			alpha = f;
		}

		max = 0.5 + 0.5 * sin( (float)( ( cg.time / 10 ) % 1000 ) / 1000.0 );
		if ( alpha > max ) {
			alpha = max;
		}
		col[0] = alpha;
		col[1] = alpha;
		col[2] = alpha;
		col[3] = alpha;
		trap_R_SetColor( col );
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_DrawPic( -10, -10, 650, 490, cgs.media.viewFlashFire[( cg.time / 50 ) % 16] );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_DrawPic( -10, -10, 650, 490, cgs.media.viewFlashFire[( cg.time / 50 ) % 16] );
		}
		trap_R_SetColor( NULL );

		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)( 255.0 * alpha ) );
		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.flameCrackSound, (int)( 255.0 * alpha ) );
	} else {
		cg.v_noFireTime = cg.time;
	}
}

/*
=================
CG_DrawFlashLightning
=================
*/
static void CG_DrawFlashLightning( void ) {
	centity_t *cent;
	qhandle_t shader;

	if ( !cg.snap ) {
		return;
	}

	if ( cg_thirdPerson.integer ) {
		return;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ( !cent->pe.teslaDamagedTime || ( cent->pe.teslaDamagedTime > cg.time ) ) {
		return;
	}

	if ( ( cg.time / 50 ) % ( 2 + ( cg.time % 2 ) ) == 0 ) {
		shader = cgs.media.viewTeslaAltDamageEffectShader;
	} else {
		shader = cgs.media.viewTeslaDamageEffectShader;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		CG_DrawPic( -10, -10, 650, 490, shader );
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	} else {
		CG_DrawPic( -10, -10, 650, 490, shader );
	}
}


/*
==============
CG_DrawFlashBlendBehindHUD
	screen flash stuff drawn first (on top of world, behind HUD)
==============
*/
static void CG_DrawFlashBlendBehindHUD( void ) {
	CG_DrawFlashZoomTransition();
}


/*
=================
CG_DrawFlashBlend
	screen flash stuff drawn last (on top of everything)
=================
*/
static void CG_DrawFlashBlend( void ) {
	CG_DrawFlashLightning();
	CG_DrawFlashFire();
	CG_DrawFlashDamage();
	CG_DrawFlashFade();
}

// NERVE - SMF
/*
=================
CG_DrawObjectiveInfo
=================
*/
#define OID_LEFT    10
#define OID_TOP     65

void CG_ObjectivePrint( const char *str, int charWidth, int team ) {
	char    *s;

	Q_strncpyz( cg.oidPrint, str, sizeof( cg.oidPrint ) );

	cg.oidPrintTime = cg.time;
	cg.oidPrintY = OID_TOP;
	cg.oidPrintCharWidth = charWidth;

	// count the number of lines for oiding
	cg.oidPrintLines = 1;
	s = cg.oidPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.oidPrintLines++;
		}
		s++;
	}
}

static void CG_DrawObjectiveInfo( void ) {
	char    *start;
	int l;
	int x, y, w;
	int x1, y1, x2, y2;
	float   *color;
	vec4_t backColor = { 0.2f, 0.2f, 0.2f, 1.f };

	if ( !cg.oidPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.oidPrintTime, 1000 * 5 );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.oidPrint;

	y = cg.oidPrintY - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	x1 = OID_LEFT - 2;
	y1 = y - 2;
	x2 = 0;

	// first just find the bounding rect
	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 40; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen( linebuffer );
		if ( x1 + w > x2 ) {
			x2 = x1 + w;
		}

		y += cg.oidPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	x2 = x2 + 4;
	y2 = y - cg.oidPrintCharWidth * 1.5 + 4;

	backColor[3] = color[3];
	CG_FillRect( x1, y1, x2 - x1, y2 - y1, backColor );

	VectorSet( backColor, 0, 0, 0 );
	CG_DrawRect( x1, y1, x2 - x1, y2 - y1, 1, backColor );

	//trap_R_SetColor( color );

	// do the actual drawing
	start = cg.oidPrint;
	y = cg.oidPrintY - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 40; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen( linebuffer );
		if ( x1 + w > x2 ) {
			x2 = x1 + w;
		}

		x = OID_LEFT;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
						  cg.oidPrintCharWidth, (int)( cg.oidPrintCharWidth * 1.5 ), 0 );

		y += cg.oidPrintCharWidth * 1.5;

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
// -NERVE - SMF

//==================================================================================


void CG_DrawTimedMenus( void ) {
	if ( cg.voiceTime ) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName( "voiceMenu" );
			trap_Cvar_Set( "cl_conXOffset", "0" );
			cg.voiceTime = 0;
		}
	}
}


/*
=================
CG_Fade
=================
*/
void CG_Fade( int r, int g, int b, int a, int time, int duration ) {

	// incorporate this into the current fade scheme

	cgs.scrFadeAlpha = (float)a / 255.0f;
	cgs.scrFadeStartTime = time;
	cgs.scrFadeDuration = duration;

	if ( cgs.scrFadeStartTime + cgs.scrFadeDuration <= cg.time ) {
		cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
	}

	return;
}


/*
===============
CG_DrawGameScreenFade
===============
*/
static void CG_DrawGameScreenFade( void ) {
	vec4_t col;

	if ( cg.viewFade <= 0.0 ) {
		return;
	}

	if ( !cg.snap ) {
		return;
	}

	VectorClear( col );
	col[3] = cg.viewFade;
	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		CG_FillRect( 0, 0, 640, 480, col );
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	} else {
		CG_FillRect( 0, 0, 640, 480, col );
	}
}

/*
=================
CG_ScreenFade
=================
*/
static void CG_ScreenFade( void ) {
	int msec;
	int i;
	float t, invt;
	vec4_t color;

	// Ridah, fade the screen (in-game)
	CG_DrawGameScreenFade();

	if ( !cg.fadeRate ) {
		return;
	}

	msec = cg.fadeTime - cg.time;
	if ( msec <= 0 ) {
		cg.fadeColor1[ 0 ] = cg.fadeColor2[ 0 ];
		cg.fadeColor1[ 1 ] = cg.fadeColor2[ 1 ];
		cg.fadeColor1[ 2 ] = cg.fadeColor2[ 2 ];
		cg.fadeColor1[ 3 ] = cg.fadeColor2[ 3 ];

		if ( !cg.fadeColor1[ 3 ] ) {
			cg.fadeRate = 0;
			return;
		}

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( 0, 0, 640, 480, cg.fadeColor1 );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( 0, 0, 640, 480, cg.fadeColor1 );
		}

	} else {
		t = ( float )msec * cg.fadeRate;
		invt = 1.0f - t;

		for ( i = 0; i < 4; i++ ) {
			color[ i ] = cg.fadeColor1[ i ] * t + cg.fadeColor2[ i ] * invt;
		}

		if ( color[ 3 ] ) {
			if ( cg_fixedAspect.integer ) {
				CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
				CG_FillRect( 0, 0, 640, 480, color );
				CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
			} else {
				CG_FillRect( 0, 0, 640, 480, color );
			}
		}
	}
}



/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame) {

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg.cameraMode ) { //----(SA)	no 2d when in camera view
		CG_DrawFlashBlend();    // (for fades)
	//	return;
	}

	if ( cg_nohudChallenge.integer ) {
		CG_DrawFlashBlend();    // (for fades)
		CG_DrawWeapReticle();   // (for scopes)
		CG_DrawCrosshair();
		CG_DrawHoldableSelect();
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();  // (for binocs)
		return;
	}
		return;
	}

	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}



	if ( cg_draw2D.integer == 0 ) {
		CG_DrawFlashBlend();    // (for fades)
		return;
	}

	CG_ScreenFade(); 

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	CG_DrawFlashBlendBehindHUD();

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead
		if ( cg.snap->ps.stats[STAT_HEALTH] > 0 && !cg.cameraMode) {

			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();

			if ( cg_drawStatus.integer ) {
				if ( cg_fixedAspect.integer == 2 ) {
					CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				}

if ( !cg_oldWolfUI.integer ) {
					Menu_PaintAll();
					CG_DrawTimedMenus();
				}
			}

			if ( cg_oldWolfUI.integer ) {
				CG_DrawStatusBar();
				CG_DrawHoldableItem_old();
			}
			CG_DrawAmmoWarning();
			CG_DrawDynamiteStatus();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();
			CG_DrawHoldableSelect();
			CG_DrawPickupItem();
			CG_DrawCheckpointString();
			CG_DrawGameSavedString();
			CG_DrawReward();
		}
	}

	CG_DrawVote();

	CG_DrawLagometer();

	if ( !cg_paused.integer ) {
		CG_DrawUpperRight(stereoFrame);
		//1NTERRUPTOR
		CG_DrawScriptLabel();
	}

	if ( cg_oldWolfUI.integer ) {
		CG_DrawLowerRight();
	}

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	// don't draw center string if scoreboard is up
	if ( !CG_DrawScoreboard() ) {
		CG_DrawCenterString();
		CG_DrawSubtitleString();
		CG_DrawObjectiveInfo();     // NERVE - SMF
	}

	// Ridah, draw flash blends now
	CG_DrawFlashBlend();
}

/*
====================
CG_StartShakeCamera
====================
*/
void CG_StartShakeCamera( float p, int duration, vec3_t src, float radius ) {
	int i;

	// find a free shake slot
	for ( i = 0; i < MAX_CAMERA_SHAKE; i++ ) {
		if ( cg.cameraShake[i].time > cg.time || cg.cameraShake[i].time + cg.cameraShake[i].length <= cg.time ) {
			break;
		}
	}

	if ( i == MAX_CAMERA_SHAKE ) {
		return; // no free slots

	}
	cg.cameraShake[i].scale = p;

	cg.cameraShake[i].length = duration;
	cg.cameraShake[i].time = cg.time;
	VectorCopy( src, cg.cameraShake[i].src );
	cg.cameraShake[i].radius = radius;
}

/*
====================
CG_CalcShakeCamera
====================
*/
void CG_CalcShakeCamera() {
	float val, scale, dist, x, sx;
	float bx = 0.0f; // TTimo: init
	int i;

	// build the scale
	scale = 0.0f;
	sx = (float)cg.time / 600.0; // x * (float)(cg.cameraShake[i].length) / 600.0;
	for ( i = 0; i < MAX_CAMERA_SHAKE; i++ ) {
		if ( cg.cameraShake[i].time <= cg.time && cg.cameraShake[i].time + cg.cameraShake[i].length > cg.time ) {
			dist = Distance( cg.cameraShake[i].src, cg.refdef.vieworg );
			// fade with distance
			val = cg.cameraShake[i].scale * ( 1.0f - ( dist / cg.cameraShake[i].radius ) );
			// fade with time
			x = 1.0f - ( ( cg.time - cg.cameraShake[i].time ) / cg.cameraShake[i].length );
			val *= x;
			// overwrite global scale if larger
			if ( val > scale ) {
				scale = val;
				bx = x;
			}
		}
	}

	// check the current rumble status
	if ( cg.rumbleScale > scale ) {
		scale = cg.rumbleScale;
		bx = cg.rumbleScale;
	}

	if ( scale <= 0.0f ) {
		cg.cameraShakePhase = crandom() * M_PI; // randomize the phase
		return;
	}

	if ( scale > 1.0f ) {
		scale = 1.0f;
	}

	// up/down
	val = sin( M_PI * 8 * sx + cg.cameraShakePhase ) * bx * 18.0f * scale;
	cg.cameraShakeAngles[0] = val;
	//cg.refdefViewAngles[0] += val;

	// left/right
	val = sin( M_PI * 15 * sx + cg.cameraShakePhase ) * bx * 16.0f * scale;
	cg.cameraShakeAngles[1] = val;
	//cg.refdefViewAngles[1] += val;

	// roll
	val = sin( M_PI * 12 * sx + cg.cameraShakePhase ) * bx * 10.0f * scale;
	cg.cameraShakeAngles[2] = val;
	//cg.refdefViewAngles[2] += val;
}

/*
====================
CG_ApplyShakeCamera
====================
*/
void CG_ApplyShakeCamera() {
	VectorAdd( cg.refdefViewAngles, cg.cameraShakeAngles, cg.refdefViewAngles );
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// if they are waiting at the mission stats screen, show the stats
		if ( strlen( cg_missionStats.string ) > 1 ) {
			trap_Cvar_Set( "com_expectedhunkusage", "-2" );
			CG_DrawInformation();
			return;
		}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		 ( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	cg.refdef.glfog.registered = 0; // make sure it doesn't use fog from another scene

	cg.refdef.rdflags |= RDF_DRAWSKYBOX;
	if ( !cg_skybox.integer ) {
		cg.refdef.rdflags &= ~RDF_DRAWSKYBOX;
	}

	trap_R_RenderScene( &cg.refdef );

	// clear around the rendered view if sized down
	CG_TileClear();     //----(SA)	moved to 2d section to avoid 2d/3d fog-state problems

	// draw status bar and other floating elements
	CG_Draw2D(stereoView);
}

