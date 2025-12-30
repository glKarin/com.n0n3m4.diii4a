// WL_DEBUG.C

#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif
#include <climits>

#include "wl_def.h"
#include "wl_menu.h"
#include "am_map.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "g_mapinfo.h"
#include "actor.h"
#include "language.h"
#include "m_png.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_debug.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_play.h"
#include "w_wad.h"
#include "thingdef/thingdef.h"
#include "g_shared/a_keys.h"
#include "r_sprites.h"
#include "wl_shade.h"
#include "filesys.h"

#ifdef USE_CLOUDSKY
#include "wl_cloudsky.h"
#endif

/*
=============================================================================

												LOCAL CONSTANTS

=============================================================================
*/

#define VIEWTILEX       (viewwidth/16)
#define VIEWTILEY       (viewheight/16)

/*
=============================================================================

												GLOBAL VARIABLES

=============================================================================
*/


int DebugKeys (void);


// from WL_DRAW.C

void ScalePost();

/*
=============================================================================

												LOCAL VARIABLES

=============================================================================
*/

int     maporgx;
int     maporgy;
enum ViewType {mapview,tilemapview,actoratview,visview};
ViewType viewtype;

void ViewMap (void);

//===========================================================================

/*
===================
=
= PictureGrabber
=
===================
*/
void PictureGrabber (void)
{
	static char fname[] = "WSHOT000.PNG";
	FString screenshotDir = FileSys::GetDirectoryPath(FileSys::DIR_Screenshots);

	for(int i = 0; i < 1000; i++)
	{
		fname[7] = i % 10 + '0';
		fname[6] = (i / 10) % 10 + '0';
		fname[5] = i / 100 + '0';
		if(!File(screenshotDir + PATH_SEPARATOR + fname).exists())
			break;
	}

	// overwrites WSHOT999.PNG if all wshot files exist
	const BYTE* buffer;
	int pitch;
	ESSType color_type;
	screen->GetScreenshotBuffer(buffer, pitch, color_type);
	FILE *file = File(screenshotDir + PATH_SEPARATOR + fname).open("wb");
	M_CreatePNG(file, buffer, GPalette.BaseColors, color_type, SCREENWIDTH, SCREENHEIGHT, pitch);
	M_FinishPNG(file);
	fclose(file);
	screen->ReleaseScreenshotBuffer();

	US_CenterWindow (18,2);
	US_PrintCentered ("Screenshot taken");
	VW_UpdateScreen();
	IN_Ack();
}

//===========================================================================


static void GiveAllWeaponsAndAmmo()
{
	// Give Weapons and Max out ammo
	const ClassDef *bestWeapon = NULL;
	int bestWeaponOrder = players[0].ReadyWeapon ? players[0].ReadyWeapon->GetClass()->Meta.GetMetaInt(AWMETA_SelectionOrder) : INT_MAX;

	ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
	ClassDef::ClassPair *pair;
	while(iter.NextPair(pair))
	{
		const ClassDef *cls = pair->Value;
		AInventory *inv = NULL;
		// Don't give replaced weapons or ammo sub-classes
		if((cls->IsDescendantOf(NATIVE_CLASS(Weapon)) && cls != NATIVE_CLASS(Weapon) && cls->GetReplacement() == cls) ||
			(cls->GetParent() == NATIVE_CLASS(Ammo))
		)
		{
			inv = (AInventory *) AActor::Spawn(cls, 0, 0, 0, 0);
			inv->RemoveFromWorld();
			const Frame * const readyState = cls->FindState(NAME_Ready);
			if(cls->GetParent() == NATIVE_CLASS(Ammo))
				inv->amount = inv->maxamount;
			else if(!readyState || !R_CheckSpriteValid(readyState->spriteInf))
			{ // Only give valid weapons
				inv->Destroy();
				continue;
			}
			else // Should be a weapon that we're giving
			{
				int selectionOrder = cls->Meta.GetMetaInt(AWMETA_SelectionOrder);
				if(selectionOrder < bestWeaponOrder)
				{
					bestWeapon = cls;
					bestWeaponOrder = selectionOrder;
				}
			}

			if(!inv->CallTryPickup(players[0].mo))
				inv->Destroy();
		}
	}

	// Switch to best weapon
	if(bestWeapon)
	{
		AWeapon *weapon = static_cast<AWeapon *>(players[0].mo->FindInventory(bestWeapon));
		if(weapon)
			players[0].PendingWeapon = weapon;
	}
	else
		players[0].PendingWeapon = WP_NOCHANGE;
}

/*
================
=
= DebugKeys
=
================
*/

int DebugKeys (void)
{
	bool esc;
	int level;
	char str[80];

	if (Keyboard[sc_B])             // B = border color
	{
		US_CenterWindow(22,3);
		PrintY+=6;
		US_Print(SmallFont, " Border texture: ");
		VW_UpdateScreen();
		esc = !US_LineInput (SmallFont,PrintX,py,str,NULL,true,8,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			FTextureID texID = TexMan.CheckForTexture(str, FTexture::TEX_Any);
			if (texID.isValid())
			{
				levelInfo->BorderTexture = texID;
				StatusBar->RefreshBackground();

				return 0;
			}
		}
		return 1;
	}
	if (Keyboard[sc_C])             // C = count objects
	{
		US_CenterWindow (17,4);

		FString actorCount;
		actorCount.Format("\nTotal actors : %d", AActor::actors.Size());

		US_Print (SmallFont, actorCount);

		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	if (Keyboard[sc_D])             // D = Darkone's FPS counter
	{
		US_CenterWindow (22,2);
		if (fpscounter)
			US_PrintCentered ("Darkone's FPS Counter OFF");
		else
			US_PrintCentered ("Darkone's FPS Counter ON");
		VW_UpdateScreen();
		IN_Ack();
		fpscounter ^= 1;
		return 1;
	}
	if (Keyboard[sc_E])             // E = quit level
	{
		playstate = ex_completed;
		IN_ClearKeysDown();
	}

	if (Keyboard[sc_F])             // F = facing spot
	{
		FString position;
		position.Format("X: %d\nY: %d\nA: %d",
			players[0].mo->x >> 10,
			players[0].mo->y >> 10,
			players[0].mo->angle/ANGLE_1
		);
		US_CenterWindow (14,6);
		US_PrintCentered(position);
		VW_UpdateScreen();
		IN_Ack();
		return 1;
	}

	if (Keyboard[sc_G])             // G = god mode
	{
		US_CenterWindow (12,2);
		if (godmode == 0)
			US_PrintCentered ("God mode ON");
		else if (godmode == 1)
			US_PrintCentered ("God (no flash)");
		else if (godmode == 2)
			US_PrintCentered ("God mode OFF");

		VW_UpdateScreen();
		IN_Ack();
		if (godmode != 2)
			godmode++;
		else
			godmode = 0;
		return 1;
	}
	if (Keyboard[sc_H])             // H = hurt self
	{
		IN_ClearKeysDown ();
		players[0].TakeDamage (16,NULL);
	}
	else if (Keyboard[sc_I])        // I = item cheat
	{
		US_CenterWindow (12,3);
		US_PrintCentered ("Free items!");
		VW_UpdateScreen();
		GiveAllWeaponsAndAmmo();
		players[0].GivePoints (100000);
		players[0].health = 100;
		StatusBar->DrawStatusBar();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_K])        // K = give keys
	{
		US_CenterWindow(16,3);
		PrintY+=6;
		US_Print(SmallFont, "  Give Key (#): ");
		VW_UpdateScreen();
		esc = !US_LineInput (SmallFont,PrintX,py,str,NULL,true,3,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			level = atoi (str);
			P_GiveKeys(players[0].mo, level);
		}
		return 1;
	}
	else if (Keyboard[sc_L])        // L = level ratios
	{
		int ak = 0, as = 0, at = 0, kr = 100, sr = 100, tr = 100;
		if(LevelRatios.numLevels)
		{
			ak = LevelRatios.killratio / LevelRatios.numLevels;
			as = LevelRatios.secretsratio / LevelRatios.numLevels;
			at = LevelRatios.treasureratio / LevelRatios.numLevels;
		}
		if(gamestate.killtotal)
			kr = gamestate.killcount*100/gamestate.killtotal;
		if(gamestate.secrettotal)
			sr = gamestate.secretcount*100/gamestate.secrettotal;
		if(gamestate.treasuretotal)
			tr = gamestate.treasurecount*100/gamestate.treasuretotal;
		FString ratios;
		ratios.Format(
			"Current Level: %02d:%02d\nKills: %d%%\nSecrets: %d%%\nTreasure: %d%%\n\n"
			"Averages: %02d:%02d\nKills: %d%%\nSecrets: %d%%\nTreasure: %d%%",
			gamestate.TimeCount/4200, (gamestate.TimeCount/70)%60,
			kr, sr, tr,
			LevelRatios.time/60, LevelRatios.time%60,
			ak, as, at
 		);
		US_CenterWindow(17, 12);
		US_PrintCentered(ratios);
		VW_UpdateScreen();
		IN_Ack();

		return 1;
	}
	else if (Keyboard[sc_M]) // M = Mouse look
	{
		mouselook ^= 1;
		US_CenterWindow (17,3);
		if (mouselook)
			US_PrintCentered ("Mouse look ON");
		else
			US_PrintCentered ("Mouse look OFF");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_N])        // N = no clip
	{
		noclip^=1;
		US_CenterWindow (18,3);
		if (noclip)
			US_PrintCentered ("No clipping ON");
		else
			US_PrintCentered ("No clipping OFF");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_O])
	{
		am_cheat ^= 1;
		US_CenterWindow (18,3);
		if (am_cheat)
			US_PrintCentered ("Automap revealed");
		else
			US_PrintCentered ("Automap hidden");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	else if(Keyboard[sc_P])         // P = Ripper's picture grabber
	{
		PictureGrabber();
		return 1;
	}
	else if (Keyboard[sc_Q])        // Q = fast quit
		Quit ();
	else if (Keyboard[sc_S])        // S = slow motion
	{
		US_CenterWindow(30,3);
		PrintY+=6;
		US_Print(SmallFont, " Slow Motion steps (default 14): ");
		VW_UpdateScreen();
		esc = !US_LineInput (SmallFont,PrintX,py,str,NULL,true,2,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			level = atoi (str);
			if (level>=0 && level<=50)
				singlestep = level;
		}
		return 1;
	}
	else if (Keyboard[sc_T])
	{
		notargetmode = !notargetmode;
		US_CenterWindow (20,3);
		if(notargetmode)
			US_PrintCentered("No target mode ON");
		else
			US_PrintCentered("No target mode OFF");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_V])        // V = extra VBLs
	{
		US_CenterWindow(30,3);
		PrintY+=6;
		US_Print(SmallFont, "  Add how many extra VBLs(0-8): ");
		VW_UpdateScreen();
		esc = !US_LineInput (SmallFont,PrintX,py,str,NULL,true,1,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			level = atoi (str);
			if (level>=0 && level<=8)
				extravbls = level;
		}
		return 1;
	}
	else if (Keyboard[sc_W])        // W = warp to level
	{
		US_CenterWindow(26,3);
		PrintY+=6;
		US_Print(SmallFont, "  Warp to which level: ");
		VW_UpdateScreen();
		esc = !US_LineInput (SmallFont,PrintX,py,str,NULL,true,8,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc && str[0])
		{
			// Check if a number was provided
			bool isNumber = true;
			for(size_t i = strlen(str);i-- > 0;)
			{
				if(str[i] < '0' || str[i] > '9')
				{
					isNumber = false;
					break;
				}
			}
			if(isNumber)
			{
				int num = atoi(str);
				LevelInfo &info = LevelInfo::FindByNumber(num);
				if(info.MapName[0])
					strcpy(str, info.MapName);
			}

			if(GameMap::CheckMapExists(str))
			{
				strncpy(gamestate.mapname, str, 8);
				gamestate.mapname[8] = 0;
				playstate = ex_warped;
			}
		}
		return 1;
	}
	else if (Keyboard[sc_X] || Keyboard[sc_Z])        // X = item cheat, Z = summon
	{
		bool summon = Keyboard[sc_Z];
		US_CenterWindow (22,3);
		PrintY += 6;
		US_Print (SmallFont, summon ? "Summon: " : "Give: ");
		VW_UpdateScreen();
		esc = !US_LineInput (SmallFont,PrintX,py,str,NULL,true,summon ? 20 : 22,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			const ClassDef *cls = ClassDef::FindClass(str);
			if(summon && cls)
			{
				fixed distance = FixedMul(cls->GetDefault()->radius + players[0].mo->radius, 0x16A0A); // sqrt(2)
				AActor *newobj = AActor::Spawn(cls,
					players[0].mo->x + FixedMul(distance, finecosine[players[0].mo->angle>>ANGLETOFINESHIFT]),
					players[0].mo->y - FixedMul(distance, finesine[players[0].mo->angle>>ANGLETOFINESHIFT]),
					0, 0);
				newobj->angle = players[0].mo->angle;
			}
			else
			{
				if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
					return 1;

				players[0].mo->GiveInventory(cls, 0, false);
			}
		}
		return 1;
	}
#ifdef USE_CLOUDSKY
	else if(Keyboard[sc_Z])
	{
		char defstr[15];

		US_CenterWindow(34,4);
		PrintY+=6;
		US_Print(SmallFont, "  Recalculate sky with seek: ");
		int seekpx = px, seekpy = py;
		US_PrintUnsigned(curSky->seed);
		US_Print(SmallFont, "\n  Use color map (0-");
		US_PrintUnsigned(numColorMaps - 1);
		US_Print(SmallFont, "): ");
		int mappx = px, mappy = py;
		US_PrintUnsigned(curSky->colorMapIndex);
		VW_UpdateScreen();

		sprintf(defstr, "%u", curSky->seed);
		esc = !US_LineInput(SmallFont,seekpx, seekpy, str, defstr, true, 10, 0,GPalette.WhiteIndex);
		if(esc) return 0;
		curSky->seed = (uint32_t) atoi(str);

		sprintf(defstr, "%u", curSky->colorMapIndex);
		esc = !US_LineInput(SmallFont,mappx, mappy, str, defstr, true, 10, 0,GPalette.WhiteIndex);
		if(esc) return 0;
		uint32_t newInd = (uint32_t) atoi(str);
		if(newInd < (uint32_t) numColorMaps)
		{
			curSky->colorMapIndex = newInd;
			InitSky();
		}
		else
		{
			US_CenterWindow (18,3);
			US_PrintCentered ("Illegal color map!");
			VW_UpdateScreen();
			IN_Ack ();
		}
	}
#endif
	else if(Keyboard[sc_Comma])
	{
		gLevelLight = MAX(1, gLevelLight-1);
		Printf("Light = %d\n", gLevelLight);
	}
	else if(Keyboard[sc_Peroid])
	{
		gLevelLight = MIN(256, gLevelLight+1);
		Printf("Light = %d\n", gLevelLight);
	}
	else if(Keyboard[sc_Y])
	{
		gLevelVisibility = MAX(1, gLevelVisibility-20000);
		Printf("Vis = %d\n", gLevelVisibility);
		CalcVisibility(gLevelVisibility);
	}
	else if(Keyboard[sc_U])
	{
		gLevelVisibility = MIN(200<<FRACBITS, gLevelVisibility+20000);
		Printf("Vis = %d\n", gLevelVisibility);
		CalcVisibility(gLevelVisibility);
	}
	else if(Keyboard[sc_3])
	{
		UseWolf4SDL3DSpriteScaler = !UseWolf4SDL3DSpriteScaler;
		US_CenterWindow (20,3);
		if(UseWolf4SDL3DSpriteScaler)
			US_PrintCentered("3D Sprite scaler: 4SDL");
		else
			US_PrintCentered("3D Sprite scaler: ECWolf");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}

	return 0;
}

static void GiveMLI()
{
	players[0].health = 100;
	players[0].score = 0;
	gamestate.TimeCount += 42000L;
	GiveAllWeaponsAndAmmo();
	P_GiveKeys(players[0].mo, 101);
	DrawPlayScreen();
}

void DebugMLI()
{
	GiveMLI();

	ClearSplitVWB ();

	Message (language["STR_CHEATER"]);

	IN_ClearKeysDown ();
	IN_Ack ();

	DrawPlayScreen();
}

void DebugGod(bool noah)
{
	WindowH = 160;

	if (noah)
	{
		if (godmode)
		{
			Message ("Invulnerability OFF");
			SD_PlaySound ("misc/no_bonus");
		}
		else
		{
			Message ("Invulnerability ON");
			SD_PlaySound ("misc/1up");
		}
	}
	else
	{
		if (godmode)
		{
			Message ("God mode OFF");
			SD_PlaySound ("misc/no_bonus");
		}
		else
		{
			Message ("God mode ON");
			SD_PlaySound ("misc/end_bonus2");
		}
	}

	godmode ^= 1;

	IN_ClearKeysDown ();
	IN_Ack ();

	if (noah)
	{
		GiveMLI();
	}

	if (viewsize < 18)
		StatusBar->RefreshBackground ();
}
