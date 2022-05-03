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

import java.io.BufferedReader;
import java.io.FileReader;

public class Q3EJNI {	
	public static native void setCallbackObject(Object obj);	
	public static native void init(String LibPath,int width, int height, String GameDir, String Args);	
	public static native void drawFrame();
	public static native void sendKeyEvent(int state,int key,int character);
	public static native void sendAnalog(int enable,float x,float y);
	public static native void sendMotionEvent(float x, float y);	
	public static native void requestAudioData();
	public static native void vidRestart();
	
	public static boolean detectNeon()
	{
		try
		{
		BufferedReader br=new BufferedReader(new FileReader("/proc/cpuinfo"));
		String l;
		while ((l=br.readLine())!=null)
			if ((l.contains("Features")) && (l.contains("neon"))) 
				{
				br.close();
				return true;		
				}
		br.close();
		return false;
		}
		catch (Exception e){return false;}
	}

	static {
		System.loadLibrary("q3eloader");
	}
}

