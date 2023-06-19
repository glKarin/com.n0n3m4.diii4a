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
import android.os.Environment;

import java.io.File;
import java.util.Arrays;

public class Q3EInterface
{
	private static int[] _defaultArgs;
	private static int[] _defaultType;
	static {
		InitDefaultTypeTable();
		InitDefaultArgTable();
	}
	
	public int UI_SIZE;
	public String[] defaults_table;
	public String[] texture_table;
	public int[] type_table;
	public int[] arg_table; //key,key,key,style or key,canbeheld,style,null	

	public boolean isRTCW=false;
	public boolean isQ1=false;
	public boolean isQ2=false;
	public boolean isQ3=false;
	public boolean isD3=false;	
	public boolean isD3BFG=false;
    public boolean isQ4 = false;
	public boolean isPrey = false;
	
	public String default_path;
	
	public String libname;
	public String config_name;
	public String game;
	public String game_name;
	public String game_base;
	public String[] libs;

	public Q3ECallbackObj callbackObj;
    
    public boolean view_motion_control_gyro = false;
    public String start_temporary_extra_command = "";
	public String cmd = "game.arm";
	public boolean multithread = false;
	public boolean function_key_toolbar = false;
	public float joystick_release_range = 0.0f;
	public float joystick_inner_dead_zone = 0.0f;
	public boolean joystick_unfixed = false;
	public boolean joystick_smooth = true;

	public String app_storage_path;
	
	//RTCW4A:
	public final int RTCW4A_UI_ACTION=6;
	public final int RTCW4A_UI_KICK=7;

	//k volume key map
	public int VOLUME_UP_KEY_CODE = Q3EKeyCodes.KeyCodes.K_F3;
	public int VOLUME_DOWN_KEY_CODE = Q3EKeyCodes.KeyCodes.K_F2;

	public String EngineLibName()
	{
		if(isPrey)
			return Q3EGlobals.LIB_ENGINE_HUMANHEAD;
		else if(isQ4)
			return Q3EGlobals.LIB_ENGINE_RAVEN;
		else
			return Q3EGlobals.LIB_ENGINE_ID;
	}

	public String ConfigFileName()
	{
		if(isPrey)
			return Q3EGlobals.CONFIG_FILE_PREY;
		else if(isQ4)
			return Q3EGlobals.CONFIG_FILE_QUAKE4;
		else
			return Q3EGlobals.CONFIG_FILE_DOOM3;
	}

	public String GameName()
	{
		if(isPrey)
			return Q3EGlobals.GAME_NAME_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_NAME_QUAKE4;
		else
			return Q3EGlobals.GAME_NAME_DOOM3;
	}

	public String GameType()
	{
		if(isPrey)
			return Q3EGlobals.GAME_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_QUAKE4;
		else
			return Q3EGlobals.GAME_DOOM3;
	}

	public String GameBase()
	{
		if(isPrey)
			return Q3EGlobals.GAME_BASE_PREY;
		else if(isQ4)
			return Q3EGlobals.GAME_BASE_QUAKE4;
		else
			return Q3EGlobals.GAME_BASE_DOOM3;
	}

	public String[] GameLibs()
	{
		if(isPrey)
			return Q3EGlobals.PREY_LIBS;
		else if(isQ4)
			return Q3EGlobals.Q4_LIBS;
		else
			return Q3EGlobals.LIBS;
	}

	public void SetupEngineLib()
	{
		libname = EngineLibName();
	}

	private void SetupConfigFile()
	{
		config_name = ConfigFileName();
	}

	private void SetupGameTypeAndName()
	{
		game = GameType();
		game_name = GameName();
		game_base = GameBase();
	}

	private void SetupGameLibs()
	{
		libs = GameLibs();
	}

	public void SetupGame(String name)
	{
		if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
			SetupPrey();
		else if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
			SetupQuake4();
		else
			SetupDOOM3();
	}

	public void SetupDOOM3()
	{
		isD3 = true;
		isPrey = false;
		isQ4 = false;
		SetupGameTypeAndName();
		SetupEngineLib();
		SetupGameLibs();
		SetupConfigFile();
	}

	public void SetupPrey()
	{
		isD3 = true;
		isQ4 = false;
		isPrey = true;
		SetupGameTypeAndName();
		SetupEngineLib();
		SetupGameLibs();
		SetupConfigFile();
	}

	public void SetupQuake4()
	{
		isD3 = true;
		isPrey = false;
		isQ4 = true;
		SetupGameTypeAndName();
		SetupEngineLib();
		SetupGameLibs();
		SetupConfigFile();
    }

    public void InitTextureTable()
    {
        texture_table = new String[Q3EGlobals.UI_SIZE];

        texture_table[Q3EGlobals.UI_JOYSTICK] = "joystick_bg.png;joystick_center.png"; // "";
        texture_table[Q3EGlobals.UI_SHOOT] = "btn_sht.png";
        texture_table[Q3EGlobals.UI_JUMP] = "btn_jump.png";
        texture_table[Q3EGlobals.UI_CROUCH] = "btn_crouch.png";
        texture_table[Q3EGlobals.UI_RELOADBAR] = "btn_reload.png";
        texture_table[Q3EGlobals.UI_PDA] = "btn_pda.png";
        texture_table[Q3EGlobals.UI_FLASHLIGHT] = "btn_flashlight.png";
        texture_table[Q3EGlobals.UI_SAVE] = "btn_pause.png";
        texture_table[Q3EGlobals.UI_1] = "btn_1.png";
        texture_table[Q3EGlobals.UI_2] = "btn_2.png";
        texture_table[Q3EGlobals.UI_3] = "btn_3.png";
        texture_table[Q3EGlobals.UI_KBD] = "btn_keyboard.png";
        texture_table[Q3EGlobals.UI_CONSOLE] = "btn_notepad.png";
        texture_table[Q3EGlobals.UI_INTERACT] = "btn_activate.png";
        texture_table[Q3EGlobals.UI_ZOOM] = "btn_binocular.png";
        texture_table[Q3EGlobals.UI_RUN] = "btn_kick.png";

        texture_table[Q3EGlobals.UI_WEAPON_PANEL] = "disc_weapon.png";
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
        isD3BFG = true;
		InitTable();
    }

    public boolean IsInitGame()
	{
		return isD3 || isD3BFG || isQ2 || isQ1 || isQ3 || isRTCW;
	}

	public void SetAppStoragePath(Context context)
	{
		Q3EUtils.q3ei.app_storage_path = Q3EUtils.GetAppStoragePath(context, null);
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

		_defaultType = Arrays.copyOf(type_table, type_table.length);
	}

	private static void InitDefaultArgTable()
	{
		int[] arg_table = new int[Q3EGlobals.UI_SIZE * 4];

		arg_table[Q3EGlobals.UI_SHOOT * 4] = Q3EKeyCodes.KeyCodes.K_MOUSE1;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_SHOOT * 4 + 3] = 0;


		arg_table[Q3EGlobals.UI_JUMP * 4] = Q3EKeyCodes.KeyCodes.K_SPACE;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_JUMP * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_CROUCH * 4] = Q3EKeyCodes.KeyCodesD3.K_C; // BFG
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 1] = 1;
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 2] = 1;
		arg_table[Q3EGlobals.UI_CROUCH * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_RELOADBAR * 4] = Q3EKeyCodes.KeyCodesD3.K_BRACKET_RIGHT; // 93
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 1] = Q3EKeyCodes.KeyCodesD3.K_R; // 114
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 2] = Q3EKeyCodes.KeyCodesD3.K_BRACKET_LEFT; // 91
		arg_table[Q3EGlobals.UI_RELOADBAR * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_PDA * 4] = Q3EKeyCodes.KeyCodes.K_TAB;
		arg_table[Q3EGlobals.UI_PDA * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_PDA * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_PDA * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4] = Q3EKeyCodes.KeyCodesD3.K_F; // BFG
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_FLASHLIGHT * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_SAVE * 4] = Q3EKeyCodes.KeyCodes.K_F5;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 1] = Q3EKeyCodes.KeyCodes.K_ESCAPE;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 2] = Q3EKeyCodes.KeyCodes.K_F9;
		arg_table[Q3EGlobals.UI_SAVE * 4 + 3] = 1;

		arg_table[Q3EGlobals.UI_1 * 4] = Q3EKeyCodes.KeyCodesD3BFG.K_1;
		arg_table[Q3EGlobals.UI_1 * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_1 * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_1 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_2 * 4] = Q3EKeyCodes.KeyCodesD3BFG.K_2;
		arg_table[Q3EGlobals.UI_2 * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_2 * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_2 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_3 * 4] = Q3EKeyCodes.KeyCodesD3BFG.K_3;
		arg_table[Q3EGlobals.UI_3 * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_3 * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_3 * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_KBD * 4] = Q3EKeyCodes.K_VKBD;
		arg_table[Q3EGlobals.UI_KBD * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_KBD * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_KBD * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_CONSOLE * 4] = Q3EKeyCodes.KeyCodesD3.K_CONSOLE;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_CONSOLE * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_RUN * 4] = Q3EKeyCodes.KeyCodesD3.K_SHIFT;
		arg_table[Q3EGlobals.UI_RUN * 4 + 1] = 1;
		arg_table[Q3EGlobals.UI_RUN * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_RUN * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_ZOOM * 4] = Q3EKeyCodes.KeyCodesD3.K_Z;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 1] = 1;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_ZOOM * 4 + 3] = 0;

		arg_table[Q3EGlobals.UI_INTERACT * 4] = Q3EKeyCodes.KeyCodesD3.K_MOUSE2;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 1] = 0;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 2] = 0;
		arg_table[Q3EGlobals.UI_INTERACT * 4 + 3] = 0;

		_defaultArgs = Arrays.copyOf(arg_table, arg_table.length);
	}

	public String GetGameModPreferenceKey()
	{
		return isPrey ? Q3EPreference.pref_harm_prey_fs_game
				: (isQ4 ? Q3EPreference.pref_harm_q4_fs_game
				: Q3EPreference.pref_harm_fs_game);
	}

	public String GetGameModLibPreferenceKey()
	{
		return isPrey ? Q3EPreference.pref_harm_prey_game_lib
				: (isQ4 ? Q3EPreference.pref_harm_q4_game_lib
				: Q3EPreference.pref_harm_game_lib);
	}
}
