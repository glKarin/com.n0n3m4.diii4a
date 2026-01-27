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

import android.os.Environment;
import android.util.Log;
import android.view.Surface;

import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;

public class Q3EJNI {	
	public static native void setCallbackObject(Object obj);

    public static native boolean init(
            String LibPath, // engine's library file path
            String nativeLibPath, // apk's dynamic library directory path
            int width, // surface width
            int height, // surface height
            String GameDir, // game data directory(external)
			String gameSubDir, // game data sub directory(external)
            String Args, // doom3 command line arguments
            Surface view, // render surface
            int format, // OpenGL color buffer format: 0x8888, 0x4444, 0x5551, 0x565
			int depth, // OpenGL depth buffer bits: 24 16 32
            int msaa, // MSAA: 0, 4, 16
            int glVersion, // OpenGLES verison: 0x00020000, 0x00030000
            boolean redirect_output_to_file, // save runtime log to file
            int no_handle_signals, // not handle signals
            boolean usingMouse, // using mouse
			int refreshRate, // refresh rate,
			String appHome, // app home path
			boolean smoothJoystick, // is smooth joystick
			int consoleMaxHeightFrac, // max console height frac(0 - 100)
			boolean usingExternalLibs, // using extern libraries
			int sdlAudioDriver, // SDL audio driver: 0, 1, 2
            boolean continueNoGLContext
    );
	public static native void sendKeyEvent(int state,int key,int character);
	public static native void sendAnalog(int enable,float x,float y);
	public static native void sendMotionEvent(float x, float y);
	public static native void sendMouseEvent(float x, float y, int relativeMode);
	public static native void sendTextEvent(String text);
	public static native void sendCharEvent(int ch);
	public static native void sendWheelEvent(float x, float y);
    public static native void shutdown();
    public static native boolean Is64();
    public static native void OnPause();
    public static native void OnResume();
    public static native void SetSurface(Surface view);
	public static native void PushKeyEvent(int state, int key, int character);
	public static native void PushAnalogEvent(int enable, float x, float y);
	public static native void PushMotionEvent(float x, float y);
	public static native void PushMouseEvent(float x, float y, int relativeMode);
	public static native void PushTextEvent(String text);
	public static native void PushCharEvent(int ch);
	public static native void PushWheelEvent(float x, float y);
	public static native void PreInit(int eventQueueType, int gameThreadType, int stackSize);
	public static native int main();
	public static native long StartThread();
	public static native void StopThread();
	public static native void NotifyExit();
	public static native int Setenv(String name, String value, int override);
	public static native int AlignedStackSize(int kb);

	static {
		/*
		boolean loaded = false;
		StringBuilder buf = new StringBuilder();
		String localPath = "/sdcard";
		String localLibPath = KStr.AppendPath(localPath, "diii4a", "libq3eloader.so");
		File file = new File(localLibPath);
		try
		{
			if(file.isFile())
			{
				Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Find local q3eloader library at " + localLibPath + "......");
				String cacheFile = Q3E.CopyDLLToCache(localLibPath,  "lib", null);
				buf.append("Local q3eloader library file path: ").append(localLibPath).append(" to ").append(cacheFile).append("\n");
				if(null != cacheFile)
				{
					Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Load local q3eloader library: " + cacheFile);
					System.load(cacheFile);
					loaded = true;
				}
				else
				{
					Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "Upload local q3eloader library fail: " + localLibPath);
				}
			}
		}
		catch(Throwable e)
		{
		}
		if(!loaded)
		{
		 */
			System.loadLibrary("q3eloader");
			/*
		}

		{
			try
			{
				FileWriter os = new FileWriter(KStr.AppendPath(localPath, "diii4a", "q3eloader.txt"));
				os.write(buf.toString());
				os.flush();
				os.close();
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}

		}*/
	}
}

