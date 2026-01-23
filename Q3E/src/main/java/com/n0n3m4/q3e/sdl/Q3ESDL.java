package com.n0n3m4.q3e.sdl;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.karin.KLog;

public final class Q3ESDL
{
	// SDLActivity
	public static native int nativeSetupJNI();
	public static native void onNativeKeyDown(int keycode);
	public static native void onNativeKeyUp(int keycode);
	//public static native void onNativeOrientationChanged(int orientation);
	public static native void nativeAddTouch(int touchId, String name);
	public static native void onNativeMouse(int button, int action, float x, float y, boolean relative);

	// SDLControllerManager
	public static native int controller_nativeSetupJNI();
	public static native int onNativePadDown(int device_id, int keycode);
	public static native int onNativePadUp(int device_id, int keycode);
	public static native int nativeAddJoystick(int device_id, String name, String desc,
											   int vendor_id, int product_id,
											   boolean is_accelerometer, int button_mask,
											   int naxes, int axis_mask, int nhats, int nballs);
	public static native int nativeRemoveJoystick(int device_id);
	public static native int nativeAddHaptic(int device_id, String name);
	public static native int nativeRemoveHaptic(int device_id);
	public static native void onNativeJoy(int device_id, int axis,
										  float value);
	public static native void onNativeHat(int device_id, int hat_id,
										  int x, int y);

	// SDLInputConnection
	public static native void nativeCommitText(String text, int newCursorPosition);

	public static native boolean UsingSDL();

	public static boolean usingSDL = false;

	private static SDLActivity activity;
	public static void InitSDL()
	{
		if(null != activity)
			return;

		activity = new SDLActivity();
		Q3E.runOnUiThread(new Runnable() {
			@Override
			public void run()
			{
				KLog.I("[Java]: Init SDL2");
				activity.onCreate(null);
			}
		});
	}
}

