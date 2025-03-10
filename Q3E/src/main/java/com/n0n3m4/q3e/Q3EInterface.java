/*
 	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;

import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTech4Command;
import com.n0n3m4.q3e.karin.KidTechCommand;
import com.n0n3m4.q3e.karin.KidTechQuakeCommand;

import java.util.Arrays;
import java.util.Set;

public class Q3EInterface
{
	static {
		Q3EKeyCodes.InitD3Keycodes();
		InitDefaultTypeTable();
		InitDefaultArgTable();
	}

	public static int[] _defaultArgs;
	public static int[] _defaultType;

	public int UI_SIZE;
	public String[] defaults_table;
	public String[] texture_table;
	public int[] type_table;
	public int[] arg_table; // slider: key,key,key,style | button: key,canbeheld,style,null

	public boolean isRTCW=false;
	public boolean isQ1=false;
	public boolean isQ2=false;
	public boolean isQ3=false;
	public boolean isD3=false;
	public boolean isD3BFG=false;
    public boolean isQ4 = false;
	public boolean isPrey = false;
	public boolean isTDM = false;
	public boolean isDOOM = false;
	public boolean isETW = false;
	public boolean isRealRTCW = false;
	public boolean isFTEQW = false;
	public boolean isJA = false;
	public boolean isJO = false;

	public boolean isD3BFG_Vulkan = false;

	public String default_path = Environment.getExternalStorageDirectory() + "/diii4a";

	public String libname;
	public String config_name;
	public String game;
	public String game_name;
	public String game_base;
	public String game_version;
	public String datadir;
	public boolean standalone = false;
	public String subdatadir;
	public int game_id;

	public Q3ECallbackObj callbackObj;

    public boolean view_motion_control_gyro = false;
    public String start_temporary_extra_command = "";
	public String cmd = Q3EGlobals.GAME_EXECUABLE;
	public boolean multithread = false;
	public boolean function_key_toolbar = false;
	public float joystick_release_range = 0.0f;
	public float joystick_inner_dead_zone = 0.0f;
	public boolean joystick_unfixed = false;
	public boolean joystick_smooth = true; // Q3EView::analog

	public String app_storage_path = "/sdcard/diii4a";
	
	//RTCW4A:
	/*
	public final int RTCW4A_UI_ACTION=6;
	public final int RTCW4A_UI_KICK=7;
	 */

	//k volume key map
	public int VOLUME_UP_KEY_CODE = Q3EKeyCodes.KeyCodesGeneric.K_F3;
	public int VOLUME_DOWN_KEY_CODE = Q3EKeyCodes.KeyCodesGeneric.K_F2;

	public String EngineLibName()
	{
		if(isPrey)
			return Q3EGlobals.LIB_ENGINE_HUMANHEAD;
		else if(isQ4)
			return Q3EGlobals.LIB_ENGINE_RAVEN;
		else if(isQ2)
			return Q3EGlobals.LIB_ENGINE2_ID;
		else if(isQ3)
			return Q3EGlobals.LIB_ENGINE3_ID;
		else if(isRTCW)
			return Q3EGlobals.LIB_ENGINE3_RTCW;
		else if(isTDM)
			return Q3EGlobals.LIB_ENGINE4_TDM;
		else if(isQ1)
			return Q3EGlobals.LIB_ENGINE1_QUAKE;
		else if(isD3BFG)
			return Q3EGlobals.LIB_ENGINE4_D3BFG;
		else if(isDOOM)
			return Q3EGlobals.LIB_ENGINE1_DOOM;
		else if(isETW)
			return Q3EGlobals.LIB_ENGINE3_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.LIB_ENGINE3_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.LIB_ENGINE_FTEQW;
		else if(isJA)
			return Q3EGlobals.LIB_ENGINE_JA;
		else if(isJO)
			return Q3EGlobals.LIB_ENGINE_JO;
		else
			return Q3EGlobals.LIB_ENGINE_ID;
	}

	public String GetEngineLibName()
	{
		if(null == game_version || game_version.isEmpty())
			return EngineLibName();

		if(isD3BFG)
		{
			if(Q3EGlobals.GAME_VERSION_D3BFG_VULKAN.equalsIgnoreCase(game_version))
				return Q3EGlobals.LIB_ENGINE4_D3BFG_VULKAN;
			else
				return Q3EGlobals.LIB_ENGINE4_D3BFG;
		}
		else if(isRealRTCW)
		{
			if(Q3EGlobals.GAME_VERSION_REALRTCW_5_0.equalsIgnoreCase(game_version))
				return Q3EGlobals.LIB_ENGINE3_REALRTCW_5_0;
			else
				return Q3EGlobals.LIB_ENGINE3_REALRTCW;
		}
		else
			return EngineLibName();
	}

	public String GameVersion(Context context)
	{
		SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
		String key = GetGameVersionPreferenceKey();
		if(null == key)
			return null;
		String str = preferences.getString(key, null);
		if(null == str)
			return null;
		if(str.isEmpty())
			return null;
		return str;
	}

	public String ConfigFileName()
	{
		if(isPrey)
			return Q3EGlobals.CONFIG_FILE_PREY;
		else if(isQ4)
			return Q3EGlobals.CONFIG_FILE_QUAKE4;
		else if(isQ2)
			return Q3EGlobals.CONFIG_FILE_QUAKE2;
		else if(isQ3)
			return Q3EGlobals.CONFIG_FILE_QUAKE3;
		else if(isRTCW)
			return Q3EGlobals.CONFIG_FILE_RTCW;
		else if(isTDM)
			return Q3EGlobals.CONFIG_FILE_TDM;
		else if(isQ1)
			return Q3EGlobals.CONFIG_FILE_QUAKE1;
		else if(isD3BFG)
			return Q3EGlobals.CONFIG_FILE_DOOM3BFG;
		else if(isDOOM)
			return Q3EGlobals.CONFIG_FILE_GZDOOM;
		else if(isETW)
			return Q3EGlobals.CONFIG_FILE_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.CONFIG_FILE_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.CONFIG_FILE_FTEQW;
		else if(isJA)
			return Q3EGlobals.CONFIG_FILE_JA;
		else if(isJO)
			return Q3EGlobals.CONFIG_FILE_JO;
		else
			return Q3EGlobals.CONFIG_FILE_DOOM3;
	}

	public String GameType()
	{
		if(isPrey)
			return Q3EGlobals.GAME_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_QUAKE4;
		else if(isQ2)
			return Q3EGlobals.GAME_QUAKE2;
		else if(isQ3)
			return Q3EGlobals.GAME_QUAKE3;
		else if(isRTCW)
			return Q3EGlobals.GAME_RTCW;
		else if(isTDM)
			return Q3EGlobals.GAME_TDM;
		else if(isQ1)
			return Q3EGlobals.GAME_QUAKE1;
		else if(isD3BFG)
			return Q3EGlobals.GAME_DOOM3BFG;
		else if(isDOOM)
			return Q3EGlobals.GAME_GZDOOM;
		else if(isETW)
			return Q3EGlobals.GAME_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.GAME_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.GAME_FTEQW;
		else if(isJA)
			return Q3EGlobals.GAME_JA;
		else if(isJO)
			return Q3EGlobals.GAME_JO;
		else
			return Q3EGlobals.GAME_DOOM3;
	}

	public String GameName()
	{
		if(isPrey)
			return Q3EGlobals.GAME_NAME_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_NAME_QUAKE4;
		else if(isQ2)
			return Q3EGlobals.GAME_NAME_QUAKE2;
		else if(isQ3)
			return Q3EGlobals.GAME_NAME_QUAKE3;
		else if(isRTCW)
			return Q3EGlobals.GAME_NAME_RTCW;
		else if(isTDM)
			return Q3EGlobals.GAME_NAME_TDM;
		else if(isQ1)
			return Q3EGlobals.GAME_NAME_QUAKE1;
		else if(isD3BFG)
			return Q3EGlobals.GAME_NAME_DOOM3BFG;
		else if(isDOOM)
			return Q3EGlobals.GAME_NAME_GZDOOM;
		else if(isETW)
			return Q3EGlobals.GAME_NAME_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.GAME_NAME_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.GAME_NAME_FTEQW;
		else if(isJA)
			return Q3EGlobals.GAME_NAME_JA;
		else if(isJO)
			return Q3EGlobals.GAME_NAME_JO;
		else
			return Q3EGlobals.GAME_NAME_DOOM3;
	}

	public String GameBase()
	{
		if(isPrey)
			return Q3EGlobals.GAME_BASE_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_BASE_QUAKE4;
		else if(isQ2)
			return Q3EGlobals.GAME_BASE_QUAKE2;
		else if(isQ3)
			return Q3EGlobals.GAME_BASE_QUAKE3;
		else if(isRTCW)
			return Q3EGlobals.GAME_BASE_RTCW;
		else if(isTDM)
			return Q3EGlobals.GAME_BASE_TDM;
		else if(isQ1)
			return Q3EGlobals.GAME_BASE_QUAKE1;
		else if(isD3BFG)
			return Q3EGlobals.GAME_BASE_DOOM3BFG;
		else if(isDOOM)
			return Q3EGlobals.GAME_BASE_GZDOOM;
		else if(isETW)
			return Q3EGlobals.GAME_BASE_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.GAME_BASE_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.GAME_BASE_FTEQW;
		else if(isJA)
			return Q3EGlobals.GAME_BASE_JA;
		else if(isJO)
			return Q3EGlobals.GAME_BASE_JO;
		else
			return Q3EGlobals.GAME_BASE_DOOM3;
	}

	public String GameSubDirectory()
	{
		if(isQ4)
			return Q3EGlobals.GAME_SUBDIR_QUAKE4;
		else if(isPrey)
			return Q3EGlobals.GAME_SUBDIR_PREY;
		else if(isRTCW)
			return Q3EGlobals.GAME_SUBDIR_RTCW;
		else if(isQ3)
			return Q3EGlobals.GAME_SUBDIR_QUAKE3;
		else if(isQ2)
			return Q3EGlobals.GAME_SUBDIR_QUAKE2;
		else if(isQ1)
			return Q3EGlobals.GAME_SUBDIR_QUAKE1;
		else if(isTDM)
			return Q3EGlobals.GAME_SUBDIR_TDM;
		else if(isD3BFG)
			return Q3EGlobals.GAME_SUBDIR_DOOMBFG;
		else if(isDOOM)
			return Q3EGlobals.GAME_SUBDIR_GZDOOM;
		else if(isETW)
			return Q3EGlobals.GAME_SUBDIR_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.GAME_SUBDIR_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.GAME_SUBDIR_FTEQW;
		else if(isJA)
			return Q3EGlobals.GAME_SUBDIR_JA;
		else if(isJO)
			return Q3EGlobals.GAME_SUBDIR_JO;
		else
			return Q3EGlobals.GAME_SUBDIR_DOOM3;
	}

	public int GameID()
	{
		if(isPrey)
			return Q3EGlobals.GAME_ID_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_ID_QUAKE4;
		else if(isQ2)
			return Q3EGlobals.GAME_ID_QUAKE2;
		else if(isQ3)
			return Q3EGlobals.GAME_ID_QUAKE3;
		else if(isRTCW)
			return Q3EGlobals.GAME_ID_RTCW;
		else if(isTDM)
			return Q3EGlobals.GAME_ID_TDM;
		else if(isQ1)
			return Q3EGlobals.GAME_ID_QUAKE1;
		else if(isD3BFG)
			return Q3EGlobals.GAME_ID_DOOM3BFG;
		else if(isDOOM)
			return Q3EGlobals.GAME_ID_GZDOOM;
		else if(isETW)
			return Q3EGlobals.GAME_ID_ETW;
		else if(isRealRTCW)
			return Q3EGlobals.GAME_ID_REALRTCW;
		else if(isFTEQW)
			return Q3EGlobals.GAME_ID_FTEQW;
		else if(isJA)
			return Q3EGlobals.GAME_ID_JA;
		else if(isJO)
			return Q3EGlobals.GAME_ID_JO;
		else
			return Q3EGlobals.GAME_ID_DOOM3;
	}

	public void SetupEngineLib()
	{
		libname = EngineLibName();
	}

	public void SetupKeycodes()
	{
		if(isPrey)
			Q3EKeyCodes.InitPreyKeycodes();
		else if(isQ4)
			Q3EKeyCodes.InitQ4Keycodes();
		else if(isQ2)
			Q3EKeyCodes.InitQ2Keycodes();
		else if(isQ3)
			Q3EKeyCodes.InitQ3Keycodes();
		else if(isRTCW)
			Q3EKeyCodes.InitRTCWKeycodes();
		else if(isTDM)
			Q3EKeyCodes.InitTDMKeycodes();
		else if(isQ1)
			Q3EKeyCodes.InitQ1Keycodes();
		else if(isD3BFG)
			Q3EKeyCodes.InitD3BFGKeycodes();
		else if(isDOOM)
			Q3EKeyCodes.InitSDLKeycodes();
		else if(isETW)
			Q3EKeyCodes.InitETWKeycodes();
		else if(isRealRTCW)
			Q3EKeyCodes.InitRealRTCWKeycodes();
		else if(isFTEQW)
			Q3EKeyCodes.InitFTEQWKeycodes();
		else if(isJA)
			Q3EKeyCodes.InitJKKeycodes();
		else if(isJO)
			Q3EKeyCodes.InitJKKeycodes();
		else
			Q3EKeyCodes.InitD3Keycodes();
	}

	private void SetupConfigFile()
	{
		config_name = ConfigFileName();
	}

	public static String GetStandaloneDirectory(boolean standalone, String game)
	{
		String subdir = GetGameStandaloneDirectory(game);
		if(standalone)
			return subdir;
		else if(IsStandaloneGame(game))
			return subdir;
		else
			return null;
	}

	private void SetupSubDir()
	{
		String subdir = GameSubDirectory();
		if(standalone)
			subdatadir = subdir;
		else if(isTDM
				|| isDOOM
				|| isFTEQW)
			subdatadir = subdir;
		else
			subdatadir = null;
	}

	private void SetupGameTypeAndName()
	{
		game = GameType();
		game_name = GameName();
		game_base = GameBase();
		game_id = GameID();
		game_version = null;
	}

	public void SetupGame(String name)
	{
		Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "SetupGame: " + name);
		if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			SetupPrey();
		else if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			SetupQuake4();
		else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
			SetupQuake2();
		else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
			SetupQuake3();
		else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
			SetupRTCW();
		else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
			SetupTDM();
		else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
			SetupQuake1();
		else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			SetupDoom3BFG();
		else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
			SetupGZDoom();
		else if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
			SetupETW();
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			SetupRealRTCW();
		else if(Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(name))
			SetupFTEQW();
		else if(Q3EGlobals.GAME_JA.equalsIgnoreCase(name))
			SetupJA();
		else if(Q3EGlobals.GAME_JO.equalsIgnoreCase(name))
			SetupJO();
		else
			SetupDOOM3();
	}

	public void SetupGameConfig()
	{
		SetupGameTypeAndName();
		SetupEngineLib();
		SetupConfigFile();
		SetupKeycodes();
		SetupSubDir();
	}

	public void ResetGameState()
	{
		isD3 = false;
		isPrey = false;
		isQ4 = false;
		isTDM = false;
		isQ2 = false;
		isRTCW = false;
		isQ3 = false;
		isQ1 = false;
		isD3BFG = false;
		isDOOM = false;
		isETW = false;
		isRealRTCW = false;
		isFTEQW = false;
		isJA = false;
		isJO = false;
	}

	public void SetupDOOM3()
	{
		ResetGameState();
		isD3 = true;
		SetupGameConfig();
	}

	public void SetupPrey()
	{
		ResetGameState();
		isD3 = true;
		isPrey = true;
		SetupGameConfig();
	}

	public void SetupQuake4()
	{
		ResetGameState();
		isD3 = true;
		isQ4 = true;
		SetupGameConfig();
    }

	public void SetupTDM()
	{
		ResetGameState();
		isTDM = true;
		SetupGameConfig();
	}

	public void SetupQuake2()
	{
		ResetGameState();
		isQ2 = true;
		SetupGameConfig();
	}

	public void SetupRTCW()
	{
		ResetGameState();
		isRTCW = true;
		SetupGameConfig();
	}

	public void SetupQuake3()
	{
		ResetGameState();
		isQ3 = true;
		SetupGameConfig();
	}

	public void SetupQuake1()
	{
		ResetGameState();
		isQ1 = true;
		SetupGameConfig();
	}

	public void SetupDoom3BFG()
	{
		ResetGameState();
		isD3BFG = true;
		SetupGameConfig();
	}

	public void SetupGZDoom()
	{
		ResetGameState();
		isDOOM = true;
		SetupGameConfig();
	}

	public void SetupETW()
	{
		ResetGameState();
		isETW = true;
		SetupGameConfig();
	}

	public void SetupRealRTCW()
	{
		ResetGameState();
		isRealRTCW = true;
		SetupGameConfig();
	}

	public void SetupFTEQW()
	{
		ResetGameState();
		isFTEQW = true;
		SetupGameConfig();
	}

	public void SetupJA()
	{
		ResetGameState();
		isJA = true;
		SetupGameConfig();
	}

	public void SetupJO()
	{
		ResetGameState();
		isJO = true;
		SetupGameConfig();
	}

	public void SetupGameVersion(String version)
	{
		if("".equals(version))
			version = null;
		game_version = version;
	}

	public void SetupGameVersion(Context context)
	{
		SetupGameVersion(GameVersion(context));
	}

	public boolean IsTDMTech()
	{
		return isTDM;
	}

	public boolean IsIdTech4()
	{
		return isD3 || isQ4 || isPrey;
	}

	public boolean IsIdTech3()
	{
		return isQ3 || isRTCW || isETW || isRealRTCW || isJA || isJO;
	}

	public boolean IsIdTech2()
	{
		return isQ2;
	}

	public boolean IsIdQuakeTech()
	{
		return isQ1;
	}

	public boolean IsIdTech4BFG() // 4.5
	{
		return isD3BFG;
	}

	public boolean IsIdTech1()
	{
		return isDOOM;
	}

	public String GetGameCommandParm()
	{
		if(isPrey)
			return "fs_game";
		else if(isQ4)
			return "fs_game";
		else if(isQ2)
			return "game";
		else if(isQ3)
			return "fs_game";
		else if(isRTCW)
			return "fs_game";
		else if(isTDM)
			return "fs_currentfm"; // fs_mod
		else if(isQ1)
			return "game";
		else if(isD3BFG)
			return "fs_game";
		else if(isDOOM)
			return "iwad";
		else if(isETW)
			return "fs_game";
		else if(isRealRTCW)
			return "fs_game";
		else if(isFTEQW)
			return ""; // game
		else if(isJA)
			return "fs_game";
		else if(isJO)
			return "fs_game";
		else
			return "fs_game";
	}

	public String GetSecondaryGameCommandParm()
	{
		if(isPrey)
			return "fs_game_base";
		else if(isQ4)
			return "fs_game_base";
		else if(isFTEQW)
			return "game";
		else
			return "fs_game_base";
	}

	public String GetGameCommandPrefix()
	{
		if(isQ1 || isFTEQW)
			return KidTechCommand.ARG_PREFIX_QUAKETECH;
		if(isDOOM)
			return KidTechCommand.ARG_PREFIX_QUAKETECH + KidTechCommand.ARG_PREFIX_IDTECH;
		else
			return KidTechCommand.ARG_PREFIX_IDTECH;
	}

	public String GetGameModSubDirectory()
	{
		if(isQ1)
			return "darkplaces";
		else if(isTDM)
			return "fms";
		else
			return null;
	}

	public KidTechCommand GetGameCommandEngine(String cmd)
	{
		if(isQ1 || isDOOM)
			return new KidTechQuakeCommand(cmd);
		else
			return new KidTech4Command(cmd);
	}

    public void InitTextureTable()
    {
        texture_table = new String[Q3EGlobals.UI_SIZE];

        texture_table[Q3EGlobals.UI_JOYSTICK] = "joystick_bg.png;joystick_center.png";
        texture_table[Q3EGlobals.UI_SHOOT] = "btn_sht.png";
        texture_table[Q3EGlobals.UI_JUMP] = "btn_jump.png";
        texture_table[Q3EGlobals.UI_CROUCH] = "btn_crouch.png";
        texture_table[Q3EGlobals.UI_RELOADBAR] = "btn_reload.png;btn_prevweapon.png;btn_ammo.png;btn_nextweapon.png";
        texture_table[Q3EGlobals.UI_PDA] = "btn_pda.png";
        texture_table[Q3EGlobals.UI_FLASHLIGHT] = "btn_flashlight.png";
        texture_table[Q3EGlobals.UI_SAVE] = "btn_pause.png;btn_savegame.png;btn_escape.png;btn_loadgame.png";
        texture_table[Q3EGlobals.UI_1] = "btn_1.png";
        texture_table[Q3EGlobals.UI_2] = "btn_2.png";
        texture_table[Q3EGlobals.UI_3] = "btn_3.png";
        texture_table[Q3EGlobals.UI_KBD] = "btn_keyboard.png";
        texture_table[Q3EGlobals.UI_CONSOLE] = "btn_notepad.png";
        texture_table[Q3EGlobals.UI_INTERACT] = "btn_activate.png";
        texture_table[Q3EGlobals.UI_ZOOM] = "btn_binocular.png";
        texture_table[Q3EGlobals.UI_RUN] = "btn_kick.png";

        texture_table[Q3EGlobals.UI_WEAPON_PANEL] = "disc_weapon.png";

		texture_table[Q3EGlobals.UI_SCORE] = "btn_score.png";

		texture_table[Q3EGlobals.UI_0] = "btn_0.png";
		texture_table[Q3EGlobals.UI_4] = "btn_4.png";
		texture_table[Q3EGlobals.UI_5] = "btn_5.png";
		texture_table[Q3EGlobals.UI_6] = "btn_6.png";
		texture_table[Q3EGlobals.UI_7] = "btn_7.png";
		texture_table[Q3EGlobals.UI_8] = "btn_8.png";
		texture_table[Q3EGlobals.UI_9] = "btn_9.png";
		texture_table[Q3EGlobals.UI_Y] = "btn_y.png";
		texture_table[Q3EGlobals.UI_N] = "btn_n.png";
		texture_table[Q3EGlobals.UI_PLUS] = "btn_plus.png";
		texture_table[Q3EGlobals.UI_MINUS] = "btn_minus.png";

		texture_table[Q3EGlobals.UI_NUM_PANEL] = "disc_num.png";
    }

    public void InitTypeTable()
    {
        type_table = Arrays.copyOf(_defaultType, _defaultType.length);
    }

    public void InitArgTable()
    {
        arg_table = Arrays.copyOf(_defaultArgs, _defaultArgs.length);
    }

    public void InitDefaultsTable()
    {
        defaults_table = new String[Q3EGlobals.UI_SIZE];
        Arrays.fill(defaults_table, "0 0 1 30");
    }

    public void InitTable()
    {
        UI_SIZE = Q3EGlobals.UI_SIZE;

        InitTypeTable();
        InitArgTable();
        InitTextureTable();
    }

    public void InitD3()
    {
        isD3 = true;
        //isD3BFG = true; // ???
		InitTable();
    }

    public boolean IsInitGame()
	{
		return isD3 || isD3BFG || isQ2 || isQ1 || isQ3 || isRTCW || isTDM || isDOOM || isETW || isRealRTCW || isFTEQW || isJA || isJO;
	}

	public boolean IsStandaloneGame()
	{
		return isTDM || isDOOM || isFTEQW;
	}

	public static boolean IsStandaloneGame(String game)
	{
		return Q3EGlobals.GAME_TDM.equalsIgnoreCase(game)
				|| Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(game)
				|| Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(game)
				;
	}

	public static String GetGameStandaloneDirectory(String name)
	{
		if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_PREY;
		else if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_QUAKE4;
		else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_QUAKE2;
		else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_QUAKE3;
		else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_RTCW;
		else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_TDM;
		else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_QUAKE1;
		else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_DOOMBFG;
		else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_GZDOOM;
		else if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_ETW;
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_REALRTCW;
		else if(Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_FTEQW;
		else if(Q3EGlobals.GAME_JA.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_JA;
		else if(Q3EGlobals.GAME_JO.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_SUBDIR_JO;
		else
			return Q3EGlobals.GAME_SUBDIR_DOOM3;
	}

	public String GetGameModPreferenceKey()
	{
		if(isQ4)
			return Q3EPreference.pref_harm_q4_fs_game;
		else if(isPrey)
			return Q3EPreference.pref_harm_prey_fs_game;
		else if(isQ2)
			return Q3EPreference.pref_harm_q2_fs_game;
		else if(isQ3)
			return Q3EPreference.pref_harm_q3_fs_game;
		else if(isRTCW)
			return Q3EPreference.pref_harm_rtcw_fs_game;
		else if(isTDM)
			return Q3EPreference.pref_harm_tdm_fs_game;
		else if(isQ1)
			return Q3EPreference.pref_harm_q1_fs_game;
		else if(isD3BFG)
			return Q3EPreference.pref_harm_d3bfg_fs_game;
		else if(isDOOM)
			return Q3EPreference.pref_harm_gzdoom_fs_game;
		else if(isETW)
			return Q3EPreference.pref_harm_etw_fs_game;
		else if(isRealRTCW)
			return Q3EPreference.pref_harm_realrtcw_fs_game;
		else if(isFTEQW)
			return Q3EPreference.pref_harm_fteqw_fs_game;
		else if(isJA)
			return Q3EPreference.pref_harm_ja_fs_game;
		else if(isJO)
			return Q3EPreference.pref_harm_jo_fs_game;
		else
			return Q3EPreference.pref_harm_fs_game;
	}

	public String GetEnableModPreferenceKey()
	{
		if(isQ4)
			return Q3EPreference.pref_harm_q4_user_mod;
		else if(isPrey)
			return Q3EPreference.pref_harm_prey_user_mod;
		else if(isQ2)
			return Q3EPreference.pref_harm_q2_user_mod;
		else if(isQ3)
			return Q3EPreference.pref_harm_q3_user_mod;
		else if(isRTCW)
			return Q3EPreference.pref_harm_rtcw_user_mod;
		else if(isTDM)
			return Q3EPreference.pref_harm_tdm_user_mod;
		else if(isQ1)
			return Q3EPreference.pref_harm_q1_user_mod;
		else if(isD3BFG)
			return Q3EPreference.pref_harm_d3bfg_user_mod;
		else if(isDOOM)
			return Q3EPreference.pref_harm_gzdoom_user_mod;
		else if(isETW)
			return Q3EPreference.pref_harm_etw_user_mod;
		else if(isRealRTCW)
			return Q3EPreference.pref_harm_realrtcw_user_mod;
		else if(isFTEQW)
			return Q3EPreference.pref_harm_fteqw_user_mod;
		else if(isJA)
			return Q3EPreference.pref_harm_ja_user_mod;
		else if(isJO)
			return Q3EPreference.pref_harm_jo_user_mod;
		else
			return Q3EPreference.pref_harm_user_mod;
	}

	public String GetGameUserModPreferenceKey()
	{
		if(isQ4)
			return Q3EPreference.pref_harm_q4_game_mod;
		else if(isPrey)
			return Q3EPreference.pref_harm_prey_game_mod;
		else if(isQ2)
			return Q3EPreference.pref_harm_q2_game_mod;
		else if(isQ3)
			return Q3EPreference.pref_harm_q3_game_mod;
		else if(isRTCW)
			return Q3EPreference.pref_harm_rtcw_game_mod;
		else if(isTDM)
			return Q3EPreference.pref_harm_tdm_game_mod;
		else if(isQ1)
			return Q3EPreference.pref_harm_q1_game_mod;
		else if(isD3BFG)
			return Q3EPreference.pref_harm_d3bfg_game_mod;
		else if(isDOOM)
			return Q3EPreference.pref_harm_gzdoom_game_mod;
		else if(isETW)
			return Q3EPreference.pref_harm_etw_game_mod;
		else if(isRealRTCW)
			return Q3EPreference.pref_harm_realrtcw_game_mod;
		else if(isFTEQW)
			return Q3EPreference.pref_harm_fteqw_game_mod;
		else if(isJA)
			return Q3EPreference.pref_harm_ja_game_mod;
		else if(isJO)
			return Q3EPreference.pref_harm_jo_game_mod;
		else
			return Q3EPreference.pref_harm_game_mod;
	}

	public String GetGameModLibPreferenceKey()
	{
		if(isQ4)
			return Q3EPreference.pref_harm_q4_game_lib;
		else if(isPrey)
			return Q3EPreference.pref_harm_prey_game_lib;
		else if(isQ2)
			return Q3EPreference.pref_harm_q2_game_lib;
		else if(isQ3)
			return Q3EPreference.pref_harm_q3_game_lib;
		else if(isRTCW)
			return Q3EPreference.pref_harm_rtcw_game_lib;
		else if(isTDM)
			return Q3EPreference.pref_harm_tdm_game_lib;
		else if(isQ1)
			return Q3EPreference.pref_harm_q1_game_lib;
		else if(isD3BFG)
			return Q3EPreference.pref_harm_d3bfg_game_lib;
		else if(isDOOM)
			return Q3EPreference.pref_harm_gzdoom_game_lib;
		else if(isETW)
			return Q3EPreference.pref_harm_etw_game_lib;
		else if(isRealRTCW)
			return Q3EPreference.pref_harm_realrtcw_game_lib;
		else if(isFTEQW)
			return Q3EPreference.pref_harm_fteqw_game_lib;
		else if(isJA)
			return Q3EPreference.pref_harm_ja_game_lib;
		else if(isJO)
			return Q3EPreference.pref_harm_jo_game_lib;
		else
			return Q3EPreference.pref_harm_game_lib;
	}

	public String GetGameCommandPreferenceKey()
	{
		if(isQ4)
			return Q3EPreference.pref_params_quake4;
		else if(isPrey)
			return Q3EPreference.pref_params_prey;
		else if(isQ2)
			return Q3EPreference.pref_params_q2;
		else if(isQ3)
			return Q3EPreference.pref_params_q3;
		else if(isRTCW)
			return Q3EPreference.pref_params_rtcw;
		else if(isTDM)
			return Q3EPreference.pref_params_tdm;
		else if(isQ1)
			return Q3EPreference.pref_params_q1;
		else if(isD3BFG)
			return Q3EPreference.pref_params_d3bfg;
		else if(isDOOM)
			return Q3EPreference.pref_params_gzdoom;
		else if(isETW)
			return Q3EPreference.pref_params_etw;
		else if(isRealRTCW)
			return Q3EPreference.pref_params_realrtcw;
		else if(isFTEQW)
			return Q3EPreference.pref_params_fteqw;
		else if(isJA)
			return Q3EPreference.pref_params_ja;
		else if(isJO)
			return Q3EPreference.pref_params_jo;
		else
			return Q3EPreference.pref_params;
	}

	public String GetGameCommandRecordPreferenceKey()
	{
		if(isQ4)
			return Q3EPreference.pref_harm_q4_command_record;
		else if(isPrey)
			return Q3EPreference.pref_harm_prey_command_record;
		else if(isQ2)
			return Q3EPreference.pref_harm_q2_command_record;
		else if(isQ3)
			return Q3EPreference.pref_harm_q3_command_record;
		else if(isRTCW)
			return Q3EPreference.pref_harm_rtcw_command_record;
		else if(isTDM)
			return Q3EPreference.pref_harm_tdm_command_record;
		else if(isQ1)
			return Q3EPreference.pref_harm_q1_command_record;
		else if(isD3BFG)
			return Q3EPreference.pref_harm_d3bfg_command_record;
		else if(isDOOM)
			return Q3EPreference.pref_harm_gzdoom_command_record;
		else if(isETW)
			return Q3EPreference.pref_harm_etw_command_record;
		else if(isRealRTCW)
			return Q3EPreference.pref_harm_realrtcw_command_record;
		else if(isFTEQW)
			return Q3EPreference.pref_harm_fteqw_command_record;
		else if(isJA)
			return Q3EPreference.pref_harm_ja_command_record;
		else if(isJO)
			return Q3EPreference.pref_harm_jo_command_record;
		else
			return Q3EPreference.pref_harm_command_record;
	}

	public String GetGameVersionPreferenceKey()
	{
		if(isD3BFG)
			return Q3EPreference.pref_harm_d3bfg_rendererBackend;
		else if(isRealRTCW)
			return Q3EPreference.pref_harm_realrtcw_version;
		else
			return null;
	}

	public String GetGameHomeDirectoryPath()
	{
		if(isD3BFG)
			return ".local/share/rbdoom3bfg";
		else if(isQ2)
			return ".yq2";
		else if(isQ3)
			return ".q3a";
		else if(isRTCW)
			return ".wolf";
		else if(isDOOM)
			return ".config/gzdoom";
		else if(isETW)
			return ".etlegacy/legacy";
		else if(isRealRTCW)
			return ".realrtcw";
		else
			return null;
	}

	public static int GetGameID(String name)
	{
		if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_PREY;
		else if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_QUAKE4;
		else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_QUAKE2;
		else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_QUAKE3;
		else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_RTCW;
		else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_TDM;
		else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_QUAKE1;
		else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_DOOM3BFG;
		else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_GZDOOM;
		else if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_ETW;
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_REALRTCW;
		else if(Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_FTEQW;
		else if(Q3EGlobals.GAME_JA.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_JA;
		else if(Q3EGlobals.GAME_JO.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_ID_JO;
		else
			return Q3EGlobals.GAME_ID_DOOM3;
	}

	public static String[] GetGameVersions(String name)
	{
		if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return new String[] {
					Q3EGlobals.GAME_VERSION_D3BFG_OPENGL,
					Q3EGlobals.GAME_VERSION_D3BFG_VULKAN,
			};
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return new String[] {
					Q3EGlobals.GAME_VERSION_CURRENT,
					Q3EGlobals.GAME_VERSION_REALRTCW_5_0,
			};
		else
			return null;
	}

	public static String GetGameModPreferenceKey(String name)
	{
		if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q4_fs_game;
		else if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_prey_fs_game;
		else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q2_fs_game;
		else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q3_fs_game;
		else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_rtcw_fs_game;
		else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_tdm_fs_game;
		else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q1_fs_game;
		else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_d3bfg_fs_game;
		else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_gzdoom_fs_game;
		else if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_etw_fs_game;
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_realrtcw_fs_game;
		else if(Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_fteqw_fs_game;
		else if(Q3EGlobals.GAME_JA.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_ja_fs_game;
		else if(Q3EGlobals.GAME_JO.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_jo_fs_game;
		else
			return Q3EPreference.pref_harm_fs_game;
	}

	public static String GetGameVersionPreferenceKey(String name)
	{
		if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_d3bfg_rendererBackend;
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_realrtcw_version;
		else
			return null;
	}

	public static String GetEnableModPreferenceKey(String name)
	{
		if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q4_user_mod;
		else if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_prey_user_mod;
		else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q2_user_mod;
		else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q3_user_mod;
		else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_rtcw_user_mod;
		else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_tdm_user_mod;
		else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_q1_user_mod;
		else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_d3bfg_user_mod;
		else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_gzdoom_user_mod;
		else if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_etw_user_mod;
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_realrtcw_user_mod;
		else if(Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_fteqw_user_mod;
		else if(Q3EGlobals.GAME_JA.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_ja_user_mod;
		else if(Q3EGlobals.GAME_JO.equalsIgnoreCase(name))
			return Q3EPreference.pref_harm_jo_user_mod;
		else
			return Q3EPreference.pref_harm_user_mod;
	}

	public static String GetGameBaseDirectory(String name)
	{
		if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_PREY;
		else if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_QUAKE4;
		else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_QUAKE2;
		else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_QUAKE3;
		else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_RTCW;
		else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_TDM;
		else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_QUAKE1;
		else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_DOOM3BFG;
		else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_GZDOOM;
		else if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_ETW;
		else if(Q3EGlobals.GAME_REALRTCW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_REALRTCW;
		else if(Q3EGlobals.GAME_FTEQW.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_FTEQW;
		else if(Q3EGlobals.GAME_JA.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_JA;
		else if(Q3EGlobals.GAME_JO.equalsIgnoreCase(name))
			return Q3EGlobals.GAME_BASE_JO;
		else
			return Q3EGlobals.GAME_BASE_DOOM3;
	}

	public void SetAppStoragePath(Context context)
	{
		app_storage_path = Q3EUtils.GetAppStoragePath(context, null);
	}

	public String MakeTempBaseCommand(Context context)
	{
		SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
		String extraCommand = "";
		if (IsIdTech4())
		{
			int autoAspectRatio = preferences.getInt(Q3EPreference.pref_harm_r_autoAspectRatio, 1);
			if(autoAspectRatio > 0)
				extraCommand = GetGameCommandEngine(extraCommand).SetProp("harm_r_autoAspectRatio", autoAspectRatio).toString();
		}

		if ((IsIdTech4() || IsIdTech3()) && preferences.getBoolean(Q3EPreference.pref_harm_skip_intro, false))
			extraCommand = GetGameCommandEngine(extraCommand).SetCommand("disconnect", false).toString();
		if ((IsIdTech4() || isRTCW || isRealRTCW) && preferences.getBoolean(Q3EPreference.pref_harm_auto_quick_load, false))
			extraCommand = GetGameCommandEngine(extraCommand).SetParam("loadGame", "QuickSave").toString();

		if (isDOOM)
		{
			if(preferences.getBoolean(Q3EPreference.pref_harm_gzdoom_load_lights_pk3, true))
				extraCommand = GetGameCommandEngine(extraCommand).AddParam("file", "lights.pk3").toString();
			if(preferences.getBoolean(Q3EPreference.pref_harm_gzdoom_load_brightmaps_pk3, true))
				extraCommand = GetGameCommandEngine(extraCommand).AddParam("file", "brightmaps.pk3").toString();
		}
		return extraCommand.trim();
	}

	public static void DumpDefaultOnScreenConfig(int[] args, int[] type)
	{
		_defaultArgs = Arrays.copyOf(args, args.length);
		_defaultType = Arrays.copyOf(type, args.length);
	}

	public static void RestoreDefaultOnScreenConfig(int[] args, int[] type)
	{
		System.arraycopy(_defaultArgs, 0, args, 0, args.length);
		System.arraycopy(_defaultType, 0, type, 0, type.length);
	}

	private static void InitDefaultTypeTable()
	{
		int[] type_table = new int[Q3EGlobals.UI_SIZE];

		type_table[Q3EGlobals.UI_JOYSTICK] = Q3EGlobals.TYPE_JOYSTICK;
		type_table[Q3EGlobals.UI_SHOOT] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_JUMP] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_CROUCH] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_RELOADBAR] = Q3EGlobals.TYPE_SLIDER;
		type_table[Q3EGlobals.UI_PDA] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_FLASHLIGHT] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_SAVE] = Q3EGlobals.TYPE_SLIDER;
		type_table[Q3EGlobals.UI_1] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_2] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_3] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_KBD] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_CONSOLE] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_RUN] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_ZOOM] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_INTERACT] = Q3EGlobals.TYPE_BUTTON;

		type_table[Q3EGlobals.UI_WEAPON_PANEL] = Q3EGlobals.TYPE_DISC;
		type_table[Q3EGlobals.UI_NUM_PANEL] = Q3EGlobals.TYPE_DISC;

		type_table[Q3EGlobals.UI_SCORE] = Q3EGlobals.TYPE_BUTTON;

		for(int i = Q3EGlobals.UI_0; i <= Q3EGlobals.UI_9; i++)
			type_table[i] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_Y] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_N] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_PLUS] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_MINUS] = Q3EGlobals.TYPE_BUTTON;

		_defaultType = Arrays.copyOf(type_table, type_table.length);
	}

	private static void InitDefaultArgTable()
	{
		int[] arg_table = new int[Q3EGlobals.UI_SIZE * 4];

		arg_table[Q3EGlobals.UI_SHOOT * 4] = Q3EKeyCodes.KeyCodesGeneric.K_MOUSE1;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 3] = 0;


		arg_table[Q3EGlobals.UI_JUMP * 4] = Q3EKeyCodes.KeyCodesGeneric.K_SPACE;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_CROUCH * 4] = Q3EKeyCodes.KeyCodesGeneric.K_C; // BFG
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM;
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_RELOADBAR * 4] = Q3EKeyCodes.KeyCodesGeneric.K_RBRACKET; // 93
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 1] = Q3EKeyCodes.KeyCodesGeneric.K_R; // 114
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 2] = Q3EKeyCodes.KeyCodesGeneric.K_LBRACKET; // 91
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 3] = Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT;

		arg_table[Q3EGlobals.UI_PDA * 4] = Q3EKeyCodes.KeyCodesGeneric.K_TAB;
		arg_table[Q3EGlobals.UI_PDA * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_PDA * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_PDA * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F; // BFG
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_SAVE * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F5;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 1] = Q3EKeyCodes.KeyCodesGeneric.K_ESCAPE;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 2] = Q3EKeyCodes.KeyCodesGeneric.K_F9;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 3] = Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK;

		arg_table[Q3EGlobals.UI_1 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F1;
		arg_table[Q3EGlobals.UI_1 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_1 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_1 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_2 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F2;
		arg_table[Q3EGlobals.UI_2 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_2 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_2 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_3 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F3;
		arg_table[Q3EGlobals.UI_3 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_3 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_3 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_KBD * 4] = Q3EKeyCodes.K_VKBD;
		arg_table[Q3EGlobals.UI_KBD * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_KBD * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_KBD * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_CONSOLE * 4] = Q3EKeyCodes.KeyCodesGeneric.K_GRAVE;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_RUN * 4] = Q3EKeyCodes.KeyCodesGeneric.K_SHIFT;
		arg_table[Q3EGlobals.UI_RUN * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_RUN * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_RUN * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_ZOOM * 4] = Q3EKeyCodes.KeyCodesGeneric.K_Z;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_INTERACT * 4] = Q3EKeyCodes.KeyCodesGeneric.K_MOUSE2;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_SCORE * 4] = Q3EKeyCodes.KeyCodesGeneric.K_M;
		arg_table[Q3EGlobals.UI_SCORE * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_SCORE * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_SCORE * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_0 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F10;
		arg_table[Q3EGlobals.UI_0 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_0 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_0 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_4 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F4;
		arg_table[Q3EGlobals.UI_4 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_4 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_4 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_5 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F5;
		arg_table[Q3EGlobals.UI_5 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_5 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_5 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_6 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F6;
		arg_table[Q3EGlobals.UI_6 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_6 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_6 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_7 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F7;
		arg_table[Q3EGlobals.UI_7 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_7 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_7 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_8 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F8;
		arg_table[Q3EGlobals.UI_8 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_8 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_8 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_9 * 4] = Q3EKeyCodes.KeyCodesGeneric.K_F9;
		arg_table[Q3EGlobals.UI_9 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_9 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_9 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_WEAPON_PANEL * 4] = Q3EKeyCodes.ONSCRREN_DISC_KEYS_WEAPON; // all keys map index
		arg_table[Q3EGlobals.UI_WEAPON_PANEL * 4 + 1] = Q3EGlobals.ONSCRREN_DISC_SWIPE;
		arg_table[Q3EGlobals.UI_WEAPON_PANEL * 4 + 2] = 0; // 4 chars name
		arg_table[Q3EGlobals.UI_WEAPON_PANEL * 4 + 3] = 0; // keep

		arg_table[Q3EGlobals.UI_NUM_PANEL * 4] = Q3EKeyCodes.ONSCRREN_DISC_KEYS_NUM;
		arg_table[Q3EGlobals.UI_NUM_PANEL * 4 + 1] = Q3EGlobals.ONSCRREN_DISC_CLICK;
		arg_table[Q3EGlobals.UI_NUM_PANEL * 4 + 2] = ('N' << 16) | ('u' << 8) | 'm';
		arg_table[Q3EGlobals.UI_NUM_PANEL * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_Y * 4] = Q3EKeyCodes.KeyCodesGeneric.K_Y;
		arg_table[Q3EGlobals.UI_Y * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_Y * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_Y * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_N * 4] = Q3EKeyCodes.KeyCodesGeneric.K_N;
		arg_table[Q3EGlobals.UI_N * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_N * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_N * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_PLUS * 4] = Q3EKeyCodes.KeyCodesGeneric.K_EQUALS;
		arg_table[Q3EGlobals.UI_PLUS * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_PLUS * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_PLUS * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_MINUS * 4] = Q3EKeyCodes.KeyCodesGeneric.K_MINUS;
		arg_table[Q3EGlobals.UI_MINUS * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_MINUS * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_MINUS * 4 + 3] = 0;

		_defaultArgs = Arrays.copyOf(arg_table, arg_table.length);
	}

	public void LoadTypeAndArgTablePreference(Context context)
	{
		// index:type;23,1,2,0|......
		try
		{
			Set<String> configs = PreferenceManager.getDefaultSharedPreferences(context).getStringSet(Q3EPreference.ONSCREEN_BUTTON, null);
			if (null != configs && !configs.isEmpty())
			{
				for (String str : configs)
				{
					String[] subArr = str.split(":", 2);
					int index = Integer.parseInt(subArr[0]);
					subArr = subArr[1].split(";");
					type_table[index] = Integer.parseInt(subArr[0]);
					String[] argArr = subArr[1].split(",");
					arg_table[index * 4] = Integer.parseInt(argArr[0]);
					arg_table[index * 4 + 1] = Integer.parseInt(argArr[1]);
					arg_table[index * 4 + 2] = Integer.parseInt(argArr[2]);
					arg_table[index * 4 + 3] = Integer.parseInt(argArr[3]);
				}
			}
		}
		catch (Exception e)
		{
			//UncaughtExceptionHandler.DumpException(this, Thread.currentThread(), e);
			e.printStackTrace();
			RestoreDefaultOnScreenConfig(arg_table, type_table);
		}
	}

	public boolean IS_D3()
	{
		return isD3 && (!isQ4 || !isPrey);
	}

	public String GetGameDataDirectoryPath(String file)
	{
		return KStr.AppendPath(datadir, subdatadir, file);
	}
}
