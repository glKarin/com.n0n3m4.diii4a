package com.fteqw;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;

import android.view.SurfaceView;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.database.Cursor;

import android.hardware.SensorManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Environment;

import android.view.inputmethod.InputMethodManager;

import android.os.Vibrator;

public class FTEDroidActivity extends Activity
{
	private SensorManager sensorman;
	private Sensor sensoracc;
	private Sensor sensorgyro;
	private FTEView view;
	private String basedir, userdir;


	private audiothreadclass audiothread;
	private class audiothreadclass extends Thread
	{
		boolean timetodie;
		int schannels;
		int sspeed;
		int sbits;
		@Override
		public void run()
		{
			byte[] audbuf = new byte[2048];
			int avail;
			AudioTrack at;
			
			int chans;
			try
			{
				if (schannels >= 8)	//the OUT enumeration allows specific speaker control. but also api level 5+
					chans = AudioFormat.CHANNEL_OUT_7POINT1;
				else if (schannels >= 6)
					chans = AudioFormat.CHANNEL_OUT_5POINT1;
				else if (schannels >= 4)
					chans = AudioFormat.CHANNEL_OUT_QUAD;
				else if (schannels >= 2)
					chans = AudioFormat.CHANNEL_CONFIGURATION_STEREO;
				else
					chans = AudioFormat.CHANNEL_CONFIGURATION_MONO;
				int enc = (sbits == 8)?AudioFormat.ENCODING_PCM_8BIT:AudioFormat.ENCODING_PCM_16BIT;
				
				int sz = 2*AudioTrack.getMinBufferSize(sspeed, chans, enc);

//				if (sz < sspeed * 0.05)
//					sz = sspeed * 0.05;

				at = new AudioTrack(AudioManager.STREAM_MUSIC, sspeed, chans, enc, sz, AudioTrack.MODE_STREAM);
			}
			catch(IllegalArgumentException e)
			{
				//fixme: tell the engine that its bad and that it should configure some different audio attributes, instead of simply muting.
				return;
			}

			at.setStereoVolume(1, 1);
			at.play();
		
			while(!timetodie)
			{
				avail = FTEDroidEngine.paintaudio(audbuf, audbuf.length);
				at.write(audbuf, 0, avail);
			}
			
			at.stop();
		}
		public void killoff()
		{
			timetodie = true;
			try
			{
				join();
			}
			catch(InterruptedException e)
			{
			}
			timetodie = false;
		}
	};
	public void audioStop()
	{
		if (audiothread != null)
		{
			audiothread.killoff();
			audiothread = null;
		}
	}
	private void audioInit(int sspeed, int schannels, int sbits)
	{
		if (audiothread == null)
		{
			audiothread = new audiothreadclass();
			audiothread.schannels = schannels;
			audiothread.sspeed = sspeed;
			audiothread.sbits = sbits;
			audiothread.start();
		}
	}
	public void audioResume()
	{
		if (audiothread != null)
		{
			audiothread.killoff();
			audiothread.start();
		}
	}



	class FTEMultiTouchInputEvent extends FTELegacyInputEvent
	{
		/*Requires API level 5+ (android 2.0+)*/
		private void domove(MotionEvent event)
		{
			final int pointerCount = event.getPointerCount();
			int i;
			for (i = 0; i < pointerCount; i++)
				FTEDroidEngine.motion(0, event.getPointerId(i), event.getX(i), event.getY(i), event.getSize(i));
		}
		
		public boolean go(MotionEvent event)
		{
			int id;
			float x, y, size;
			final int act = event.getAction();

			domove(event);

			switch(act & event.ACTION_MASK)
			{
			case MotionEvent.ACTION_DOWN:
			case MotionEvent.ACTION_POINTER_DOWN:
				id = ((act&event.ACTION_POINTER_ID_MASK) >> event.ACTION_POINTER_ID_SHIFT);
				x = event.getX(id);
				y = event.getY(id);
				size = event.getSize(id);
				id = event.getPointerId(id);
				FTEDroidEngine.motion(1, id, x, y, size);
				break;
			case MotionEvent.ACTION_UP:
			case MotionEvent.ACTION_POINTER_UP:
				id = ((act&event.ACTION_POINTER_ID_MASK) >> event.ACTION_POINTER_ID_SHIFT);
				x = event.getX(id);
				y = event.getY(id);
				size = event.getSize(id);
				id = event.getPointerId(id);
				FTEDroidEngine.motion(2, id, x, y, size);
				break;
			case MotionEvent.ACTION_MOVE:
				break;
			default:
				return false;
			}
			return true;
		}
	}
	class FTELegacyInputEvent
	{
		public boolean go(MotionEvent event)
		{
			final int act = event.getAction();
			final float x = event.getX();
			final float y = event.getY();
			final float size = event.getSize();

			FTEDroidEngine.motion(0, 0, x, y, size);

			switch(act)
			{
			case MotionEvent.ACTION_DOWN:
				FTEDroidEngine.motion(1, 0, x, y, size);
				break;
			case MotionEvent.ACTION_UP:
				FTEDroidEngine.motion(2, 0, x, y, size);
				break;
			case MotionEvent.ACTION_MOVE:
				break;
			default:
				return false;
			}
			return true;
		}
	}
	
	private class FTEView extends SurfaceView implements SensorEventListener,android.view.SurfaceHolder.Callback
	{
		public int notifiedflags;
		final private FTEDroidActivity act;
		private FTELegacyInputEvent inputevent;
		
		private FTERenderThread rthread;
		public boolean inited;

		class FTERenderThread extends Thread
		{
			private FTEView theview;
			private android.view.SurfaceHolder surfaceholder;
			private boolean doquit;
			public FTERenderThread(FTEView parent, android.view.SurfaceHolder sh)
			{
				surfaceholder = sh;
				theview = parent;
			}
			@Override
			public void run()
			{
				FTEDroidEngine.setwindow(surfaceholder.getSurface());
				if (!theview.inited)
				{
					FTEDroidEngine.init(0, 0, basedir, userdir);
					theview.inited = true;
				}

				while (!doquit)
				{
					int flags = FTEDroidEngine.frame();
					//fixme: check return value for events to respond to
					
					if (flags != notifiedflags)
					{
						if (((flags ^ notifiedflags) & 1) != 0)
						{
							final int fl = flags;
							Runnable r = new Runnable() 
							{	//doing this on the ui thread because android sucks.
								public void run()
								{
									InputMethodManager im = (InputMethodManager) act.getSystemService(Context.INPUT_METHOD_SERVICE);
									if (im != null)
									{
										if ((fl & 1) != 0)
										{
			//								getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
											im.showSoftInput(theview, 0);//InputMethodManager.SHOW_FORCED);
										}
										else
										{
			//								getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
											im.hideSoftInputFromWindow(theview.getWindowToken(), 0);
										}
									}
									else
										android.util.Log.i("FTEDroid", "IMM failed");
								}
							};
							act.runOnUiThread(r);
						}
						if (((flags ^ notifiedflags) & 2) != 0)
						{
							int dur = FTEDroidEngine.getvibrateduration();
							flags &= ~2;
							Vibrator vib = (Vibrator) act.getSystemService(Context.VIBRATOR_SERVICE);
							if (vib != null)
							{
								android.util.Log.i("FTEDroid", "Vibrate " + dur + "ms");
								vib.vibrate(dur);
							}
							else
								android.util.Log.i("FTEDroid", "No vibrator device");
						}
						if (((flags ^ notifiedflags) & 4) != 0)
						{
							final int fl = flags;
							Runnable r = new Runnable() 
							{
								public void run()
								{
									if ((fl & 4) != 0)
										act.getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
									else
										act.getWindow().setFlags(0, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
								}
							};
							act.runOnUiThread(r);
						}
						if (((flags ^ notifiedflags) & 8) != 0)
						{
							final String errormsg = FTEDroidEngine.geterrormessage();
							
							inited = false;
							
							if (errormsg.equals(""))
							{
								finish();
								System.exit(0);
							}
							
							//8 means sys error
							Runnable r = new Runnable() 
							{
								public void run()
								{
									theview.setVisibility(theview.GONE);
									AlertDialog ad = new AlertDialog.Builder(act).create();
									ad.setTitle("FTE ERROR");
									ad.setMessage(errormsg);
									ad.setCancelable(false);
									ad.setButton("Ok", new DialogInterface.OnClickListener()
											{
												public void onClick(DialogInterface dialog, int which)
												{
													finish();
													System.exit(0);
												}
											}
										);
									ad.show();
								}
							};
							act.runOnUiThread(r);
						}
						if (((flags ^ notifiedflags) & 16) != 0)
						{		
							//16 means orientation cvar change				
							Runnable r = new Runnable()
							{
								public void run()
								{
									String ors = FTEDroidEngine.getpreferedorientation();
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
									act.setRequestedOrientation(ori);
								}
							};
							act.runOnUiThread(r);
						}

						if (((flags ^ notifiedflags) & 32) != 0)
						{
							final int fl = flags;
							Runnable r = new Runnable()
							{
								public void run()
								{
									if ((fl & 32) != 0)
										act.audioInit(FTEDroidEngine.audioinfo(0), FTEDroidEngine.audioinfo(1), FTEDroidEngine.audioinfo(2));
									else
										act.audioStop();
								}
							};
							act.runOnUiThread(r);
						}

						//clear anything which is an impulse
						notifiedflags = flags;
					}
				}
				FTEDroidEngine.setwindow(null);
			}

			public void requestExitAndWait()
			{
				doquit = true;
				try { join(); } catch(InterruptedException e) {}
			}
		}
	
		//SensorEventListener
		public void onAccuracyChanged(Sensor sensor, int accuracy)
		{
		}

		//SensorEventListener
		public void onSensorChanged(final SensorEvent event)
		{
			if (event.sensor == sensoracc)
			{
				FTEDroidEngine.accelerometer(event.values[0], event.values[1], event.values[2]);
			}
			else if (event.sensor == sensorgyro)
			{
				FTEDroidEngine.gyroscope(event.values[0], event.values[1], event.values[2]);
			}
		}

/*		private FTEJoystickInputEvent joystickevent;
		class FTEJoystickInputEvent
		{
			//API level 12+
			public boolean go(MotionEvent event)
			{
				if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK))
				{
					//FIXME: get MotionRange values from the device, so we can query the ideal size of the deadzone
					FTEDroidEngine.axischange(0, event.getAxisValue(MotionEvent.AXIS_X));
					FTEDroidEngine.axischange(1, event.getAxisValue(MotionEvent.AXIS_Y));
					FTEDroidEngine.axischange(2, event.getAxisValue(MotionEvent.AXIS_Z));
					FTEDroidEngine.axischange(3, event.getAxisValue(MotionEvent.AXIS_RZ));
					FTEDroidEngine.axischange(4, event.getAxisValue(MotionEvent.AXIS_HAT_X));
					FTEDroidEngine.axischange(5, event.getAxisValue(MotionEvent.AXIS_HAT_Y));
					FTEDroidEngine.axischange(6, event.getAxisValue(MotionEvent.AXIS_LTRIGGER));
					FTEDroidEngine.axischange(7, event.getAxisValue(MotionEvent.AXIS_RTRIGGER));
					FTEDroidEngine.axischange(8, event.getAxisValue(MotionEvent.AXIS_BREAK));
					FTEDroidEngine.axischange(9, event.getAxisValue(MotionEvent.AXIS_GAS));
					return true;
				}
				return false;
			}
		}
*/
		public FTEView(FTEDroidActivity context)
		{
			super(context);
			act = context;
			getHolder().addCallback(this);
			
			if (android.os.Build.VERSION.SDK_INT >= 5)
				inputevent = new FTEMultiTouchInputEvent();
			else
				inputevent = new FTELegacyInputEvent();
				
//			if (android.os.Build.VERSION.SDK_INT >= 12)
//				joystickevent = new FTEJoystickInputEvent();

			setFocusable(true);
			setFocusableInTouchMode(true);
		}

		@Override
		protected void onDraw(android.graphics.Canvas canvas)
		{	//no race conditions please.
		}

		@Override
		protected void onAttachedToWindow()
		{
			android.util.Log.i("FTEDroid", "onAttachedToWindow");
			super.onAttachedToWindow();
/*
			if (rthread != null)
				rthread.requestExitAndWait();
			rthread = new FTERenderThread(this);
			rthread.start();
*/
		}

		@Override
		protected void onDetachedFromWindow()
		{
			android.util.Log.i("FTEDroid", "onDetachedFromWindow");
			if (rthread != null)
				rthread.requestExitAndWait();
			rthread = null;

			super.onDetachedFromWindow();
		}
		public void surfaceDestroyed(android.view.SurfaceHolder holder)
		{
			android.util.Log.i("FTEDroid", "surfaceDestroyed");
			if (rthread != null)
				rthread.requestExitAndWait();
			rthread = null;
		}
		public void surfaceChanged(android.view.SurfaceHolder holder, int format, int width, int height)
		{
			android.util.Log.i("FTEDroid", "surfaceChanged");
		}
		public void surfaceCreated(android.view.SurfaceHolder holder)
		{
			android.util.Log.i("FTEDroid", "surfaceCreated");
			if (rthread != null)
				rthread.requestExitAndWait();
			rthread = new FTERenderThread(this, holder);
			rthread.start();
		}


		private boolean sendKey(final boolean presseddown, final int qcode, final int unicode)
		{
			return 0!=FTEDroidEngine.keypress(presseddown?1:0, qcode, unicode);
		}
		
		//view (inputs)
		@Override
		public boolean onTouchEvent(MotionEvent event)
		{
			return inputevent.go(event);
		}

		private static final int K_ENTER		= 13;
		private static final int K_ESCAPE		= 27;
		private static final int K_DEL			= 127;
		private static final int K_POWER		= 130;
		private static final int K_UPARROW		= 132;
		private static final int K_DOWNARROW	= 133;
		private static final int K_LEFTARROW	= 134;
		private static final int K_RIGHTARROW	= 135;
		private static final int K_APP			= 241;
		private static final int K_SEARCH		= 242;
		private static final int K_VOLUP		= 243;
		private static final int K_VOLDOWN		= 244;

/*		private static final int K_JOY1			= 203;
		private static final int K_JOY2			= 204;
		private static final int K_JOY3			= 205;
		private static final int K_JOY4			= 206;
		private static final int K_AUX1			= 207;
		private static final int K_AUX2			= 208;
		private static final int K_AUX3			= 209;
		private static final int K_AUX4			= 210;
		private static final int K_AUX5			= 211;
*/
		private static final int K_GP_A				= 190;
		private static final int K_GP_B				= 191;
		private static final int K_GP_X				= 192;
		private static final int K_GP_Y				= 193;
		private static final int K_GP_LEFT_SHOULDER	= 194;
		private static final int K_GP_RIGHT_SHOULDER= 195;
		private static final int K_GP_LEFT_TRIGGER	= 196;
		private static final int K_GP_RIGHT_TRIGGER	= 197;
		private static final int K_GP_BACK			= 198;
		private static final int K_GP_START			= 199;
		private static final int K_GP_LEFT_THUMB	= 200;
		private static final int K_GP_RIGHT_THUMB	= 201;
		private static final int K_GP_DPAD_UP		= 251;
		private static final int K_GP_DPAD_DOWN		= 252;
		private static final int K_GP_DPAD_LEFT		= 253;
		private static final int K_GP_DPAD_RIGHT	= 254;
		private static final int K_GP_GUIDE			= 202;
		private static final int K_GP_UNKNOWN		= 255;

		private int mapKey(int acode, int unicode)
		{
			switch(acode)
			{
			case 280/*KeyEvent.KEYCODE_SYSTEM_NAVIGATION_UP*/:
				return K_UPARROW;
			case 281/*KeyEvent.KEYCODE_SYSTEM_NAVIGATION_DOWN*/:
				return K_DOWNARROW;
			case 282/*KeyEvent.KEYCODE_SYSTEM_NAVIGATION_LEFT*/:
				return K_LEFTARROW;
			case 283/*KeyEvent.KEYCODE_SYSTEM_NAVIGATION_RIGHT*/:
				return K_RIGHTARROW;

			case KeyEvent.KEYCODE_DPAD_CENTER:
			case KeyEvent.KEYCODE_ENTER:
				return K_ENTER;
			case KeyEvent.KEYCODE_BACK:
				return K_ESCAPE;	//escape, cannot be rebound
			case KeyEvent.KEYCODE_MENU:
				return K_APP;		//"app"
			case KeyEvent.KEYCODE_DEL:
				return K_DEL;		//"del"
			case KeyEvent.KEYCODE_SEARCH:
				return K_SEARCH;	//"search"
			case KeyEvent.KEYCODE_POWER:
				return K_POWER;		//"power"
			case KeyEvent.KEYCODE_VOLUME_DOWN:
				return K_VOLDOWN;	//"voldown"
			case KeyEvent.KEYCODE_VOLUME_UP:
				return K_VOLUP;		//"volup"

			case KeyEvent.KEYCODE_DPAD_UP:
				return K_GP_DPAD_UP;
			case KeyEvent.KEYCODE_DPAD_DOWN:
				return K_GP_DPAD_DOWN;
			case KeyEvent.KEYCODE_DPAD_LEFT:
				return K_GP_DPAD_LEFT;
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				return K_GP_DPAD_RIGHT;
			case 96/*KeyEvent.KEYCODE_BUTTON_A*/:
				return K_GP_A;
			case 97/*KeyEvent.KEYCODE_BUTTON_B*/:
				return K_GP_B;
//			case 98/*KeyEvent.KEYCODE_BUTTON_C*/:
//				return K_GP_C;
			case 99/*KeyEvent.KEYCODE_BUTTON_X*/:
				return K_GP_X;
			case 100/*KeyEvent.KEYCODE_BUTTON_Y*/:
				return K_GP_Y;
//			case 101/*KeyEvent.KEYCODE_BUTTON_Z*/:
//				return K_GP_Z;
			case 102/*KeyEvent.KEYCODE_BUTTON_L1*/:
				return K_GP_LEFT_SHOULDER;
			case 103/*KeyEvent.KEYCODE_BUTTON_R1*/:
				return K_GP_RIGHT_SHOULDER;
			case 104/*KeyEvent.KEYCODE_BUTTON_L2*/:
				return K_GP_LEFT_TRIGGER;
			case 105/*KeyEvent.KEYCODE_BUTTON_R2*/:
				return K_GP_RIGHT_TRIGGER;
			case 106/*KeyEvent.KEYCODE_BUTTON_THUMBL*/:
				return K_GP_LEFT_THUMB;
			case 107/*KeyEvent.KEYCODE_BUTTON_THUMBR*/:
				return K_GP_RIGHT_THUMB;
			case 108/*KeyEvent.KEYCODE_BUTTON_START*/:
				return K_GP_START;
			case 109/*KeyEvent.KEYCODE_BUTTON_SELECT*/:
				return K_GP_BACK;
			case 110/*KeyEvent.KEYCODE_BUTTON_MODE*/:
				return K_GP_GUIDE;
//			case KeyEvent.KEYCODE_BUTTON_OTHER:
//				return K_GP_UNKNOWN;

			default:
				if (unicode < 128)
					return Character.toLowerCase(unicode);
			}
			return 0;
		}

		//view (inputs)
		@Override
		public boolean onKeyDown(int keyCode, KeyEvent event)
		{
			int uc = event.getUnicodeChar();
			int qc = mapKey(keyCode, uc);
			return sendKey(true, qc, uc);
		}

		//view (inputs)
		@Override
		public boolean onKeyUp(int keyCode, KeyEvent event)
		{
			int uc = event.getUnicodeChar();
			int qc = mapKey(keyCode, uc);
			return sendKey(false, qc, uc);
		}
	}

	private boolean runningintheemulator()
	{
		android.util.Log.i("FTEDroid", "model: " + android.os.Build.MODEL + " product: " + android.os.Build.PRODUCT + " device: " + android.os.Build.DEVICE);
		return android.os.Build.MODEL.equals("sdk") && android.os.Build.PRODUCT.equals("sdk") && android.os.Build.DEVICE.equals("generic");
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		android.util.Log.i("FTEDroid", "onCreate");
		
		try
		{
			String packagename = this.getComponentName().getPackageName();	//"com.fteqw", but not hardcoded.
			android.util.Log.i("FTEDroid", "Installed in package \"" + packagename + "\".");
			android.content.pm.PackageInfo info = this.getPackageManager().getPackageInfo(packagename, 0);
			basedir = info.applicationInfo.sourceDir;
		}
		catch(android.content.pm.PackageManager.NameNotFoundException e)
		{
			/*oh well, can just use the homedir instead*/
		}
		userdir = Environment.getExternalStorageDirectory().getPath() + "/fte";
		android.util.Log.i("FTEDroid", "Base dir is \"" + basedir + "\".");
		android.util.Log.i("FTEDroid", "User dir is \"" + userdir + "\".");

		//go full-screen
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);    	
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		super.onCreate(savedInstanceState);

		android.util.Log.i("FTEDroid", "create view");
		view = new FTEView(this);
		setContentView(view);


		if (runningintheemulator())
		{
			android.util.Log.i("FTEDroid", "emulator detected - skipping sensors to avoid emulator bugs");
			sensorman = null;
		}
		else
		{
			android.util.Log.i("FTEDroid", "init sensor manager");
			sensorman = (SensorManager)getSystemService(SENSOR_SERVICE);
		}
		if (sensorman != null)
		{
			android.util.Log.i("FTEDroid", "init accelerometer");
			sensoracc = sensorman.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
			android.util.Log.i("FTEDroid", "init gyro");
			sensorgyro = sensorman.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
		}
		android.util.Log.i("FTEDroid", "done");
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		if (sensorman != null && sensoracc != null)
			sensorman.registerListener((SensorEventListener)view, sensoracc, SensorManager.SENSOR_DELAY_GAME);
		if (sensorman != null && sensorgyro != null)
			sensorman.registerListener((SensorEventListener)view, sensorgyro, SensorManager.SENSOR_DELAY_GAME);

		audioResume();
	}

	@Override
	protected void onStart()
	{
		super.onStart();
		final android.content.Intent intent = getIntent();
		if (intent != null)
		{
			final android.net.Uri data = intent.getData();
			if (data != null)
			{
				String myloc;
				if (data.getScheme().equals("content"))
				{	//wtf.
					Cursor cursor = this.getContentResolver().query(data, null, null, null, null);
					cursor.moveToFirst();   
					myloc = cursor.getString(0);
					cursor.close();
					android.util.Log.i("FTEDroid", "intent content: " + myloc);
				}
				else
				{
					myloc = data.toString();
					android.util.Log.i("FTEDroid", "intent url: " + myloc);
				}
				FTEDroidEngine.openfile(myloc);
			}
		}
	}

	@Override
	protected void onStop()
	{
		if (sensorman != null && (sensoracc != null || sensorgyro != null))
			sensorman.unregisterListener(view);
		audioStop();
		super.onStop();
	}

	@Override
	protected void onPause()
	{
		if (sensorman != null && (sensoracc != null || sensorgyro != null))
			sensorman.unregisterListener(view);
		audioStop();
		super.onPause();
	}
}
