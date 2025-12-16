/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cvardef.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "eventscripts.h"
#include "com_weapons.h"
#include "draw_util.h"
#include "triangleapi.h"
#include "weapontype.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// Last weapon menu selection 

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = 0;
int g_weaponselect_frames = 0;
int g_iShotsFired;

void WeaponsResource :: LoadAllWeaponSprites( void )
{
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
}

int WeaponsResource :: CountAmmo( int iId ) 
{ 
	if ( iId < 0 )
		return 0;

	return riAmmo[iId];
}

int WeaponsResource :: HasAmmo( WEAPON *p )
{
	if ( !p )
		return FALSE;

	// weapons with no max ammo can always be selected
	if ( p->iMax1 == -1 )
		return TRUE;

	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo(p->iAmmoType) 
		|| CountAmmo(p->iAmmo2Type) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}


void WeaponsResource :: LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes = gHUD.GetSpriteRes();

	char sz[256];

	if ( !pWeapon )
		return;

	memset( &pWeapon->rcActive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcInactive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	snprintf(sz, sizeof(sz), "sprites/%s.txt", pWeapon->szName);
	client_sprite_t *pList = SPR_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t *p;
	
	p = GetSpriteList( pList, "crosshair", iRes, i );
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hCrosshair = SPR_Load(sz);
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = 0;

	p = GetSpriteList(pList, "autoaim", iRes, i);
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hAutoaim = SPR_Load(sz);
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList( pList, "zoom", iRes, i );
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedCrosshair = SPR_Load(sz);
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; //default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList(pList, "zoom_autoaim", iRes, i);
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedAutoaim = SPR_Load(sz);
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair;  //default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList(pList, "weapon", iRes, i);
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hInactive = SPR_Load(sz);
		pWeapon->rcInactive = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.Height() );
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

	p = GetSpriteList(pList, "ammo", iRes, i);
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo = SPR_Load(sz);
		pWeapon->rcAmmo = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.Height() );
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", iRes, i);
	if (p)
	{
		snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo2 = SPR_Load(sz);
		pWeapon->rcAmmo2 = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.Height() );
	}
	else
		pWeapon->hAmmo2 = 0;

}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] /*&& HasAmmo( rgSlots[iSlot][i] )*/ )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p || !gWR.HasAmmo(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"5"

int CHudAmmo::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(gHUD.m_Ammo, CurWeapon);
	HOOK_MESSAGE(gHUD.m_Ammo, WeaponList);
	HOOK_MESSAGE(gHUD.m_Ammo, AmmoPickup);
	HOOK_MESSAGE(gHUD.m_Ammo, WeapPickup);
	HOOK_MESSAGE(gHUD.m_Ammo, ItemPickup);
	HOOK_MESSAGE(gHUD.m_Ammo, HideWeapon);
	HOOK_MESSAGE(gHUD.m_Ammo, AmmoX);
	HOOK_MESSAGE(gHUD.m_Ammo, Crosshair);
	HOOK_MESSAGE(gHUD.m_Ammo, Brass);

	HOOK_COMMAND(gHUD.m_Ammo, "slot1", Slot1);
	HOOK_COMMAND(gHUD.m_Ammo, "slot2", Slot2);
	HOOK_COMMAND(gHUD.m_Ammo, "slot3", Slot3);
	HOOK_COMMAND(gHUD.m_Ammo, "slot4", Slot4);
	HOOK_COMMAND(gHUD.m_Ammo, "slot5", Slot5);
	HOOK_COMMAND(gHUD.m_Ammo, "slot6", Slot6);
	HOOK_COMMAND(gHUD.m_Ammo, "slot7", Slot7);
	HOOK_COMMAND(gHUD.m_Ammo, "slot8", Slot8);
	HOOK_COMMAND(gHUD.m_Ammo, "slot9", Slot9);
	HOOK_COMMAND(gHUD.m_Ammo, "slot10", Slot10);
	HOOK_COMMAND(gHUD.m_Ammo, "cancelselect", Close);
	HOOK_COMMAND(gHUD.m_Ammo, "invnext", NextWeapon);
	HOOK_COMMAND(gHUD.m_Ammo, "invprev", PrevWeapon);
	HOOK_COMMAND(gHUD.m_Ammo, "adjust_crosshair", Adjust_Crosshair);
	HOOK_COMMAND(gHUD.m_Ammo, "rebuy", Rebuy);
	HOOK_COMMAND(gHUD.m_Ammo, "autobuy", Autobuy);

	Reset();

	m_pHud_DrawHistory_Time = CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	m_pHud_FastSwitch = CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress
	// CVAR_CREATE( "cl_observercrosshair", "1", 0 );
	m_pClCrosshairColor = (convar_t*)CVAR_CREATE( "cl_crosshair_color", "50 250 50", FCVAR_ARCHIVE );
	m_pClCrosshairTranslucent = (convar_t*)CVAR_CREATE( "cl_crosshair_translucent", "1", FCVAR_ARCHIVE );
	m_pClCrosshairSize = (convar_t*)CVAR_CREATE( "cl_crosshair_size", "auto", FCVAR_ARCHIVE );
	m_pClDynamicCrosshair = CVAR_CREATE("cl_dynamiccrosshair", "1", FCVAR_ARCHIVE);

	m_hStaticSpr = 0;

	m_iFlags = HUD_DRAW | HUD_THINK; //!!!
	m_R = 50;
	m_G = 250;
	m_B = 50;
	m_iAlpha = 200;

	m_cvarB = m_cvarR = m_cvarG = -1;
	m_iCurrentCrosshair = 0;
	m_bAdditive = true;
	m_iCrosshairScaleBase = -1;
	m_bDrawCrosshair = true;

	gWR.Init();
	gHR.Init();

	xhair_enable = CVAR_CREATE( "xhair_enable", "0", FCVAR_ARCHIVE );
	xhair_gap = CVAR_CREATE( "xhair_gap", "0", FCVAR_ARCHIVE );
	xhair_size = CVAR_CREATE( "xhair_size", "4", FCVAR_ARCHIVE );
	xhair_thick = CVAR_CREATE( "xhair_thick", "0", FCVAR_ARCHIVE );
	xhair_pad = CVAR_CREATE( "xhair_pad", "0", FCVAR_ARCHIVE );
	xhair_dot = CVAR_CREATE( "xhair_dot", "0", FCVAR_ARCHIVE );
	xhair_t = CVAR_CREATE( "xhair_t", "0", FCVAR_ARCHIVE );
	xhair_dynamic_scale = CVAR_CREATE( "xhair_dynamic_scale", "0", FCVAR_ARCHIVE );
	xhair_gap_useweaponvalue = CVAR_CREATE( "xhair_gap_useweaponvalue", "0", FCVAR_ARCHIVE );
	xhair_dynamic_move = CVAR_CREATE( "xhair_dynamic_move", "1", FCVAR_ARCHIVE );

	xhair_color = CVAR_CREATE( "xhair_color", "0 255 0 255", FCVAR_ARCHIVE );
	xhair_additive = CVAR_CREATE( "xhair_additive", "0", FCVAR_ARCHIVE );

	return 1;
}

void CHudAmmo::Reset(void)
{
	m_fFade = 0;

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();

	//	VidInit();

}

int CHudAmmo::VidInit(void)
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).Width();
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).Height();

	gHR.iHistoryGap = max( gHR.iHistoryGap, giBucketHeight);

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	giABWidth = 20;
	giABHeight = 4;

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think(void)
{
	if ( gHUD.m_fPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = 0; i < MAX_WEAPONS-1; i++ )
		{
			WEAPON *p = gWR.GetWeapon(i);

			if ( p )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->iId ) )
				{
					gWR.PickupWeapon( p );
				}
				else
				{
					if( gHUD.GetGameType() != GAME_CZERODS )
						gWR.DropWeapon( p );
				}
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (gpActiveSel != (WEAPON *)1)
		{
			ServerCmd(gpActiveSel->szName);
			g_weaponselect = gpActiveSel->iId;
			g_weaponselect_frames = 3;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}

}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if ( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}


// Menu Selection Code

void WeaponsResource :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return;

	if ( ! ( gHUD.m_iWeaponBits & ~(1<<(WEAPON_SUIT)) ))
		return;

	WEAPON *p = NULL;
	bool fastSwitch = gHUD.m_Ammo.m_pHud_FastSwitch->value != 0.0f;

	if ( (gpActiveSel == NULL) || (gpActiveSel == (WEAPON *)1) || (iSlot != gpActiveSel->iSlot) )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );
			if ( !p2 )
			{	// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				g_weaponselect = p->iId;
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if ( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	int iIndex = reader.ReadByte();
	int iCount = reader.ReadByte();

	gWR.SetAmmo( iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int iIndex = reader.ReadByte();
	int iCount = reader.ReadByte();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int iIndex = reader.ReadByte();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	if( gHUD.GetGameType() == GAME_CZERODS )
	{
		gWR.PickupWeapon( iIndex );
	}

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	const char *szName = reader.ReadString();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	return 1;
}


int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = reader.ReadByte();

	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) )
	{
		gpActiveSel = NULL;
		HideCrosshair();
	}

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int iState = reader.ReadByte();
	int iId = reader.ReadChar();
	int iClip = reader.ReadChar();

	if ( iId < 1 )
	{
		HideCrosshair();
		return 0;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = TRUE;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = FALSE;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if ( !pWeapon )
		return 0;

	if ( iClip < -1 )
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;


	if ( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	m_fFade = 200.0f; //!!!

	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	
	WEAPON Weapon;

	strncpy( Weapon.szName, reader.ReadString(), MAX_WEAPON_NAME );
	Weapon.szName[MAX_WEAPON_NAME-1] = 0;
	Weapon.iAmmoType = (int)reader.ReadChar();
	
	Weapon.iMax1 = reader.ReadByte();
	if (Weapon.iMax1 == 255)
		Weapon.iMax1 = -1;

	Weapon.iAmmo2Type = reader.ReadChar();
	Weapon.iMax2 = reader.ReadByte();
	if (Weapon.iMax2 == 255)
		Weapon.iMax2 = -1;

	Weapon.iSlot = reader.ReadChar();
	Weapon.iSlotPos = reader.ReadChar();
	Weapon.iId = reader.ReadChar();
	Weapon.iFlags = reader.ReadByte();
	Weapon.iClip = 0;

	gWR.AddWeapon( &Weapon );

	return 1;

}

int CHudAmmo::MsgFunc_Crosshair(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	if( reader.ReadByte() > 0)
	{
		m_bDrawCrosshair = true;
	}
	else
	{
		m_bDrawCrosshair = false;
	}
   return 0;
}

int CHudAmmo::MsgFunc_Brass( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	reader.ReadByte(); // unused!

	Vector origin, velocity;
	origin.x = reader.ReadCoord();
	origin.y = reader.ReadCoord();
	origin.z = reader.ReadCoord();
	reader.ReadCoord(); // unused!
	reader.ReadCoord(); // unused!
	reader.ReadCoord(); // unused!
	velocity.x = reader.ReadCoord();
	velocity.y = reader.ReadCoord();
	velocity.z = reader.ReadCoord();

	float Rotation = M_PI * reader.ReadAngle() / 180.0f;
	int ModelIndex = reader.ReadShort();
	int BounceSoundType = reader.ReadByte();
	int Life = reader.ReadByte();
	int Client = reader.ReadByte();

	float sin, cos, x, y;
	sin = fabs(Rotation);
	cos = fabs(Rotation);

	if( gHUD.cl_righthand->value != 0.0f && EV_IsLocal( Client ) )
	{
		velocity.x += sin * -120.0;
		velocity.y += cos * 120.0;
		x = 9.0 * sin;
		y = -9.0 * cos;
	}
	else
	{
		x = -9.0 * sin;
		y = 9.0 * cos;
	}

	origin.x += x;
	origin.y += y;
	EV_EjectBrass( origin, velocity, Rotation, ModelIndex, BounceSoundType, Life );
	return 1;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	gWR.SelectSlot(iSlot, FALSE, 1);
}

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Adjust_Crosshair()
{
	int newCrosshair;
	int oldCrosshair = m_iCurrentCrosshair;

	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		newCrosshair = (oldCrosshair + 1) % 5;
	}
	else
	{
		const char *arg = gEngfuncs.Cmd_Argv(1);
		newCrosshair = atoi(arg) % 10;
	}

	m_iCurrentCrosshair = newCrosshair;
	if ( newCrosshair <= 9 )
	{
		switch ( newCrosshair )
		{
		case 0:
		case 5:
			m_R = 50;
			m_G = 250;
			m_B = 50;
			break;
		case 1:
		case 6:
			m_R = 250;
			m_G = 50;
			m_B = 50;
			break;
		case 2:
		case 7:
			m_R = 50;
			m_G = 50;
			m_B = 250;
			break;
		case 3:
		case 8:
			m_R = 250;
			m_G = 250;
			m_B = 50;
			break;
		case 4:
		case 9:
			m_R = 50;
			m_G = 250;
			m_B = 250;
			break;
		}
		m_bAdditive = newCrosshair < 5 ? true: false;
	}
	else
	{
		m_R = 50;
		m_G = 250;
		m_B = 50;
		m_bAdditive = 1;
	}

	char s[16];
	sprintf(s, "%d %d %d", m_R, m_G, m_B);
	gEngfuncs.Cvar_Set("cl_crosshair_color", s);
	gEngfuncs.Cvar_Set("cl_crosshair_translucent", (char*)(m_bAdditive ? "1" : "0"));
}


void CHudAmmo::UserCmd_Close(void)
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		ClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp /*&& gWR.HasAmmo(wsp)*/ )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS-1;
	int slot = MAX_WEAPON_SLOTS-1;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}
	
	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot >= 0; slot-- )
		{
			for ( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp /*&& gWR.HasAmmo(wsp)*/ )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	gpActiveSel = NULL;
}

void CHudAmmo::UserCmd_Autobuy()
{
	char *afile = (char*)gEngfuncs.COM_LoadFile("autobuy.txt", 5, NULL);
	char *pfile = afile;
	char token[1024];
	char szCmd[1024];
	int remaining = 1023;

	if( !pfile )
	{
		ConsolePrint( "Can't open autobuy.txt file.\n" );
		return;
	}

	strcpy(szCmd, "cl_setautobuy");
	remaining -= sizeof( "cl_setautobuy" );

	while((pfile = gEngfuncs.COM_ParseFile( pfile, token )))
	{
		// append space first
		strncat(szCmd, " ", remaining);
		strncat(szCmd, token, remaining - 1);

		remaining -= strlen( token ) - 1;
	}

	gEngfuncs.pfnServerCmd( szCmd );
	gEngfuncs.COM_FreeFile( afile );
}

void CHudAmmo::UserCmd_Rebuy()
{
	char *afile = (char*)gEngfuncs.COM_LoadFile("rebuy.txt", 5, NULL);
	char *pfile = afile;
	char token[1024];
	char szCmd[1024];
	int lastCh;
	int remaining = 1023;

	if( !pfile )
	{
		ConsolePrint( "Can't open rebuy.txt file.\n" );
		return;
	}

	// start with \"
	strcpy(szCmd, "cl_setrebuy \"" );
	remaining -= sizeof( "cl_setrebuy \"" );

	while((pfile = gEngfuncs.COM_ParseFile( pfile, token )))
	{
		strncat(szCmd, token, remaining );
		remaining -= strlen( token );

		// append space after token
		strncat(szCmd, " ", remaining );
		remaining--;
	}
	// replace last space with ", before terminator
	lastCh = strlen(szCmd);
	szCmd[lastCh] = '\"';

	gEngfuncs.pfnServerCmd( szCmd );
	gEngfuncs.COM_FreeFile( afile );
}


//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

int CHudAmmo::Draw(float flTime)
{
	int a, x, y, r, g, b;
	int AmmoWidth;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	// place it here, so pretty dynamic crosshair will work even in spectator!
	if( gHUD.m_iFOV > 40 )
	{
		HideCrosshair(); // hide static

		// draw a dynamic crosshair
		DrawCrosshair();
	}
	else
	{
		if( m_pWeapon )
		{
			SetCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 255, 255);

			DrawSpriteCrosshair();
		}
	}

	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
		return 1;

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if (!m_pWeapon)
		return 0;

	WEAPON *pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
		return 0;

	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).Width();

	a = (int) max( MIN_ALPHA, m_fFade );

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	DrawUtils::ScaleColors(r, g, b, a );

	// Does this weapon have a clip?
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight/2;

	// Does weapon have any ammo at all?
	if (m_pWeapon->iAmmoType > 0)
	{
		int iIconWidth = m_pWeapon->rcAmmo.Width();
		
		if (pw->iClip >= 0)
		{
			// room for the number and the '|' and the current ammo
			
			x = ScreenWidth - (8 * AmmoWidth) - iIconWidth;
			x = DrawUtils::DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b);

			int iBarWidth =  AmmoWidth/10;

			x += AmmoWidth/2;

			DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

			// draw the | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);

			x += iBarWidth + AmmoWidth/2;;

			// GL Seems to need this
			DrawUtils::ScaleColors(r, g, b, a );
			x = DrawUtils::DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);


		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = DrawUtils::DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);
		}

		// Draw the ammo Icon
		int iOffset = (m_pWeapon->rcAmmo.Height())/8;
		SPR_Set(m_pWeapon->hAmmo, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo);
	}

	// Does weapon have seconday ammo?
	if (pw->iAmmo2Type > 0) 
	{
		int iIconWidth = m_pWeapon->rcAmmo2.Width();

		// Do we have secondary ammo?
		if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) > 0))
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight/4;
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = DrawUtils::DrawHudNumber(x, y, iFlags|DHN_3DIGITS, gWR.CountAmmo(pw->iAmmo2Type), r, g, b);

			// Draw the ammo Icon
			SPR_Set(m_pWeapon->hAmmo2, r, g, b);
			int iOffset = (m_pWeapon->rcAmmo2.Height())/8;
			SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo2);
		}
	}

	return 1;
}

void CHudAmmo::DrawSpriteCrosshair()
{
	int x, y;

	if( !m_hStaticSpr )
		return;

	gEngfuncs.pfnSPR_Set( m_hStaticSpr, m_staticRgba.r, m_staticRgba.g, m_staticRgba.b );

	x = ( ScreenWidth - m_rcStaticRc.Width() ) / 2;
	y = ( ScreenHeight - m_rcStaticRc.Height() ) / 2;

	// gEngfuncs.pfnSPR_Draw( 0, x, y, &m_rcStaticRc );
	gEngfuncs.pfnSPR_DrawHoles( 0, x, y, &m_rcStaticRc );
}

#define WEST_XPOS (ScreenWidth / 2 - flCrosshairDistance - iLength + 1)
#define EAST_XPOS (flCrosshairDistance + ScreenWidth / 2)
#define EAST_WEST_YPOS (ScreenHeight / 2)

#define NORTH_YPOS (ScreenHeight / 2 - flCrosshairDistance - iLength + 1)
#define SOUTH_YPOS (ScreenHeight / 2 + flCrosshairDistance)
#define NORTH_SOUTH_XPOS (ScreenWidth / 2)

#define WEST_XPOS_R (TrueWidth / 2 - flCrosshairDistance - iLength + 1)
#define EAST_XPOS_R (flCrosshairDistance + TrueWidth / 2)
#define EAST_WEST_YPOS_R (TrueHeight / 2)

#define NORTH_YPOS_R (TrueHeight / 2 - flCrosshairDistance - iLength + 1)
#define SOUTH_YPOS_R (TrueHeight / 2 + flCrosshairDistance)
#define NORTH_SOUTH_XPOS_R (TrueWidth / 2)

int Distances[30][2] =
{
{ 8, 3 }, // 0
{ 4, 3 }, // 1
{ 5, 3 }, // 2
{ 8, 3 }, // 3
{ 9, 4 }, // 4
{ 6, 3 }, // 5
{ 9, 3 }, // 6
{ 3, 3 }, // 7
{ 8, 3 }, // 8
{ 4, 3 }, // 9
{ 8, 3 }, // 10
{ 6, 3 }, // 11
{ 5, 3 }, // 12
{ 4, 3 }, // 13
{ 4, 3 }, // 14
{ 8, 3 }, // 15
{ 8, 3 }, // 16
{ 8, 3 }, // 17
{ 6, 3 }, // 18
{ 6, 3 }, // 19
{ 8, 6 }, // 20
{ 4, 3 }, // 21
{ 7, 3 }, // 22
{ 6, 4 }, // 23
{ 8, 3 }, // 24
{ 8, 3 }, // 25
{ 5, 3 }, // 26
{ 4, 4 }, // 27
{ 7, 3 }, // 28
{ 7, 3 }, // 29
};

enum
{
	ACCURACY_NONE = 0,
	ACCURACY_JUMP = ( 1 << 0 ),
	ACCURACY_RUN = ( 1 << 1 ),
	// ACCURACY_DUCK = (1 << 2),
	ACCURACY_INACCURATE = ( 1 << 3 ),
	ACCURACY_VERY_INACCURATE = ( 1 << 4 )
};

int CHudAmmo::GetWeaponAccuracyFlags( int weaponId )
{
	int xhairWeaponFlags = g_iWeaponFlags;

	switch ( weaponId )
	{
	case WEAPON_P228:
	case WEAPON_FIVESEVEN:
	case WEAPON_DEAGLE:
		return ( ACCURACY_DUCK | ACCURACY_RUN | ACCURACY_JUMP );

	case WEAPON_MAC10:
	case WEAPON_UMP45:
	case WEAPON_MP5N:
	case WEAPON_TMP:
		return ACCURACY_JUMP;

	case WEAPON_AUG:
	case WEAPON_GALIL:
	case WEAPON_M249:
	case WEAPON_SG552:
	case WEAPON_AK47:
	case WEAPON_P90:
		return ( ACCURACY_RUN | ACCURACY_JUMP );

	case WEAPON_FAMAS:
		return ( xhairWeaponFlags & 16 ) ? ( ACCURACY_VERY_INACCURATE | ACCURACY_RUN | ACCURACY_JUMP ) : ( ACCURACY_RUN | ACCURACY_JUMP );

	case WEAPON_USP:
		return ( xhairWeaponFlags & 1 ) ? ( ACCURACY_INACCURATE | ACCURACY_DUCK | ACCURACY_RUN | ACCURACY_JUMP ) : ( ACCURACY_DUCK | ACCURACY_RUN | ACCURACY_JUMP );

	case WEAPON_GLOCK18:
		return ( xhairWeaponFlags & 2 ) ? ( ACCURACY_VERY_INACCURATE | ACCURACY_DUCK | ACCURACY_RUN | ACCURACY_JUMP ) : ( ACCURACY_DUCK | ACCURACY_RUN | ACCURACY_JUMP );

	case WEAPON_M4A1:
		return ( xhairWeaponFlags & 4 ) ? ( ACCURACY_INACCURATE | ACCURACY_RUN | ACCURACY_JUMP ) : ( ACCURACY_RUN | ACCURACY_JUMP );
	}

	return ACCURACY_NONE;
}

#define MAX_XHAIR_GAP 15

int CHudAmmo::ScaleForRes( float value, int height )
{
	/* "default" resolution is 640x480 */
	return rint( value * ( (float)height / 480.0f ) );
}

float CHudAmmo::GetCrosshairGap( int weaponId )
{
	static float xhairGap;
	static int lastShotsFired;
	static float xhairPrevTime;
	float minGap, deltaGap;

	int xhairPlayerFlags = g_iPlayerFlags;
	float xhairPlayerSpeed = g_flPlayerSpeed;
	float clientTime = gEngfuncs.GetClientTime();
	int xhairShotsFired = g_iShotsFired;

	switch ( weaponId )
	{
	case WEAPON_P228:
	case WEAPON_HEGRENADE:
	case WEAPON_SMOKEGRENADE:
	case WEAPON_FIVESEVEN:
	case WEAPON_USP:
	case WEAPON_GLOCK18:
	case WEAPON_AWP:
	case WEAPON_FLASHBANG:
	case WEAPON_DEAGLE:
		minGap = 8;
		deltaGap = 3;
		break;

	case WEAPON_SCOUT:
	case WEAPON_SG550:
	case WEAPON_SG552:
		minGap = 5;
		deltaGap = 3;
		break;

	case WEAPON_XM1014:
		minGap = 9;
		deltaGap = 4;
		break;

	case WEAPON_C4:
	case WEAPON_UMP45:
	case WEAPON_M249:
		minGap = 6;
		deltaGap = 3;
		break;

	case WEAPON_MAC10:
		minGap = 9;
		deltaGap = 3;
		break;

	case WEAPON_AUG:
		minGap = 3;
		deltaGap = 3;
		break;

	case WEAPON_MP5N:
		minGap = 6;
		deltaGap = 2;
		break;

	case WEAPON_M3:
		minGap = 8;
		deltaGap = 6;
		break;

	case WEAPON_TMP:
	case WEAPON_KNIFE:
	case WEAPON_P90:
		minGap = 7;
		deltaGap = 3;
		break;

	case WEAPON_G3SG1:
		minGap = 6;
		deltaGap = 4;
		break;

	case WEAPON_AK47:
		minGap = 4;
		deltaGap = 4;
		break;

	default:
		minGap = 4;
		deltaGap = 3;
		break;
	}

	if ( !xhair_gap_useweaponvalue->value )
		minGap = 4;

	float baseMinGap = minGap;
	float absMinGap = baseMinGap * 0.5f;

	int flags = GetWeaponAccuracyFlags( weaponId );
	if ( xhair_dynamic_move->value && flags )
	{
		if ( !( xhairPlayerFlags & FL_ONGROUND ) && ( flags & ACCURACY_AIR ) )
		{
			minGap *= 2.0f;
		}
		else if ( ( xhairPlayerFlags & FL_DUCKING ) && ( flags & ACCURACY_DUCK ) )
		{
			minGap *= 0.5f;
		}
		else
		{
			float runLimit;

			switch ( weaponId )
			{
			case WEAPON_AUG:
			case WEAPON_GALIL:
			case WEAPON_FAMAS:
			case WEAPON_M249:
			case WEAPON_M4A1:
			case WEAPON_SG552:
			case WEAPON_AK47:
				runLimit = 140;
				break;

			case WEAPON_P90:
				runLimit = 170;
				break;

			default:
				runLimit = 0;
				break;
			}

			if ( xhairPlayerSpeed > runLimit && ( flags & ACCURACY_RUN ) )
				minGap *= 1.5f;
		}

		if ( flags & ACCURACY_INACCURATE )
			minGap *= 1.4f;

		if ( flags & ACCURACY_VERY_INACCURATE )
			minGap *= 1.4f;

		minGap = baseMinGap + ( minGap - baseMinGap ) * xhair_dynamic_scale->value;
		minGap = max( minGap, absMinGap );
	}

	if ( xhairPrevTime > clientTime )
	{
		// client restart
		xhairPrevTime = clientTime;
	}

	float deltaTime = clientTime - xhairPrevTime;
	xhairPrevTime = clientTime;

	if ( xhairShotsFired <= lastShotsFired )
	{
		// decay the crosshair as if we were always running at 100 fps
		xhairGap -= ( 100 * deltaTime ) * ( 0.013f * xhairGap + 0.1f );
	}
	else
	{
		xhairGap += deltaGap * xhair_dynamic_scale->value;
		xhairGap = min( xhairGap, MAX_XHAIR_GAP );
	}

	if ( xhairShotsFired > 600 )
		xhairShotsFired = 1;

	lastShotsFired = xhairShotsFired;

	xhairGap = max( xhairGap, minGap );

	return xhairGap + xhair_gap->value;
}

void CHudAmmo::DrawCrosshairSection( int _x0, int _y0, int _x1, int _y1 )
{
	float x0 = (float)_x0;
	float y0 = (float)_y0;
	float x1 = (float)_x1;
	float y1 = (float)_y1;
	int color[4] = { 0, 255, 0, 255 };

	// float top_left[2] = { x0, y0 };
	// float top_right[2] = { x1, y0 };
	// float bottom_right[2] = { x1, y1 };
	// float bottom_left[2] = { x0, y1 };

	if ( sscanf( xhair_color->string, "%d %d %d %d", &color[0], &color[1], &color[2], &color[3] ) == 4 )
	{
		color[0] = bound( 0, color[0], 255 );
		color[1] = bound( 0, color[1], 255 );
		color[2] = bound( 0, color[2], 255 );
		color[3] = bound( 0, color[3], 255 );
	}
	gEngfuncs.pTriAPI->Color4f( color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, color[3] / 255.0f );
	// gEngfuncs.pTriAPI->Brightness( 1.0f );

	DrawUtils::Draw2DQuad( x0, y0, x1, y1 );
}

void CHudAmmo::DrawCrosshairPadding( int _pad, int _x0, int _y0, int _x1, int _y1 )
{
	float pad = (float)_pad;
	float x0 = (float)_x0;
	float y0 = (float)_y0;
	float x1 = (float)_x1;
	float y1 = (float)_y1;
	int alpha = 255;

	// float out_top_left[2] = { x0 - pad, y0 - pad };
	// float out_top_right[2] = { x1 + pad, y0 - pad };
	// float out_bottom_right[2] = { x1 + pad, y1 + pad };
	// float out_bottom_left[2] = { x0 - pad, y1 + pad };
	// float in_top_left[2] = { x0, y0 };
	// float in_top_right[2] = { x1, y0 };
	// float in_bottom_right[2] = { x1, y1 };
	// float in_bottom_left[2] = { x0, y1 };

	if ( sscanf( xhair_color->string, "%*d %*d %*d %d", &alpha ) == 1 )
	{
		alpha = bound( 0, alpha, 255 );
	}
	gEngfuncs.pTriAPI->Color4f( 0, 0, 0, alpha / 255.0f );
	// gEngfuncs.pTriAPI->Brightness( 1.0f );

	DrawUtils::Draw2DQuad( x0 - pad, y0 - pad, x1 + pad, y0 ); // top part
	DrawUtils::Draw2DQuad( x0 - pad, y1, x1 + pad, y1 + pad ); // bottom part
	DrawUtils::Draw2DQuad( x0 - pad, y0, x0, y1 );             // left part
	DrawUtils::Draw2DQuad( x1, y0, x1 + pad, y1 );             // right part
}

void CHudAmmo::DrawCrosshair( int weaponId )
{
	int center_x, center_y;
	int gap, length, thickness;
	int y0, y1, x0, x1;
	wrect_t inner;
	wrect_t outer;

	/* calculate the coordinates */
	center_x = ( ScreenWidth / 2 ) * gHUD.m_flScale;
	center_y = ( ScreenHeight / 2 ) * gHUD.m_flScale;

	gap = ScaleForRes( GetCrosshairGap( weaponId ), ScreenHeight * gHUD.m_flScale );
	length = ScaleForRes( xhair_size->value, ScreenHeight * gHUD.m_flScale );
	thickness = ScaleForRes( xhair_thick->value, ScreenHeight * gHUD.m_flScale );
	thickness = max( 1, thickness );

	inner.left = ( center_x - gap - thickness / 2 );
	inner.right = ( inner.left + 2 * gap + thickness );
	inner.top = ( center_y - gap - thickness / 2 );
	inner.bottom = ( inner.top + 2 * gap + thickness );

	outer.left = ( inner.left - length );
	outer.right = ( inner.right + length );
	outer.top = ( inner.top - length );
	outer.bottom = ( inner.bottom + length );

	y0 = ( center_y - thickness / 2 );
	x0 = ( center_x - thickness / 2 );
	y1 = ( y0 + thickness );
	x1 = ( x0 + thickness );

	gEngfuncs.pTriAPI->Brightness( 1.0f );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	gRenderAPI.GL_SelectTexture( 0 );
	gRenderAPI.GL_Bind( 0, gHUD.m_WhiteTex );

	if ( xhair_additive->value )
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	else
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );

	if ( xhair_dot->value )
		DrawCrosshairSection( x0, y0, x1, y1 );

	if ( !xhair_t->value )
		DrawCrosshairSection( x0, outer.top, x1, inner.top );

	DrawCrosshairSection( x0, inner.bottom, x1, outer.bottom );
	DrawCrosshairSection( outer.left, y0, inner.left, y1 );
	DrawCrosshairSection( inner.right, y0, outer.right, y1 );

	if ( xhair_additive->value )
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );

	/* draw padding if wanted */
	if ( xhair_pad->value )
	{
		/* don't scale this */
		int pad = (int)xhair_pad->value;

		if ( xhair_dot->value )
			DrawCrosshairPadding( pad, x0, y0, x1, y1 );

		if ( !xhair_t->value )
			DrawCrosshairPadding( pad, x0, outer.top, x1, inner.top );

		DrawCrosshairPadding( pad, x0, inner.bottom, x1, outer.bottom );
		DrawCrosshairPadding( pad, outer.left, y0, inner.left, y1 );
		DrawCrosshairPadding( pad, inner.right, y0, outer.right, y1 );
	}
}

void CHudAmmo::DrawCrosshair()
{
	int flags, iDeltaDistance, iDistance, iLength, weaponid;
	float flCrosshairDistance;

	if ( !m_pWeapon )
		return;

	weaponid = m_pWeapon->iId;

	if ( weaponid == WEAPON_AWP
	     || weaponid == WEAPON_SCOUT
	     || weaponid == WEAPON_SG550
	     || weaponid == WEAPON_G3SG1 )
		return;

	if ( g_iWeaponFlags & WPNSTATE_SHIELD_DRAWN )
		return;

	if ( weaponid <= 30 )
	{
		iDistance = Distances[weaponid - 1][0];
		iDeltaDistance = Distances[weaponid - 1][1];
	}
	else
	{
		iDistance = 4;
		iDeltaDistance = 3;
	}

	flags = GetWeaponAccuracyFlags( weaponid );
	if ( flags && m_pClDynamicCrosshair->value && !( gHUD.m_iHideHUDDisplay & 1 ) )
	{
		if ( g_iPlayerFlags & FL_ONGROUND || !( flags & ACCURACY_AIR ) )
		{
			if ( ( g_iPlayerFlags & FL_DUCKING ) && ( flags & ACCURACY_DUCK ) )
			{
				iDistance *= 0.5;
			}
			else
			{
				int iWeaponSpeed = 0;

				switch ( weaponid )
				{
				case WEAPON_P90: // p90
					iWeaponSpeed = 170;
					break;
				case WEAPON_AUG:   // aug
				case WEAPON_GALIL: // galil
				case WEAPON_FAMAS: // famas
				case WEAPON_M249:  // m249
				case WEAPON_M4A1:  // m4a1
				case WEAPON_SG552: // sg552
				case WEAPON_AK47:  // ak47
					iWeaponSpeed = 140;
					break;
				}

				if ( ( flags & ACCURACY_SPEED ) && ( g_flPlayerSpeed >= iWeaponSpeed ) )
					iDistance *= 1.5;
			}
		}
		else
		{
			iDistance *= 2;
		}
		if ( flags & ACCURACY_MULTIPLY_BY_14 )
			iDistance *= 1.4;
		if ( flags & ACCURACY_MULTIPLY_BY_14_2 )
			iDistance *= 1.4;
	}

	if ( m_iAmmoLastCheck >= g_iShotsFired )
	{
		m_flCrosshairDistance -= ( m_flCrosshairDistance * 0.013 + 0.1 );
		m_iAlpha += 2;
	}
	else
	{
		m_flCrosshairDistance = min( m_flCrosshairDistance + iDeltaDistance, 15.0f );
		m_iAlpha = max( m_iAlpha - 40, 120 );
	}

	if ( g_iShotsFired > 600 )
		g_iShotsFired = 1;

	CalcCrosshairColor();
	CalcCrosshairDrawMode();
	CalculateCrosshairSize();

	m_iAmmoLastCheck = g_iShotsFired;
	m_flCrosshairDistance = max( m_flCrosshairDistance, iDistance );
	iLength = ( m_flCrosshairDistance - iDistance ) * 0.5 + 5;

	if ( m_iAlpha > 255 )
		m_iAlpha = 255;

	if ( ScreenWidth != m_iCrosshairScaleBase )
	{
		flCrosshairDistance = ScreenWidth * m_flCrosshairDistance / m_iCrosshairScaleBase;
		iLength = ScreenWidth * iLength / m_iCrosshairScaleBase;
	}
	else
	{
		flCrosshairDistance = m_flCrosshairDistance;
	}

	// drawing
	if ( g_iXash && xhair_enable->value )
	{
		DrawCrosshair( weaponid );
		return;
	}

	if ( m_bAdditive )
	{
		FillRGBA( WEST_XPOS, EAST_WEST_YPOS, iLength, 1, m_R, m_G, m_B, m_iAlpha );
		FillRGBA( EAST_XPOS, EAST_WEST_YPOS, iLength, 1, m_R, m_G, m_B, m_iAlpha );
		FillRGBA( NORTH_SOUTH_XPOS, NORTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha );
		FillRGBA( NORTH_SOUTH_XPOS, SOUTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha );
	}
	else
	{
		FillRGBABlend( WEST_XPOS, EAST_WEST_YPOS, iLength, 1, m_R, m_G, m_B, m_iAlpha );
		FillRGBABlend( EAST_XPOS, EAST_WEST_YPOS, iLength, 1, m_R, m_G, m_B, m_iAlpha );
		FillRGBABlend( NORTH_SOUTH_XPOS, NORTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha );
		FillRGBABlend( NORTH_SOUTH_XPOS, SOUTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha );
	}
}

void CHudAmmo::CalculateCrosshairSize()
{
	int size;

	size = strtol( m_pClCrosshairSize->string, NULL, 10 );

	if( size > 3 )
	{
		size = -1;
	}
	else if( size == 0 && strcmp( "0", m_pClCrosshairSize->string ))
	{
		size = -1;
	}
	
	if( !stricmp( m_pClCrosshairSize->string, "auto" ))
	{
		size = 0;
	}
	else if( !stricmp( m_pClCrosshairSize->string, "small" ))
	{
		size = 1;
	}
	else if( !stricmp( m_pClCrosshairSize->string, "medium" ))
	{
		size = 2;
	}
	else if( !stricmp( m_pClCrosshairSize->string, "large" ))
	{
		size = 3;
	}

	switch( size )
	{
	case -1:
		gEngfuncs.Con_Printf( "usage: cl_crosshair_size <auto|small|medium|large>\n" );
        break;
	case 0:
		if( gHUD.m_scrinfo.iWidth > 640 )
		{
			if( gHUD.m_scrinfo.iWidth < 1024 )
				m_iCrosshairScaleBase = 800;
			else
				m_iCrosshairScaleBase = 640;
		}
		break;
	case 1:
		m_iCrosshairScaleBase = 1024;
		break;
	case 2:
		m_iCrosshairScaleBase = 800;
		break;
	case 3:
	default:
		m_iCrosshairScaleBase = 640;
		break;
	}
}

void CHudAmmo::CalcCrosshairDrawMode()
{
	static float prevDrawMode = -1;
	float drawMode = m_pClCrosshairTranslucent->value;
	
	if( gHUD.m_NVG.m_iFlags )
	{
		m_bAdditive = 0;
		return;
	}

	if( drawMode == prevDrawMode )
		return;
	
	if ( drawMode == 0.0f )
	{
		m_bAdditive = 0;
	}
	else if ( drawMode == 1.0f )
	{
		m_bAdditive = 1;
	}
	else
	{
		gEngfuncs.Con_Printf("usage: cl_crosshair_translucent <1|0>\n");
		gEngfuncs.Cvar_Set("cl_crosshair_translucent", "1");
	}
	
	prevDrawMode = drawMode;
}

void CHudAmmo::CalcCrosshairColor()
{
	static char prevColors[64] = { 0 };
	const char *colors = m_pClCrosshairColor->string;

	if( gHUD.m_NVG.m_iFlags )
	{
		m_R = 250;
		m_G = 50;
		m_B = 50;
		return;
	}

	if( strncmp( prevColors, colors, 64 ) )
	{
		strncpy( prevColors, colors, 64 );
		prevColors[63] = 0;
	
		sscanf( colors, "%d %d %d", &m_cvarR, &m_cvarG, &m_cvarB);

		m_R = m_cvarR = bound( 0, m_cvarR, 255 );
		m_G = m_cvarG = bound( 0, m_cvarG, 255 );
		m_B = m_cvarB = bound( 0, m_cvarB, 255 );
	}
	else
	{
		m_R = m_cvarR;
		m_G = m_cvarG;
		m_B = m_cvarB;
	}
}

//
// Draws the ammo bar on the hud
//
int DrawBar(int x, int y, int width, int height, float f)
{
	int r, g, b;

	f = bound( 0, f, 1 );
	
	if (f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		DrawUtils::UnpackRGB(r, g, b, RGB_GREENISH);
		FillRGBA(x, y, w, height, r, g, b, 255);
		x += w;
		width -= w;
	}

	DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	FillRGBA(x, y, width, height, r, g, b, 128);

	return (x + width);
}



void DrawAmmoBar(WEAPON *p, int x, int y, int width, int height)
{
	if ( !p )
		return;
	
	if (p->iAmmoType != -1)
	{
		if (!gWR.CountAmmo(p->iAmmoType))
			return;

		float f = (float)gWR.CountAmmo(p->iAmmoType)/(float)p->iMax1;
		
		x = DrawBar(x, y, width, height, f);


		// Do we have secondary ammo too?

		if (p->iAmmo2Type != -1)
		{
			f = (float)gWR.CountAmmo(p->iAmmo2Type)/(float)p->iMax2;

			x += 5; //!!!

			DrawBar(x, y, width, height, f);
		}
	}
}




//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList(float flTime)
{
	int r,g,b,x,y,a,i;

	if ( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if ( gpActiveSel == (WEAPON *)1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->iSlot;

	x = gHUD.m_Radar.m_hRadar.rect.right + 10; //!!!
	y = 10; //!!!
	

	// Ensure that there are available choices in the active slot
	if ( iActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( iActiveSlot ) )
		{
			gpActiveSel = (WEAPON *)1;
			iActiveSlot = -1;
		}
	}
		
	// Draw top line
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int iWidth;

		DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
	
		if ( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		DrawUtils::ScaleColors(r, g, b, 255);
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b );

		// make active slot wide enough to accommodate gun pictures
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos(iActiveSlot);
			if ( p )
				iWidth = p->rcActive.Width();
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket0 + i));
		
		x += iWidth + 5;
	}


	a = 128; //!!!
	x = gHUD.m_Radar.m_hRadar.rect.right + 10; //!!!;

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;
			if ( p )
				iWidth = p->rcActive.Width();

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if ( !p || !p->iId )
					continue;

			
				// if active, then we must have ammo.
				if ( gWR.HasAmmo(p) )
				{
					DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
					DrawUtils::ScaleColors(r, g, b, 192);
				}
				else
				{
					DrawUtils::UnpackRGB(r,g,b, RGB_REDISH);
					DrawUtils::ScaleColors(r, g, b, 128);
				}


				if ( gpActiveSel == p )
				{
					SPR_Set(p->hActive, r, g, b );
					SPR_DrawAdditive(0, x, y, &p->rcActive);

					SPR_Set(gHUD.GetSprite(m_HUD_selection), r, g, b );
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					// Draw Weapon if Red if no ammo
					SPR_Set( p->hInactive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &p->rcInactive );
				}

				// Draw Ammo Bar

				DrawAmmoBar(p, x + giABWidth/2, y, giABWidth, giABHeight);
				
				y += p->rcActive.Height() + 5;
			}

			x += iWidth + 5;

		}
		else
		{
			// Draw Row of weapons.

			DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );
				
				if ( !p || !p->iId )
					continue;

				if ( gWR.HasAmmo(p) )
				{
					DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
					a = 128;
				}
				else
				{
					DrawUtils::UnpackRGB(r,g,b, RGB_REDISH);
					a = 96;
				}

				FillRGBA( x, y, giBucketWidth, giBucketHeight, r, g, b, a );

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}	

	return 1;

}


/*
=================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList
=================================
*/
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount)
{
	if (!pList)
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;

	while(i--)
	{
		if ((!strcmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return NULL;
}

/*
=================
SetCrosshair

=================
*/
void CHudAmmo::SetCrosshair(HSPRITE hSpr, wrect_t rect, int r, int g, int b)
{
	m_hStaticSpr = hSpr;
	m_rcStaticRc = rect;
	m_staticRgba.r = r;
	m_staticRgba.g = g;
	m_staticRgba.b = b;
	m_staticRgba.a = 255;
}

void CHudAmmo::HideCrosshair()
{
	m_hStaticSpr = 0;
}

