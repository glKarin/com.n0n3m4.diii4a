package com.fteqw;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.InputDevice;
import android.view.WindowManager;

public class FTENativeActivity extends android.app.Activity implements android.view.SurfaceHolder.Callback2, android.view.ViewTreeObserver.OnGlobalLayoutListener
{
	//Native functions and stuff
	private native boolean startup(String externalDataPath, String libraryPath);
	private native void openfile(String url);
	private native void surfacechange(boolean teardown, boolean restart, android.view.SurfaceHolder holder, android.view.Surface surface);
	private native void shutdown();

	private static native void keypress(int devid, boolean down, int androidkey, int unicode);
	private static native void mousepress(int devid, int buttonbits);
	private static native void motion(int devid, int action, float x, float y, float z, float size);
	private static native boolean wantrelative();
	private static native void axis(int devid, int axisid, float value);
//	private static native void oncreate(String bindir, String basedir, byte[] savedstate);
	static
	{
		System.loadLibrary("ftedroid");	//registers the methods properly.
	}

	static class NativeContentView extends android.view.View
	{
		FTENativeActivity mActivity;
		public NativeContentView(android.content.Context context)
		{
			super(context);
		}
		public NativeContentView(android.content.Context context, android.util.AttributeSet attrs)
		{
			super(context, attrs);
		}
	}
	private NativeContentView mNativeContentView;

//SurfaceHolder.Callback2 methods
	public void surfaceRedrawNeeded(android.view.SurfaceHolder holder)
	{	//we constantly redraw.
	}
	public void surfaceCreated(android.view.SurfaceHolder holder)
	{
//		surfacechange(false, true, holder.getSurface());
	}
	public void surfaceChanged(android.view.SurfaceHolder holder, int format, int width, int height)
	{
		surfacechange(true, true, holder, holder.getSurface());
	}
	public void surfaceDestroyed(android.view.SurfaceHolder holder)
	{
		surfacechange(true, false, null, null);
	}

//OnGlobalLayoutListener methods
    public void onGlobalLayout()
	{
/*		mNativeContentView.getLocationInWindow(mLocation);
		int w = mNativeContentView.getWidth();
		int h = mNativeContentView.getHeight();
		if (mLocation[0] != mLastContentX || mLocation[1] != mLastContentY || w != mLastContentWidth || h != mLastContentHeight)
		{
			mLastContentX = mLocation[0];
			mLastContentY = mLocation[1];
			mLastContentWidth = w;
			mLastContentHeight = h;
			if (!mDestroyed) {
				onContentRectChangedNative(mNativeHandle, mLastContentX,
						mLastContentY, mLastContentWidth, mLastContentHeight);
			}
		}
*/	}

//Activity methods
	@Override
	protected void onCreate(android.os.Bundle savedInstanceState)
	{
		getWindow().takeSurface(this);
		getWindow().setFormat(android.graphics.PixelFormat.RGB_565);
		getWindow().setSoftInputMode(
				WindowManager.LayoutParams.SOFT_INPUT_STATE_UNSPECIFIED
				| WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
		mNativeContentView = new NativeContentView(this);
		mNativeContentView.mActivity = this;
		setContentView(mNativeContentView);
		mNativeContentView.requestFocus();
		mNativeContentView.getViewTreeObserver().addOnGlobalLayoutListener(this);

//		byte[] nativeSavedState = savedInstanceState != null
//			? savedInstanceState.getByteArray(KEY_NATIVE_SAVED_STATE) : null;
		startup(getAbsolutePath(getExternalFilesDir(null)), getNativeLibraryDirectory());
		handleIntent(getIntent());

//		if (Build.VERSION.SDK_INT >= 19)
//		{
//			int flags = 0;
//			flags |= 4096/*SYSTEM_UI_FLAG_IMMERSIVE_STICKY, api 19*/;
//			flags |= 4/*SYSTEM_UI_FLAG_FULLSCREEN, api 16*/;
//			flags |= 2/*SYSTEM_UI_FLAG_HIDE_NAVIGATION, api 14*/;			
//			mNativeContentView.setSystemUiVisibility(flags); /*api 11*/
//		}

		super.onCreate(savedInstanceState);
	}
	
	//random helpers
	private void handleIntent(android.content.Intent intent)
	{
		String s = intent.getScheme();
		if (s=="content")
		{
			android.database.Cursor cursor = this.getContentResolver().query(intent.getData(), null, null, null, null);
			cursor.moveToFirst();   
			String myloc = cursor.getString(0);
			cursor.close();
		}
		else
			openfile(intent.getDataString());
	}
	private static String getAbsolutePath(java.io.File file)
	{
		return (file != null) ? file.getAbsolutePath() : null;
	}
	public String getNativeLibraryDirectory()
	{
		android.content.Context context = getApplicationContext();
		int sdk_level = android.os.Build.VERSION.SDK_INT;
		if (sdk_level >= android.os.Build.VERSION_CODES.GINGERBREAD)
		{
			try
			{
				String secondary = (String) android.content.pm.ApplicationInfo.class.getField("nativeLibraryDir").get(context.getApplicationInfo());
				return secondary;
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			return null;
		}
		if (sdk_level >= android.os.Build.VERSION_CODES.DONUT)
			return context.getApplicationInfo().dataDir + "/lib";
		return "/data/data/" + context.getPackageName() + "/lib";
	}

	//called by C code on errors / quitting.
	public void showMessageAndQuit(String errormessage)
	{
		final android.app.Activity act = this;
		final String errormsg = errormessage;
		if (errormsg.equals(""))
		{	//just quit
			finish();
			System.exit(0);
		}
		else runOnUiThread(new Runnable()
		{	//show an error message, then quit.
			public void run()
			{
//				act.getView().setVisibility(android.view.View.GONE);
				android.app.AlertDialog ad = new android.app.AlertDialog.Builder(act).create();
				ad.setTitle("Fatal Error");
				ad.setMessage(errormsg);
				ad.setCancelable(false);
				ad.setButton("Ok", new android.content.DialogInterface.OnClickListener()
				{
					public void onClick(android.content.DialogInterface dialog, int which)
					{
						finish();
						System.exit(0);
					}
				});
				ad.show();
			}
		});
	}
	public void updateScreenKeepOn(final boolean keepon)
	{
		final android.app.Activity act = this;
		runOnUiThread(new Runnable()
		{
			public void run()
			{
				if (keepon)
					act.getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
				else
					act.getWindow().setFlags(0, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
			}
		});
	}


	//called by C code to set orientation.
	public void updateOrientation(String orientation)
	{
		final String ors = orientation;
		runOnUiThread(new Runnable()
		{
			public void run()
			{
				int ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR;
				if (ors.equalsIgnoreCase("unspecified"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
				else if (ors.equalsIgnoreCase("landscape"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
				else if (ors.equalsIgnoreCase("portrait"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
				else if (ors.equalsIgnoreCase("user"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_USER;
				else if (ors.equalsIgnoreCase("behind"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_BEHIND;
				else if (ors.equalsIgnoreCase("sensor"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR;
				else if (ors.equalsIgnoreCase("nosensor"))
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_NOSENSOR;
				//the following are api level 9+
				else if (ors.equalsIgnoreCase("sensorlandscape"))
					ori = 6;//android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
				else if (ors.equalsIgnoreCase("sensorportrait"))
					ori = 7;//android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
				else if (ors.equalsIgnoreCase("reverselandscape"))
					ori = 8;//android.content.pm.ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
				else if (ors.equalsIgnoreCase("reverseportrait"))
					ori = 9;//android.content.pm.ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
				else if (ors.equalsIgnoreCase("fullsensor"))
					ori = 10;//android.content.pm.ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
				//and the default, because specifying it again is always useless.
				else
					ori = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR;
				android.util.Log.i("FTEDroid", "Orientation changed to " + ori + " (" + ors + ").");
				setRequestedOrientation(ori);
			}
		});
	};


	//keyboard stuff, called from C.
	public void showKeyboard(int softkeyflags)
	{	//needed because the ndk's ANativeActivity_showSoftInput is defective
		final android.app.Activity act = this;
		final int flags = softkeyflags;
		runOnUiThread(new Runnable()
		{
			public void run()
			{
				if (flags != 0)
				{
					InputMethodManager imm = (InputMethodManager)getSystemService(android.content.Context.INPUT_METHOD_SERVICE);
					imm.showSoftInput(act.getWindow().getDecorView(), InputMethodManager.SHOW_FORCED);
				}
				else
				{
					InputMethodManager imm = (InputMethodManager)getSystemService(android.content.Context.INPUT_METHOD_SERVICE);
					imm.hideSoftInputFromWindow(act.getWindow().getDecorView().getWindowToken(), 0);
				}
			}
		});
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event)
	{	//needed because AKeyEvent_getUnicode is missing completely.
		int act = event.getAction();
		if (act == KeyEvent.ACTION_DOWN)
		{
			int metastate = event.getMetaState();
			int unichar = event.getUnicodeChar(metastate);
			if (unichar == 0)
				unichar = event.getUnicodeChar();
			if (unichar == 0)
				unichar = event.getDisplayLabel();

			keypress(event.getDeviceId(), true, event.getKeyCode(), unichar);
			return true;
		}
		else if (act == KeyEvent.ACTION_UP)
		{
			keypress(event.getDeviceId(), false, event.getKeyCode(), 0);
			return true;
		}
		else
			android.util.Log.i("FTEDroid", "other type of event");
		//ignore ACTION_MULTIPLE or whatever it is, apparently its deprecated anyway.

		return super.dispatchKeyEvent(event);
	}

	private static boolean canrelative;
	private static int AXIS_RELATIVE_X;//MotionEvent 24
	private static int AXIS_RELATIVE_Y;//MotionEvent 24
	private static java.lang.reflect.Method MotionEvent_getAxisValueP; //MotionEvent 12
	private static int SOURCE_MOUSE = 0x00002002;	//InputDevice 9
	//private static int SOURCE_STYLUS = 0x00004002;	//InputDevice 14
	//private static int SOURCE_STYLUS = 0x00004002;	//InputDevice 14
	private static int SOURCE_MOUSE_RELATIVE = 0x00020004;	//InputDevice 26
	private static boolean canbuttons;
	private static java.lang.reflect.Method MotionEvent_getButtonState; //MotionEvent 14

	private static boolean canjoystick;
	private static java.lang.reflect.Method MotionEvent_getAxisValueJ;
	private static java.lang.reflect.Method InputDevice_getMotionRange;
	private static int SOURCE_JOYSTICK = 0x01000010; //InputDevice 12
	private static int SOURCE_GAMEPAD = 0x00000401; //InputDevice 12
	private static int AXIS_X;
	private static int AXIS_Y;
	private static int AXIS_LTRIGGER;
	private static int AXIS_Z;
	private static int AXIS_RZ;
	private static int AXIS_RTRIGGER;
	static
	{
		//if (android.os.Build.VERSION.SDK_INT >= 12)
		try
		{
			MotionEvent_getAxisValueP = MotionEvent.class.getMethod("getAxisValue", int.class, int.class); //api12
			java.lang.reflect.Field relX = MotionEvent.class.getField("AXIS_RELATIVE_X");	//api24ish
			java.lang.reflect.Field relY = MotionEvent.class.getField("AXIS_RELATIVE_Y");	//api24ish
			AXIS_RELATIVE_X = (Integer)relX.get(null);
			AXIS_RELATIVE_Y = (Integer)relY.get(null);
//			SOURCE_MOUSE = (Integer)InputDevice.class.getField("SOURCE_MOUSE").get(null);
//			SOURCE_MOUSE_RELATIVE = (Integer)InputDevice.class.getField("SOURCE_MOUSE_RELATIVE").get(null);
			canrelative = true;	//yay, no exceptions.
			android.util.Log.i("FTEDroid", "relative mouse supported");

			MotionEvent_getButtonState = MotionEvent.class.getMethod("getButtonState");		//api14ish
			canbuttons = true;
			android.util.Log.i("FTEDroid", "mouse buttons supported");
		} catch(Exception e) {
			canrelative = false;
			android.util.Log.i("FTEDroid", "relative mouse not supported");
		}
		try
		{
			MotionEvent_getAxisValueJ = MotionEvent.class.getMethod("getAxisValue", int.class); //api12
			InputDevice_getMotionRange = InputDevice.class.getMethod("getMotionRange", int.class); //api12
			AXIS_X = (Integer)MotionEvent.class.getField("AXIS_X").get(null);
			AXIS_Y = (Integer)MotionEvent.class.getField("AXIS_Y").get(null);
			AXIS_LTRIGGER = (Integer)MotionEvent.class.getField("AXIS_LTRIGGER").get(null);
			AXIS_Z = (Integer)MotionEvent.class.getField("AXIS_Z").get(null);
			AXIS_RZ = (Integer)MotionEvent.class.getField("AXIS_RZ").get(null);
			AXIS_RTRIGGER = (Integer)MotionEvent.class.getField("AXIS_RTRIGGER").get(null);
//			SOURCE_JOYSTICK = (Integer)InputDevice.class.getField("SOURCE_JOYSTICK").get(null);
//			SOURCE_GAMEPAD = (Integer)InputDevice.class.getField("SOURCE_GAMEPAD").get(null);
			canjoystick = true;
			android.util.Log.i("FTEDroid", "gamepad supported");
		} catch(Exception e) {
			canjoystick = false;
			android.util.Log.i("FTEDroid", "gamepad not supported");
		}
	}
	private static void handleJoystickAxis(MotionEvent event, InputDevice dev, int aaxis, int qaxis)
	{
		try
		{
			final InputDevice.MotionRange range = (InputDevice.MotionRange)InputDevice_getMotionRange.invoke(dev, aaxis, event.getSource());
			if (range != null)
			{
				final float flat = range.getFlat();
				float v = (Float)MotionEvent_getAxisValueJ.invoke(event, aaxis, 0);
				if (Math.abs(v) < flat)
					v = 0;	//read as 0 if its within the deadzone.
				axis(event.getDeviceId(), qaxis, v);
			}
		}
		catch(Exception e)
		{
		}
	}
	private boolean motionEvent(MotionEvent event)
	{
		int id;
		float x, y, size;
		final int act = event.getAction();
		final int src = event.getSource();

		//handle gamepad axis
		if ((event.getSource() & (SOURCE_GAMEPAD|SOURCE_JOYSTICK))!=0 && event.getAction() == MotionEvent.ACTION_MOVE)
		{
			InputDevice dev = event.getDevice();
			handleJoystickAxis(event, dev, AXIS_X, 0);
			handleJoystickAxis(event, dev, AXIS_Y, 1);
			handleJoystickAxis(event, dev, AXIS_LTRIGGER, 2);
			
			handleJoystickAxis(event, dev, AXIS_Z, 3);
			handleJoystickAxis(event, dev, AXIS_RZ, 4);
			handleJoystickAxis(event, dev, AXIS_RTRIGGER, 5);

			return true;
		}

		final int pointerCount = event.getPointerCount();
		int i;
		for (i = 0; i < pointerCount; i++)
		{
			if (canrelative && src == SOURCE_MOUSE && wantrelative())
			{
				try
				{
					x = (Float)MotionEvent_getAxisValueP.invoke(event, AXIS_RELATIVE_X, i);
					y = (Float)MotionEvent_getAxisValueP.invoke(event, AXIS_RELATIVE_Y, i);
					motion(event.getPointerId(i), 1, x, y, 0, event.getSize(i));
				}
				catch(Exception e)
				{
					android.util.Log.i("FTEDroid", "exception using relative mouse");
					canrelative=false;
				}
			}
			else
			{
				motion(event.getPointerId(i), 0, event.getX(i), event.getY(i), 0, event.getSize(i));
			}
		}

		switch(act & event.ACTION_MASK)
		{
		case MotionEvent.ACTION_DOWN:
		case MotionEvent.ACTION_POINTER_DOWN:
			id = ((act&event.ACTION_POINTER_ID_MASK) >> event.ACTION_POINTER_ID_SHIFT);
			x = event.getX(id);
			y = event.getY(id);
			size = event.getSize(id);
			id = event.getPointerId(id);
			if (canbuttons && src == SOURCE_MOUSE)
			{
				try {mousepress(id, (Integer)MotionEvent_getButtonState.invoke(event));}
				catch(Exception e){}
			}
			else
				motion(id, 2, x, y, 0, size);
			break;
		case MotionEvent.ACTION_UP:
		case MotionEvent.ACTION_POINTER_UP:
			id = ((act&event.ACTION_POINTER_ID_MASK) >> event.ACTION_POINTER_ID_SHIFT);
			x = event.getX(id);
			y = event.getY(id);
			size = event.getSize(id);
			id = event.getPointerId(id);
			if (canbuttons && event.getSource() == SOURCE_MOUSE)
			{
				try {mousepress(id, (Integer)MotionEvent_getButtonState.invoke(event));}
				catch(Exception e){}
			}
			else
				motion(id, 3, x, y, 0, size);
			break;
		case MotionEvent.ACTION_MOVE:
			break;
		default:
			return false;
		}
		return true;
	}

	@Override
	public boolean dispatchTouchEvent(MotionEvent event)
	{	//works when mouse is pressed...
		return motionEvent(event);
	}
//	@Override
	public boolean dispatchGenericMotionEvent(MotionEvent event)
	{	//works even when mouse is not pressed
		return motionEvent(event);
	}


	//launching stuff
	private static native int unicodeKeyPress(int unicode);
	@Override
	protected void onNewIntent(android.content.Intent intent)
	{
		handleIntent(intent);
		super.onNewIntent(intent);
	}
}

