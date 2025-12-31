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
#include "wl_iwad.h"
#include "wl_net.h"
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

#ifdef __ANDROID__
extern bool ShadowingEnabled;
#endif

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
	IN_Ack(ACK_Block);
}

//===========================================================================


static void GiveAllWeaponsAndAmmo(player_t &player)
{
	// Give Weapons and Max out ammo
	const ClassDef *bestWeapon = NULL;
	int bestWeaponOrder = player.ReadyWeapon ? player.ReadyWeapon->GetClass()->Meta.GetMetaInt(AWMETA_SelectionOrder) : INT_MAX;

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

			if(!inv->CallTryPickup(player.mo))
				inv->Destroy();
		}
	}

	// Switch to best weapon
	if(bestWeapon)
	{
		AWeapon *weapon = static_cast<AWeapon *>(player.mo->FindInventory(bestWeapon));
		if(weapon)
			player.PendingWeapon = weapon;
	}
	else
		player.PendingWeapon = WP_NOCHANGE;
}

/*
================
=
= DebugKeys
=
================
*/

static int DebugKeys (void)
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
		IN_Ack (ACK_Block);
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
		IN_Ack(ACK_Block);
		fpscounter ^= 1;
		return 1;
	}
	if (Keyboard[sc_E])             // E = quit level
	{
		IN_ClearKeysDown();
		if(Net::IsArbiter())
		{
			DebugCmd cmd = {DEBUG_NextLevel};
			Net::DebugKey(cmd);
		}
		return 0;
	}

	if (Keyboard[sc_F])             // F = facing spot
	{
		FString position;
		position.Format("X: %d\nY: %d\nA: %d",
			players[ConsolePlayer].mo->x >> 10,
			players[ConsolePlayer].mo->y >> 10,
			players[ConsolePlayer].mo->angle/ANGLE_1
		);
		US_CenterWindow (14,6);
		US_PrintCentered(position);
		VW_UpdateScreen();
		IN_Ack(ACK_Block);
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
		IN_Ack(ACK_Block);

		DebugCmd cmd = {DEBUG_GodMode};
		cmd.ArgI = (godmode+1)%3;
		Net::DebugKey(cmd);
		return 1;
	}
	if (Keyboard[sc_H])             // H = hurt self
	{
		IN_ClearKeysDown ();

		DebugCmd cmd = {DEBUG_HurtSelf};
		Net::DebugKey(cmd);
	}
	else if (Keyboard[sc_I])        // I = item cheat
	{
		US_CenterWindow (12,3);
		US_PrintCentered ("Free items!");
		VW_UpdateScreen();
		IN_Ack (ACK_Block);

		DebugCmd cmd = {DEBUG_GiveItems};
		Net::DebugKey(cmd);
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
			DebugCmd cmd = {DEBUG_GiveKey};
			cmd.ArgI = atoi (str);
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
		IN_Ack(ACK_Block);

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
		IN_Ack (ACK_Block);
		return 1;
	}
	else if (Keyboard[sc_N])        // N = no clip
	{
		US_CenterWindow (18,3);
		if (noclip)
			US_PrintCentered ("No clipping ON");
		else
			US_PrintCentered ("No clipping OFF");
		VW_UpdateScreen();
		IN_Ack (ACK_Block);

		DebugCmd cmd = {DEBUG_NoClip};
		Net::DebugKey(cmd);
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
		IN_Ack (ACK_Block);
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
		IN_Ack (ACK_Block);
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
		if(!Net::IsArbiter())
			return 0;

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

			DebugCmd cmd = {DEBUG_Warp};
			cmd.ArgS = str;
			Net::DebugKey(cmd);
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
			FName clsName(str, true);
			if(clsName == NAME_None)
				return 1;

			DebugCmd cmd = {summon ? DEBUG_Summon : DEBUG_Give};
			cmd.ArgS = clsName;
			Net::DebugKey(cmd);
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
			IN_Ack (ACK_Block);
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
		IN_Ack (ACK_Block);
		return 1;
	}

	return 0;
}

static void GiveMLI(player_t &player)
{
	player.health = 100;
	player.score = 0;
	gamestate.TimeCount += 42000L;
	GiveAllWeaponsAndAmmo(player);
	P_GiveKeys(player.mo, 101);
	DrawPlayScreen();
}

static void DebugMLI()
{
	DebugCmd cmd = {DEBUG_MLI};
	Net::DebugKey(cmd);

	ClearSplitVWB ();

	Message (language["STR_CHEATER"]);

	IN_ClearKeysDown ();
	IN_Ack (ACK_Block);

	DrawPlayScreen();
}

static void DebugGod(bool noah)
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

	DebugCmd cmd = {DEBUG_GodMode};
	cmd.ArgI = !godmode;
	Net::DebugKey(cmd);

	IN_ClearKeysDown ();
	IN_Ack (ACK_Block);

	if (noah)
	{
		DebugCmd cmd2 = {DEBUG_MLI};
		Net::DebugKey(cmd2);
	}

	if (viewsize < 18)
		StatusBar->RefreshBackground ();
}

/*
================
=
= CheckDebugKeys
=
================
*/

void CheckDebugKeys()
{
	static bool DebugOk = false;

	if (screenfaded || demoplayback) // don't do anything with a faded screen
		return;

	if(IWad::CheckGameFilter(NAME_Wolf3D))
	{
		//
		// SECRET CHEAT CODE: TAB-G-F10
		//
		if (Keyboard[sc_Tab] && Keyboard[sc_G] && Keyboard[sc_F10])
		{
			DebugGod(false);
			return;
		}

		//
		// SECRET CHEAT CODE: 'MLI'
		//
		if (Keyboard[sc_M] && Keyboard[sc_L] && Keyboard[sc_I])
			DebugMLI();

		//
		// TRYING THE KEEN CHEAT CODE!
		//
		if (Keyboard[sc_B] && Keyboard[sc_A] && Keyboard[sc_T])
		{
			ClearSplitVWB ();

			Message ("Commander Keen is also\n"
					"available from Apogee, but\n"
					"then, you already know\n" "that - right, Cheatmeister?!");

			IN_ClearKeysDown ();
			IN_Ack (ACK_Block);

			if (viewsize < 18)
				StatusBar->RefreshBackground ();
		}
	}
	else if(IWad::CheckGameFilter(NAME_Noah))
	{
		//
		// Secret cheat code: JIM
		//
		if (Keyboard[sc_J] && Keyboard[sc_I] && Keyboard[sc_M])
		{
			DebugGod(true);
		}
	}

	//
	// OPEN UP DEBUG KEYS
	//
	if (Keyboard[sc_BackSpace] && Keyboard[sc_LShift] && Keyboard[sc_Alt])
	{
		ClearSplitVWB ();

		Message ("Debugging keys are\nnow available!");
		IN_ClearKeysDown ();
		IN_Ack (ACK_Block);

		DrawPlayBorderSides ();
		DebugOk = 1;
	}

#ifdef __ANDROID__
	if(ShadowingEnabled)
		DebugOk = 1;
#endif

	//
	// TAB-? debug keys
	//
	if(DebugOk)
	{
		// Jam debug sequence if we're trying to open the automap
		// We really only need to check for the automap control since it's
		// likely to be put in the Tab space and be tapped while using other controls
		bool keyDown = Keyboard[sc_Tab] || Keyboard[sc_BackSpace] || Keyboard[sc_Grave];
		if ((schemeAutomapKey.keyboard == sc_Tab || schemeAutomapKey.keyboard == sc_BackSpace || schemeAutomapKey.keyboard == sc_Grave)
			&& (control[ConsolePlayer].buttonstate[bt_automap] || control[ConsolePlayer].buttonheld[bt_automap]))
			keyDown = false;

#ifdef __ANDROID__
		// Soft keyboard
		if (ShadowingEnabled)
			keyDown = true;
#endif

		if (keyDown)
		{
			if (DebugKeys ())
			{
				if (viewsize < 20)
					StatusBar->RefreshBackground ();       // dont let the blue borders flash

				if (MousePresent && IN_IsInputGrabbed())
					IN_CenterMouse();     // Clear accumulated mouse movement

				ResetTimeCount();
			}
		}
	}
}


/*
================
=
= DoDebugKey
=
================
*/

void DoDebugKey(int player, const DebugCmd &cmd)
{
	switch(cmd.Type)
	{
		case DEBUG_Give:
			if(const ClassDef *cls = ClassDef::FindClass(FName(cmd.ArgS, true)))
			{
				if(!cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
					return;

				players[player].mo->GiveInventory(cls, 0, false);
			}
			break;

		case DEBUG_GiveItems:
			GiveAllWeaponsAndAmmo(players[player]);
			players[player].GivePoints(100000);
			players[player].health = 100;
			StatusBar->DrawStatusBar();
			break;

		case DEBUG_GiveKey:
			P_GiveKeys(players[player].mo, cmd.ArgI);
			break;

		case DEBUG_GodMode:
			godmode = cmd.ArgI;
			break;

		case DEBUG_HurtSelf:
			players[player].TakeDamage(16,NULL);
			break;

		case DEBUG_MLI:
			GiveMLI(players[player]);
			break;

		case DEBUG_NextLevel:
			playstate = ex_completed;
			break;

		case DEBUG_NoClip:
			noclip^=1;
			break;

		case DEBUG_Summon:
			if(const ClassDef *cls = ClassDef::FindClass(FName(cmd.ArgS, true)))
			{
				fixed distance = FixedMul(cls->GetDefault()->radius + players[player].mo->radius, 0x16A0A); // sqrt(2)
				AActor *newobj = AActor::Spawn(cls,
					players[player].mo->x + FixedMul(distance, finecosine[players[player].mo->angle>>ANGLETOFINESHIFT]),
					players[player].mo->y - FixedMul(distance, finesine[players[player].mo->angle>>ANGLETOFINESHIFT]),
					0, 0);
				newobj->angle = players[player].mo->angle;
			}
			break;

		case DEBUG_Warp:
			if(GameMap::CheckMapExists(cmd.ArgS))
			{
				strncpy(gamestate.mapname, cmd.ArgS, 8);
				gamestate.mapname[8] = 0;
				playstate = ex_warped;
			}
			break;
	}
}
