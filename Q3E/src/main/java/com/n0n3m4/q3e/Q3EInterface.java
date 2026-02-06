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

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;

import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTech4Command;
import com.n0n3m4.q3e.karin.KidTechCommand;
import com.n0n3m4.q3e.karin.KidTechQuakeCommand;
import com.n0n3m4.q3e.karin.KidTechWolf3DCommand;
import com.n0n3m4.q3e.keycode.KeyCodesGeneric;
import com.n0n3m4.q3e.onscreen.Q3EControls;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
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
	public String[] _defaultTable;
	public String[] _defaultPortraitTable;

	public int UI_SIZE;
	public String[] defaults_table;
	public String[] texture_table;
	public int[] type_table;
	public int[] arg_table; // slider: key,key,key,style | button: key,canbeheld,style,null

	// KARIN_NEW_GAME_BOOKMARK: add isXXX state
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
	public boolean isSamTFE = false;
	public boolean isSamTSE = false;
	public boolean isXash3D = false;
	public boolean isSource = false;
	public boolean isUrT = false;
	public boolean isMOHAA = false;
	public boolean isWolf3D = false;
	public boolean isSkinDeep = false;

	public String default_path = Environment.getExternalStorageDirectory() + "/diii4a";

	public String libname;
	public String config_name;
	public String game;
	public String game_name;
	public String game_base;
	public String engine_version;
	public String datadir;
	public boolean standalone = false;
	public String subdatadir;
	public int game_id;
	public String game_version;

    public String start_temporary_extra_command = "";
	public String cmd = Q3EGameConstants.GAME_EXECUABLE;

	public String app_storage_path = "/sdcard/diii4a";
	
	//RTCW4A:
	/*
	public final int RTCW4A_UI_ACTION=6;
	public final int RTCW4A_UI_KICK=7;
	 */

	public String EngineLibName()
	{
		return Q3EGame.Find(game_id).ENGINE_LIB;
	}

	// KARIN_GAME_VERSION_BOOKMARK: add/remove other game engine library by version
	public String GetEngineLibName()
	{
		if(null == engine_version || engine_version.isEmpty())
			return EngineLibName();

		if(isD3BFG)
		{
			if(Q3EGameConstants.GAME_VERSION_D3BFG_VULKAN.equalsIgnoreCase(engine_version))
				return Q3EGameConstants.LIB_ENGINE4_D3BFG_VULKAN;
			else
				return Q3EGameConstants.LIB_ENGINE4_D3BFG;
		}
/*		else if(isRealRTCW)
		{
			if(Q3EGameConstants.GAME_VERSION_REALRTCW_5_2.equalsIgnoreCase(engine_version))
				return Q3EGameConstants.LIB_ENGINE3_REALRTCW_5_2;
			else
				return Q3EGameConstants.LIB_ENGINE3_REALRTCW;
		}*/
		/*else if(isTDM)
		{
			if(Q3EGameConstants.GAME_VERSION_TDM_2_12.equalsIgnoreCase(engine_version))
				return Q3EGameConstants.LIB_ENGINE4_TDM_2_12;
			else
				return Q3EGameConstants.LIB_ENGINE4_TDM;
		}*/
		else
			return EngineLibName();
	}

	public void SetupEngineLib()
	{
		libname = EngineLibName();
	}

	public String GameVersion(Context context)
	{
		SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
		if(!HasVersions())
			return null;
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

	public String GameSubDirectory()
	{
		return Q3EGame.Find(game_id).DIR;
	}

	// KARIN_NEW_GAME_BOOKMARK: add isXXX state to game id
	public int GameID()
	{
		if(isPrey)
			return Q3EGameConstants.GAME_ID_PREY;
		else if(isQ4)
			return Q3EGameConstants.GAME_ID_QUAKE4;
		else if(isQ2)
			return Q3EGameConstants.GAME_ID_QUAKE2;
		else if(isQ3)
			return Q3EGameConstants.GAME_ID_QUAKE3;
		else if(isRTCW)
			return Q3EGameConstants.GAME_ID_RTCW;
		else if(isTDM)
			return Q3EGameConstants.GAME_ID_TDM;
		else if(isQ1)
			return Q3EGameConstants.GAME_ID_QUAKE1;
		else if(isD3BFG)
			return Q3EGameConstants.GAME_ID_DOOM3BFG;
		else if(isDOOM)
			return Q3EGameConstants.GAME_ID_ZDOOM;
		else if(isETW)
			return Q3EGameConstants.GAME_ID_ETW;
		else if(isRealRTCW)
			return Q3EGameConstants.GAME_ID_REALRTCW;
		else if(isFTEQW)
			return Q3EGameConstants.GAME_ID_FTEQW;
		else if(isJA)
			return Q3EGameConstants.GAME_ID_JA;
		else if(isJO)
			return Q3EGameConstants.GAME_ID_JO;
		else if(isSamTFE)
			return Q3EGameConstants.GAME_ID_SAMTFE;
		else if(isSamTSE)
			return Q3EGameConstants.GAME_ID_SAMTSE;
		else if(isXash3D)
			return Q3EGameConstants.GAME_ID_XASH3D;
		else if(isSource)
			return Q3EGameConstants.GAME_ID_SOURCE;
		else if(isUrT)
			return Q3EGameConstants.GAME_ID_URT;
		else if(isMOHAA)
			return Q3EGameConstants.GAME_ID_MOHAA;
		else if(isWolf3D)
			return Q3EGameConstants.GAME_ID_WOLF3D;
		else if(isSkinDeep)
			return Q3EGameConstants.GAME_ID_SKINDEEP;
		else
			return Q3EGameConstants.GAME_ID_DOOM3;
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
		else if(Q3EGame.Find(game_id).STANDALONE)
			subdatadir = subdir;
		else
			subdatadir = null;
	}

	// KARIN_NEW_GAME_BOOKMARK: add setupXXX
	public void SetupGame(String name)
	{
		Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "SetupGame: " + name);
		if(Q3EGameConstants.GAME_PREY.equalsIgnoreCase(name))
			SetupPrey();
		else if(Q3EGameConstants.GAME_QUAKE4.equalsIgnoreCase(name))
			SetupQuake4();
		else if(Q3EGameConstants.GAME_QUAKE2.equalsIgnoreCase(name))
			SetupQuake2();
		else if(Q3EGameConstants.GAME_QUAKE3.equalsIgnoreCase(name))
			SetupQuake3();
		else if(Q3EGameConstants.GAME_RTCW.equalsIgnoreCase(name))
			SetupRTCW();
		else if(Q3EGameConstants.GAME_TDM.equalsIgnoreCase(name))
			SetupTDM();
		else if(Q3EGameConstants.GAME_QUAKE1.equalsIgnoreCase(name))
			SetupQuake1();
		else if(Q3EGameConstants.GAME_DOOM3BFG.equalsIgnoreCase(name))
			SetupDoom3BFG();
		else if(Q3EGameConstants.GAME_ZDOOM.equalsIgnoreCase(name))
			SetupZDoom();
		else if(Q3EGameConstants.GAME_ETW.equalsIgnoreCase(name))
			SetupETW();
		else if(Q3EGameConstants.GAME_REALRTCW.equalsIgnoreCase(name))
			SetupRealRTCW();
		else if(Q3EGameConstants.GAME_FTEQW.equalsIgnoreCase(name))
			SetupFTEQW();
		else if(Q3EGameConstants.GAME_JA.equalsIgnoreCase(name))
			SetupJA();
		else if(Q3EGameConstants.GAME_JO.equalsIgnoreCase(name))
			SetupJO();
		else if(Q3EGameConstants.GAME_SAMTFE.equalsIgnoreCase(name))
			SetupSamTFE();
		else if(Q3EGameConstants.GAME_SAMTSE.equalsIgnoreCase(name))
			SetupSamTSE();
		else if(Q3EGameConstants.GAME_XASH3D.equalsIgnoreCase(name))
			SetupXash3D();
		else if(Q3EGameConstants.GAME_SOURCE.equalsIgnoreCase(name))
			SetupSource();
		else if(Q3EGameConstants.GAME_URT.equalsIgnoreCase(name))
			SetupUT();
		else if(Q3EGameConstants.GAME_MOHAA.equalsIgnoreCase(name))
			SetupMOHAA();
		else if(Q3EGameConstants.GAME_WOLF3D.equalsIgnoreCase(name))
			SetupWolf3D();
		else if(Q3EGameConstants.GAME_SKINDEEP.equalsIgnoreCase(name))
			SetupSkinDeep();
		else
			SetupDOOM3();
	}

	// KARIN_NEW_GAME_BOOKMARK: add reset isXXX = false
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
		isSamTFE = false;
		isSamTSE = false;
		isXash3D = false;
		isSource = false;
		isUrT = false;
		isMOHAA = false;
		isWolf3D = false;
		isSkinDeep = false;
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

	public void SetupZDoom()
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

	public void SetupSamTFE()
	{
		ResetGameState();
		isSamTFE = true;
		SetupGameConfig();
	}

	public void SetupSamTSE()
	{
		ResetGameState();
		isSamTSE = true;
		SetupGameConfig();
	}

	public void SetupXash3D()
	{
		ResetGameState();
		isXash3D = true;
		SetupGameConfig();
	}

	public void SetupSource()
	{
		ResetGameState();
		isSource = true;
		SetupGameConfig();
	}

	public void SetupUT()
	{
		ResetGameState();
		isUrT = true;
		SetupGameConfig();
	}

	public void SetupMOHAA()
	{
		ResetGameState();
		isMOHAA = true;
		SetupGameConfig();
	}

	public void SetupWolf3D()
	{
		ResetGameState();
		isWolf3D = true;
		SetupGameConfig();
	}

	public void SetupSkinDeep()
	{
		ResetGameState();
		isSkinDeep = true;
		SetupGameConfig();
	}

	public void SetupGameConfig()
	{
		game_id = GameID();
		Q3EGame cfg = Q3EGame.Find(game_id);

		game = cfg.TYPE;
		game_name = cfg.NAME;
		game_base = cfg.BASE;
		game_version = cfg.VERSION;

		libname = cfg.ENGINE_LIB;
		config_name = cfg.CONFIG_FILE;

		engine_version = null;

		Q3EKeyCodes.InitKeycodes(cfg.KEYCODE);

		SetupSubDir();
	}

	public void SetupEngineVersion(String version)
	{
		if("".equals(version))
			version = null;
		engine_version = version;
	}

	public void SetupEngineVersion(Context context)
	{
		SetupEngineVersion(GameVersion(context));
	}

	// KARIN_NEW_GAME_BOOKMARK: add IsXxxXxx conditions
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
		return isQ3 || isRTCW || isETW || isRealRTCW || isJA || isJO || isUrT || isMOHAA;
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
		return isDOOM || isWolf3D;
	}

	public boolean IsUsingSDL()
	{
		return isXash3D || isSource || isWolf3D || isSkinDeep;
	}

	public boolean IsUsingVirtualMouse()
	{
		return isSamTFE || isSamTSE;
	}

	public boolean IsUsingOpenAL()
	{
		return isD3 || isQ4 || isPrey
				|| isD3BFG || isTDM || isSkinDeep
				|| isQ3 || isRTCW || isETW || isRealRTCW || isJA || isJO || isUrT || isMOHAA || isFTEQW
				|| isDOOM || isQ2 || isQ1
				|| isSamTFE || isSamTSE
				;
	}

	public boolean HasOpenGLSetting()
	{
		return isD3 || isQ4 || isPrey;
	}

	public boolean IsSupportQuickload()
	{
		return isPrey || isQ4 || isD3 || isRTCW || isRealRTCW;
	}

	public boolean IsSupportSkipIntro()
	{
		return IsSupportQuickload() || isQ3 || isJA || isJO || isUrT || isSkinDeep;
	}

	public boolean IsSupportMod()
	{
		return isD3 || isQ4 || isPrey
				|| isD3BFG || isTDM || isSkinDeep
				|| isRTCW || isQ3 || isETW || isRealRTCW || isFTEQW || isJA || isJO || isUrT || isMOHAA
				|| isQ2 || isQ1
				|| isDOOM || isWolf3D
				|| isXash3D || isSource
				;
	}

	public boolean IsInitGame()
	{
		return isD3 || isQ4 || isPrey
				|| isD3BFG || isTDM || isSkinDeep
				|| isRTCW || isQ3 || isETW || isRealRTCW || isFTEQW || isJA || isJO || isUrT || isMOHAA
				|| isQ2 || isQ1
				|| isDOOM || isWolf3D
				|| isSamTFE || isSamTSE
				|| isXash3D || isSource
				;
	}

	public boolean IsStandaloneGame()
	{
		return Q3EGame.Find(game_id).STANDALONE;
	}

	public boolean IS_D3()
	{
		return isD3 && (!isQ4 || !isPrey);
	}

	public static boolean IsStandaloneGame(String game)
	{
		return Q3EGame.Find(game).STANDALONE;
	}

	public boolean IsSupportExternalDLL()
	{
		return IsIdTech4() || isXash3D;
	}

	// KARIN_DISABLE_GAME_BOOKMARK: add isXXX state to disable games
	public boolean IsDisabled()
	{
		return
				//isFTEQW ||
						isXash3D
				;
	}

	// KARIN_DISABLE_GAME_BOOKMARK: add game type to disable games
	public static boolean IsDisabled(String game)
	{
		final String[] DisabledGames = {
				//Q3EGameConstants.GAME_FTEQW,
				Q3EGameConstants.GAME_XASH3D,
		};
		for(String disabledGame : DisabledGames)
		{
			if(disabledGame.equalsIgnoreCase(game))
				return true;
		}
		return false;
	}

	public boolean IsSupportSecondaryDirGame()
	{
		return null != Q3EGame.Find(game_id).MOD_SECONDARY_PARM;
	}

	public static boolean IsSupportSecondaryDirGame(String game)
	{
		return null != Q3EGame.Find(game).MOD_SECONDARY_PARM;
	}

	public String GetGameCommandParm()
	{
		return Q3EGame.Find(game_id).MOD_PARM;
	}

	public String GetSecondaryGameCommandParm()
	{
		return Q3EGame.Find(game_id).MOD_SECONDARY_PARM;
	}

	public String GetGameCommandPrefix()
	{
		if(isQ1 || isFTEQW || isXash3D || isSource)
			return KidTechCommand.ARG_PREFIX_QUAKETECH;
		if(isDOOM)
			return KidTechCommand.ARG_PREFIX_QUAKETECH + KidTechCommand.ARG_PREFIX_IDTECH;
		if(isWolf3D)
			return KidTechCommand.ARG_PREFIX_WOLFTECH;
		else
			return KidTechCommand.ARG_PREFIX_IDTECH;
	}

	public String GetGameModSubDirectory()
	{
		return Q3EGame.Find(game_id).MOD_DIR;
	}

	public KidTechCommand GetGameCommandEngine(String cmd)
	{
		if(isQ1 || isDOOM || isXash3D || isSource)
			return new KidTechQuakeCommand(cmd);
		if(isWolf3D)
			return new KidTechWolf3DCommand(cmd);
		else
			return new KidTech4Command(cmd);
	}

    // KARIN_NEW_GAME_BOOKMARK: if using audio track
    public boolean NeedAudioTrack()
    {
        return isQ3 || isRTCW || isQ1 || isQ2|| isETW || isRealRTCW || isFTEQW || isJA || isJO || isUrT || isMOHAA;
    }

	// KARIN_NEW_GAME_BOOKMARK: add config file list
	public String[] GetGameConfigFiles()
	{
		List<String> list = new ArrayList<>();

		if(isQ4)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_QUAKE4);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isPrey)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_PREY);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isQ2)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_QUAKE2);
			list.add("<mod>/yq2.cfg");
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/yq2.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isQ3)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_QUAKE3);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isRTCW)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_RTCW);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isQ1)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_QUAKE1);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isD3BFG)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_DOOM3BFG);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isTDM)
		{
			list.add(Q3EGameConstants.CONFIG_FILE_TDM);
			list.add("autoexec.cfg");
		}
		else if(isDOOM)
		{
			list.add(Q3EGameConstants.CONFIG_FILE_ZDOOM);
		}
		else if(isETW)
		{
			list.add(Q3EGameConstants.CONFIG_FILE_ETW);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isRealRTCW)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_REALRTCW);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isFTEQW)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_FTEQW);
			list.add("<mod>/autoexec.cfg");
		}
		else if(isJA)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_JA);
			list.add("<mod>/autoexec_sp.cfg");
			list.add("<base>/autoexec_sp.cfg");
		}
		else if(isJO)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_JO);
			list.add("<mod>/autoexec_sp.cfg");
			list.add("<base>/autoexec_sp.cfg");
		}
		else if(isSamTFE)
		{
		}
		else if(isSamTSE)
		{
		}
		else if(isXash3D)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_XASH3D);
			list.add("<mod>/autoexec.cfg");
		}
		else if(isSource)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_SOURCE);
		}
		else if(isUrT)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_URT);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else if(isMOHAA)
		{
			list.add("<base>/" + Q3EGameConstants.CONFIG_FILE_MOHAA);
		}
		else if(isWolf3D)
		{
			list.add(Q3EGameConstants.CONFIG_FILE_WOLF3D);
		}
		else if(isSkinDeep)
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_SKINDEEP);
			list.add("<mod>/autoexec.cfg");
			list.add("<mod>/graphics.cfg");
			list.add("<base>/autoexec.cfg");
		}
		else
		{
			list.add("<mod>/" + Q3EGameConstants.CONFIG_FILE_DOOM3);
			list.add("<mod>/autoexec.cfg");
			list.add("<base>/autoexec.cfg");
		}

		return list.toArray(new String[0]);
	}

	public static String GetGameStandaloneDirectory(String game)
	{
		return Q3EGame.Find(game).DIR;
	}

	public String GetGameModPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_MOD;
	}

	public String GetEnableModPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_MOD_ENABLED;
	}

	public String GetGameUserModPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_MOD_USER;
	}

	public String GetGameModLibPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_MOD_LIB;
	}

	public String GetGameCommandPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_COMMAND;
	}

	public String GetGameCommandRecordPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_CMD_RECORD;
	}

	public String GetGameEnvPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_ENV;
	}

	public String GetGameVersionPreferenceKey()
	{
		return Q3EGame.Find(game_id).PREF_VERSION;
	}

	public String GetGameHomeDirectoryPath()
	{
		return Q3EGame.Find(game_id).HOME_DIR;
	}

	public boolean HasVersions()
	{
		return GetGameVersions(game) != null;
	}

	public static int GetGameID(String name)
	{
		return Q3EGame.Find(name).ID;
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

		texture_table[Q3EGlobals.UI_Q] = "btn_q.png";
		texture_table[Q3EGlobals.UI_W] = "btn_w.png";
		texture_table[Q3EGlobals.UI_E] = "btn_e.png";
		texture_table[Q3EGlobals.UI_U] = "btn_u.png";
		texture_table[Q3EGlobals.UI_T] = "btn_t.png";
		texture_table[Q3EGlobals.UI_I] = "btn_i.png";
		texture_table[Q3EGlobals.UI_O] = "btn_o.png";
		texture_table[Q3EGlobals.UI_P] = "btn_p.png";
		texture_table[Q3EGlobals.UI_A] = "btn_a.png";
		texture_table[Q3EGlobals.UI_S] = "btn_s.png";
		texture_table[Q3EGlobals.UI_D] = "btn_d.png";
		texture_table[Q3EGlobals.UI_G] = "btn_g.png";
		texture_table[Q3EGlobals.UI_H] = "btn_h.png";
		texture_table[Q3EGlobals.UI_J] = "btn_j.png";
		texture_table[Q3EGlobals.UI_K] = "btn_k.png";
		texture_table[Q3EGlobals.UI_L] = "btn_l.png";
		texture_table[Q3EGlobals.UI_X] = "btn_x.png";
		texture_table[Q3EGlobals.UI_V] = "btn_v.png";
		texture_table[Q3EGlobals.UI_B] = "btn_b.png";

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
		_defaultTable = new String[Q3EGlobals.UI_SIZE];
        Arrays.fill(_defaultTable, "0 0 1 30");

		_defaultPortraitTable = new String[Q3EGlobals.UI_SIZE];
		Arrays.fill(_defaultPortraitTable, "0 0 1 30");
    }

	public void InitUIDefaultLayout(Activity context)
	{
		_defaultTable = Q3EControls.GetDefaultLayout(context, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY, false);
		_defaultPortraitTable = Q3EControls.GetPortraitDefaultLayout(context, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY, false);
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

	public static String OnscreenKeyPreference(String game)
	{
		if(null == game)
			return Q3EPreference.ONSCREEN_BUTTON;
		else
			return Q3EPreference.ONSCREEN_BUTTON + "_" + game;
	}

	public static String DiscPanelKeysPreference(String game, int type)
	{
		if(null == game)
			return Q3EPreference.DISC_PANEL_KEYS_PREFIX + type;
		else
			return Q3EPreference.DISC_PANEL_KEYS_PREFIX + type + "_" + game;
	}

	public static String ControlPreference(String game)
	{
		return Q3EPreference.pref_harm_controlprefix + "_" + game;
	}

	public String OnscreenKeyPreference(boolean generic)
	{
		return OnscreenKeyPreference(generic ? null : game);
	}

	public String DiscPanelKeysPreference(boolean generic, int type)
	{
		return DiscPanelKeysPreference(generic ? null : game, type);
	}

	public String ControlPreference()
	{
		return ControlPreference(game);
	}

	// KARIN_GAME_VERSION_BOOKMARK: add/remove other game version list
	public static String[] GetGameVersions(String game)
	{
		switch(game)
		{
			case Q3EGameConstants.GAME_DOOM3BFG:
				return new String[]{
						Q3EGameConstants.GAME_VERSION_D3BFG_OPENGL,
						Q3EGameConstants.GAME_VERSION_D3BFG_VULKAN,
				};
/*			case Q3EGameConstants.GAME_REALRTCW:
				return new String[]{
						Q3EGameConstants.GAME_VERSION_REALRTCW,
						Q3EGameConstants.GAME_VERSION_REALRTCW_5_2,
				};*/
			default:
				return null;
		}
	}

	public static String GetGameModPreferenceKey(String game)
	{
		return Q3EGame.Find(game).PREF_MOD;
	}

	public static String GetGameVersionPreferenceKey(String game)
	{
		return Q3EGame.Find(game).PREF_VERSION;
	}

	public static String GetEnableModPreferenceKey(String game)
	{
		return Q3EGame.Find(game).PREF_MOD_ENABLED;
	}

	public Q3EGame GameInfo()
	{
		return Q3EGame.Find(GameID());
	}

	public static String GetGameBaseDirectory(String game)
	{
		return Q3EGame.Find(game).BASE;
	}

	public void SetAppStoragePath(Context context)
	{
		app_storage_path = Q3EContextUtils.GetAppStoragePath(context, null);
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

		if ((IsSupportSkipIntro()) && preferences.getBoolean(Q3EPreference.pref_harm_skip_intro, false))
			extraCommand = GetGameCommandEngine(extraCommand).SetCommand("disconnect", false).toString();
		if ((IsSupportQuickload()) && preferences.getBoolean(Q3EPreference.pref_harm_auto_quick_load, false))
			extraCommand = GetGameCommandEngine(extraCommand).SetParam("loadGame", "QuickSave").toString();

		if (isDOOM)
		{
			if(preferences.getBoolean(Q3EPreference.pref_harm_zdoom_load_lights_pk3, true))
				extraCommand = GetGameCommandEngine(extraCommand).AddParam("file", "lights.pk3").toString();
			if(preferences.getBoolean(Q3EPreference.pref_harm_zdoom_load_brightmaps_pk3, true))
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

		type_table[Q3EGlobals.UI_Q] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_W] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_E] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_U] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_T] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_I] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_O] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_P] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_A] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_S] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_D] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_G] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_H] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_J] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_K] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_L] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_X] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_V] = Q3EGlobals.TYPE_BUTTON;
		type_table[Q3EGlobals.UI_B] = Q3EGlobals.TYPE_BUTTON;

		_defaultType = Arrays.copyOf(type_table, type_table.length);
	}

	private static void InitDefaultArgTable()
	{
		int[] arg_table = new int[Q3EGlobals.UI_SIZE * 4];

		arg_table[Q3EGlobals.UI_SHOOT * 4] = KeyCodesGeneric.K_MOUSE1;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 3] = 0;


		arg_table[Q3EGlobals.UI_JUMP * 4] = KeyCodesGeneric.K_SPACE;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_CROUCH * 4] = KeyCodesGeneric.K_C; // BFG
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM;
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_RELOADBAR * 4] = KeyCodesGeneric.K_RBRACKET; // 93
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 1] = KeyCodesGeneric.K_R; // 114
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 2] = KeyCodesGeneric.K_LBRACKET; // 91
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 3] = Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT;

		arg_table[Q3EGlobals.UI_PDA * 4] = KeyCodesGeneric.K_TAB;
		arg_table[Q3EGlobals.UI_PDA * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_PDA * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_PDA * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4] = KeyCodesGeneric.K_F; // BFG
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_SAVE * 4] = KeyCodesGeneric.K_F5;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 1] = KeyCodesGeneric.K_ESCAPE;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 2] = KeyCodesGeneric.K_F9;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 3] = Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK;

		arg_table[Q3EGlobals.UI_1 * 4] = KeyCodesGeneric.K_F1;
		arg_table[Q3EGlobals.UI_1 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_1 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_1 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_2 * 4] = KeyCodesGeneric.K_F2;
		arg_table[Q3EGlobals.UI_2 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_2 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_2 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_3 * 4] = KeyCodesGeneric.K_F3;
		arg_table[Q3EGlobals.UI_3 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_3 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_3 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_KBD * 4] = Q3EKeyCodes.K_VKBD;
		arg_table[Q3EGlobals.UI_KBD * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_KBD * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_KBD * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_CONSOLE * 4] = KeyCodesGeneric.K_GRAVE;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_RUN * 4] = KeyCodesGeneric.K_SHIFT;
		arg_table[Q3EGlobals.UI_RUN * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_RUN * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_RUN * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_ZOOM * 4] = KeyCodesGeneric.K_Z;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_INTERACT * 4] = KeyCodesGeneric.K_MOUSE2;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_SCORE * 4] = KeyCodesGeneric.K_M;
		arg_table[Q3EGlobals.UI_SCORE * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_SCORE * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_SCORE * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_0 * 4] = KeyCodesGeneric.K_F10;
		arg_table[Q3EGlobals.UI_0 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_0 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_0 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_4 * 4] = KeyCodesGeneric.K_F4;
		arg_table[Q3EGlobals.UI_4 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_4 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_4 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_5 * 4] = KeyCodesGeneric.K_F5;
		arg_table[Q3EGlobals.UI_5 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_5 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_5 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_6 * 4] = KeyCodesGeneric.K_F6;
		arg_table[Q3EGlobals.UI_6 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_6 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_6 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_7 * 4] = KeyCodesGeneric.K_F7;
		arg_table[Q3EGlobals.UI_7 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_7 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_7 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_8 * 4] = KeyCodesGeneric.K_F8;
		arg_table[Q3EGlobals.UI_8 * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_8 * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_8 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_9 * 4] = KeyCodesGeneric.K_F9;
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

		arg_table[Q3EGlobals.UI_Y * 4] = KeyCodesGeneric.K_Y;
		arg_table[Q3EGlobals.UI_Y * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_Y * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_Y * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_N * 4] = KeyCodesGeneric.K_N;
		arg_table[Q3EGlobals.UI_N * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_N * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_N * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_PLUS * 4] = KeyCodesGeneric.K_EQUALS;
		arg_table[Q3EGlobals.UI_PLUS * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_PLUS * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_PLUS * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_MINUS * 4] = KeyCodesGeneric.K_MINUS;
		arg_table[Q3EGlobals.UI_MINUS * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_MINUS * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_MINUS * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_Q * 4] = KeyCodesGeneric.K_Q;
		arg_table[Q3EGlobals.UI_Q * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_Q * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_Q * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_W * 4] = KeyCodesGeneric.K_W;
		arg_table[Q3EGlobals.UI_W * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_W * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_W * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_E * 4] = KeyCodesGeneric.K_E;
		arg_table[Q3EGlobals.UI_E * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_E * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_E * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_U * 4] = KeyCodesGeneric.K_U;
		arg_table[Q3EGlobals.UI_U * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_U * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_U * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_T * 4] = KeyCodesGeneric.K_T;
		arg_table[Q3EGlobals.UI_T * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_T * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_T * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_I * 4] = KeyCodesGeneric.K_I;
		arg_table[Q3EGlobals.UI_I * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_I * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_I * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_O * 4] = KeyCodesGeneric.K_O;
		arg_table[Q3EGlobals.UI_O * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_O * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_O * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_P * 4] = KeyCodesGeneric.K_P;
		arg_table[Q3EGlobals.UI_P * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_P * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_P * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_A * 4] = KeyCodesGeneric.K_A;
		arg_table[Q3EGlobals.UI_A * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_A * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_A * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_S * 4] = KeyCodesGeneric.K_S;
		arg_table[Q3EGlobals.UI_S * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_S * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_S * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_D * 4] = KeyCodesGeneric.K_D;
		arg_table[Q3EGlobals.UI_D * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_D * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_D * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_G * 4] = KeyCodesGeneric.K_G;
		arg_table[Q3EGlobals.UI_G * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_G * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_G * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_H * 4] = KeyCodesGeneric.K_H;
		arg_table[Q3EGlobals.UI_H * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_H * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_H * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_J * 4] = KeyCodesGeneric.K_J;
		arg_table[Q3EGlobals.UI_J * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_J * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_J * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_K * 4] = KeyCodesGeneric.K_K;
		arg_table[Q3EGlobals.UI_K * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
		arg_table[Q3EGlobals.UI_K * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_K * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_L * 4] = KeyCodesGeneric.K_L;
		arg_table[Q3EGlobals.UI_L * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_L * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_L * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_X * 4] = KeyCodesGeneric.K_X;
		arg_table[Q3EGlobals.UI_X * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_X * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_X * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_V * 4] = KeyCodesGeneric.K_V;
		arg_table[Q3EGlobals.UI_V * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_V * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_V * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_B * 4] = KeyCodesGeneric.K_B;
		arg_table[Q3EGlobals.UI_B * 4 + 1] = Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
		arg_table[Q3EGlobals.UI_B * 4 + 2] = Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL;
		arg_table[Q3EGlobals.UI_B * 4 + 3] = 0;

		_defaultArgs = Arrays.copyOf(arg_table, arg_table.length);
	}

	public static void LoadTypeAndArgTablePreference(Context context, String game, int[] argTable, int[] typeTable, boolean makeDefault)
	{
		if(makeDefault)
			RestoreDefaultOnScreenConfig(argTable, typeTable);

		// index:type;23,1,2,0|......
		try
		{
			SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
			Set<String> configs = null;
			// load game config
			if(null != game)
				configs = preferences.getStringSet(OnscreenKeyPreference(game), null);
			// load common config
			if(null == configs)
				configs = preferences.getStringSet(OnscreenKeyPreference(null), null);

			if (null != configs && !configs.isEmpty())
			{
				for (String str : configs)
				{
					String[] subArr = str.split(":", 2);
					int index = Integer.parseInt(subArr[0]);
					subArr = subArr[1].split(";");
					typeTable[index] = Integer.parseInt(subArr[0]);
					String[] argArr = subArr[1].split(",");
					argTable[index * 4] = Integer.parseInt(argArr[0]);
					argTable[index * 4 + 1] = Integer.parseInt(argArr[1]);
					argTable[index * 4 + 2] = Integer.parseInt(argArr[2]);
					argTable[index * 4 + 3] = Integer.parseInt(argArr[3]);
				}
			}
		}
		catch (Exception e)
		{
			//UncaughtExceptionHandler.DumpException(this, Thread.currentThread(), e);
			e.printStackTrace();
			RestoreDefaultOnScreenConfig(argTable, typeTable);
		}
	}

	public void LoadTypeAndArgTablePreference(Context context)
	{
		LoadTypeAndArgTablePreference(context, game, arg_table, type_table, true);
	}

	public void LoadLayoutTablePreference(Context context, boolean portrait)
	{
		LoadLayoutTablePreference(context, game, portrait);
	}

	public void LoadLayoutTablePreference(Context context, String game, boolean portrait)
	{
		if(portrait)
			defaults_table = Arrays.copyOf(_defaultPortraitTable, _defaultPortraitTable.length);
		else
			defaults_table = Arrays.copyOf(_defaultTable, _defaultTable.length);

		try
		{
			SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
			String config = null;
			// load game config
			if(null != game)
				config = preferences.getString(ControlPreference(game), null);

			if (KStr.NotEmpty(config))
			{
				String[] configs = config.split(",");
				for(int i = 0; i < defaults_table.length; i++)
				{
					if(i >= configs.length)
						break;
					if(KStr.IsEmpty(configs[i]))
						continue;
					defaults_table[i] = configs[i];
				}
			}
			else
			{
				for(int i = 0; i < defaults_table.length; i++)
				{
					String str = preferences.getString(Q3EPreference.pref_controlprefix + i, defaults_table[i]);
					if(KStr.NotEmpty(str))
						defaults_table[i] = str;
				}
			}
		}
		catch (Exception e)
		{
			//UncaughtExceptionHandler.DumpException(this, Thread.currentThread(), e);
			e.printStackTrace();
			if(portrait)
				System.arraycopy(_defaultPortraitTable, 0, defaults_table, 0, _defaultPortraitTable.length);
			else
				System.arraycopy(_defaultTable, 0, defaults_table, 0, _defaultTable.length);
		}
	}

	public String GetGameDataDirectoryPath(String file)
	{
		return KStr.AppendPath(datadir, subdatadir, file);
	}
}
