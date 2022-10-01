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
	
	public String default_path;
	
	public String libname;
	
	//RTCW4A:
	public final int RTCW4A_UI_ACTION=6;
	public final int RTCW4A_UI_KICK=7;
    
    //k volume key map
    public int VOLUME_UP_KEY_CODE = Q3EKeyCodes.KeyCodes.K_F3;
    public int VOLUME_DOWN_KEY_CODE = Q3EKeyCodes.KeyCodes.K_F2;
}
