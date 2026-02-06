package com.fteqw;

public class FTEDroidEngine
{
	public static native void init(float dpix, float dpiy, String apkpath, String usrpath); /* init/reinit */
	public static native int frame();
	public static native int openfile(String filename);
	public static native int getvibrateduration();	//in ms
	public static native int keypress(int down, int qkey, int unicode);
	public static native void motion(int act, int pointerid, float x, float y, float size);
	public static native void accelerometer(float ax, float ay, float az);
	public static native void gyroscope(float gx, float gy, float gz);
	public static native int paintaudio(byte[] stream, int len);
	public static native int audioinfo(int arg);
	public static native String geterrormessage();
	public static native String getpreferedorientation();
	public static native void setwindow(android.view.Surface window);
	public static native void windowresized(int w, int h);

	static
	{
			System.loadLibrary("ftedroid");
	}
}
