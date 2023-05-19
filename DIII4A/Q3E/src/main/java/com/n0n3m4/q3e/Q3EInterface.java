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

public class Q3EInterface {
	
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
	public boolean multithread = false;
	public boolean function_key_toolbar = false;
	public float joystick_release_range = 0.0f;
	
	//RTCW4A:
	public final int RTCW4A_UI_ACTION=6;
	public final int RTCW4A_UI_KICK=7;
    
    //k volume key map
    public int VOLUME_UP_KEY_CODE = Q3EKeyCodes.KeyCodes.K_F3;
    public int VOLUME_DOWN_KEY_CODE = Q3EKeyCodes.KeyCodes.K_F2;
    
    public static final int VIEW_MOTION_CONTROL_TOUCH = 1;
    public static final int VIEW_MOTION_CONTROL_GYROSCOPE = 1 << 1;
    public static final int VIEW_MOTION_CONTROL_ALL = VIEW_MOTION_CONTROL_TOUCH | VIEW_MOTION_CONTROL_GYROSCOPE;

    public static final String LIB_ENGINE_ID = "libdante.so"; // DOOM3
	public static final String LIB_ENGINE_RAVEN = "libdante_raven.so"; // Quake 4
	public static final String LIB_ENGINE_HUMANHEAD = "libdante_humanhead.so"; // Prey 2006

	public static final String CONFIG_FILE_DOOM3 = "DoomConfig.cfg"; // DOOM3
	public static final String CONFIG_FILE_QUAKE4 = "Quake4Config.cfg"; // Quake 4
	public static final String CONFIG_FILE_PREY = "preyconfig.cfg"; // Prey 2006

	public static final String GAME_DOOM3 = "doom3";
	public static final String GAME_QUAKE4 = "quake4";
	public static final String GAME_PREY = "prey(2006)";

	public static final String GAME_NAME_DOOM3 = "DOOM 3";
	public static final String GAME_NAME_QUAKE4 = "Quake 4";
	public static final String GAME_NAME_PREY = "Prey(2006)";

	public static final String GAME_BASE_DOOM3 = "base";
	public static final String GAME_BASE_QUAKE4 = "q4base";
	public static final String GAME_BASE_PREY = "preybase"; // Other platform is `base`

	public static final String[] LIBS = {
			"game",
			"d3xp",
			"cdoom",
			"d3le",
			"rivensin",
			"hardcorps",
	};
	public static final String[] Q4_LIBS = {
			"q4game",
	};
	public static final String[] PREY_LIBS = {
			"preygame",
	};

	public static final String[] QUAKE4_MAPS = {
			"airdefense1",
			"airdefense2",
			"hangar1",
			"hangar2",
			"mcc_landing",
			"mcc_1",
			"convoy1",
			"building_b",
			"convoy2",
			"convoy2b",
			"hub1",
			"hub2",
			"medlabs",
			"walker",
			"dispersal",
			"recomp",
			"putra",
			"waste",
			"mcc_2",
			"storage1 first",
			"storage2",
			"storage1 second",
			"tram1",
			"tram1b",
			"process1 first",
			"process2",
			"process1 second",
			"network1",
			"network2",
			"core1",
			"core2",
	};

	public static final String[] QUAKE4_LEVELS = {
			"AIR DEFENSE BUNKER", // Act I
			"AIR DEFENSE TRENCHES",
			"HANGAR PERIMETER",
			"INTERIOR HANGAR",
			"MCC LANDING SITE",
			"OPERATION: ADVANTAGE", // Act II
			"CANYON",
			"PERIMETER DEFENSE STATION",
			"AQUEDUCTS",
			"AQUEDUCTS ANNEX",
			"NEXUS HUB TUNNELS",
			"NEXUS HUB",
			"STROGG MEDICAL FACILITIES", // Act III
			"CONSTRUCTION ZONE",
			"DISPERSAL FACILITY",
			"RECOMPOSITION CENTER",
			"PUTRIFICATION CENTER",
			"WASTE PROCESSING FACILITY",
			"OPERATION: LAST HOPE", // Act IV
			"DATA STORAGE TERMINAL",
			"DATA STORAGE SECURITY",
			"DATA STORAGE TERMINAL",
			"TRAM HUB STATION",
			"TRAM RAIL",
			"DATA PROCESSING TERMINAL",
			"DATA PROCESSING SECURITY",
			"DATA PROCESSING TERMINAL",
			"DATA NETWORKING TERMINAL",
			"DATA NETWORKING SECURITY",
			"NEXUS CORE", // Act V
			"THE NEXUS",
	};

	public String EngineLibName()
	{
		if(isPrey)
			return LIB_ENGINE_HUMANHEAD;
		else if(isQ4)
			return LIB_ENGINE_RAVEN;
		else
			return LIB_ENGINE_ID;
	}

	public String ConfigFileName()
	{
		if(isPrey)
			return CONFIG_FILE_PREY;
		else if(isQ4)
			return CONFIG_FILE_QUAKE4;
		else
			return CONFIG_FILE_DOOM3;
	}

	public String GameName()
	{
		if(isPrey)
			return GAME_NAME_PREY;
		else if(isQ4)
			return GAME_NAME_QUAKE4;
		else
			return GAME_NAME_DOOM3;
	}

	public String GameType()
	{
		if(isPrey)
			return GAME_PREY;
		else if(isQ4)
			return GAME_QUAKE4;
		else
			return GAME_DOOM3;
	}

	public String GameBase()
	{
		if(isPrey)
			return GAME_BASE_PREY;
		else if(isQ4)
			return GAME_BASE_QUAKE4;
		else
			return GAME_BASE_DOOM3;
	}

	public String[] GameLibs()
	{
		if(isPrey)
			return PREY_LIBS;
		else if(isQ4)
			return Q4_LIBS;
		else
			return LIBS;
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
		if(GAME_PREY.equalsIgnoreCase(name))
			SetupPrey();
		else if(GAME_QUAKE4.equalsIgnoreCase(name))
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
}
