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

import android.view.Surface;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class Q3EJNI {	
	public static native void setCallbackObject(Object obj);

    public static native void init(
            String LibPath, // engine's library file path
            String nativeLibPath, // apk's dynamic library directory path
            int width, // surface width
            int height, // surface height
            String GameDir, // game data directory(external)
            String Args, // doom3 command line arguments
            Surface view, // render surface
            int format, // OpenGL color buffer format: 0x8888, 0x4444, 0x5551, 0x565
            int msaa, // MSAA: 0, 4, 16
            int glVersion, // OpenGLES verison: 0x00020000, 0x00030000
            boolean redirect_output_to_file, // save runtime log to file
            boolean no_handle_signals, // not handle signals
            boolean multithread, // enable multithread
            boolean continueNoGLContext
    );
	public static native void drawFrame();
	public static native void sendKeyEvent(int state,int key,int character);
	public static native void sendAnalog(int enable,float x,float y);
	public static native void sendMotionEvent(float x, float y);
	public static native void vidRestart();
    public static native void shutdown();
    public static native boolean Is64();
    public static native void OnPause();
    public static native void OnResume();
    public static native void SetSurface(Surface view);

    public static boolean IS_NEON = false; // only armv7-a 32. arm64 always support, but using hard
    public static boolean IS_64 = false;
    public static boolean SYSTEM_64 = false;
    public static String ARCH = "";
    private static boolean _is_detected = false;
    
    private static boolean GetCpuInfo()
    {
        if (_is_detected)
            return true;
        IS_64 = Is64();
        ARCH = IS_64 ? "aarch64" : "arm";
        BufferedReader br = null;
        try
        {
            br = new BufferedReader(new FileReader("/proc/cpuinfo"));
            String l;
            while ((l = br.readLine()) != null)
            {
                if ((l.contains("Features")) && (l.contains("neon")))
                {
                    IS_NEON = true;
                }
                if (l.contains("Processor") && (l.contains("AArch64")))
                {
                    SYSTEM_64 = true;
                    IS_NEON = true;
                }

            }
            _is_detected = true;
        } catch (Exception e)
        {
            e.printStackTrace();
            _is_detected = false;
        } finally
        {
            try
            {
                if (br != null)
                    br.close();
            } catch (IOException ioe)
            {
                ioe.printStackTrace();
            }
        }
        return _is_detected;
    }

	static {
		System.loadLibrary("q3eloader");
        GetCpuInfo();
	}
}

