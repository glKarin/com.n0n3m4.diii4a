/*
 Copyright (C) 2012 n0n3m4

 This file contains some code from kwaak3:
 Copyright (C) 2010 Roderick Colenbrander

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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Toast;

import com.n0n3m4.q3e.Q3EKeyCodes.KeyCodes;
import com.n0n3m4.q3e.Q3EUtils.Finger;
import com.n0n3m4.q3e.Q3EUtils.Paintable;
import com.n0n3m4.q3e.Q3EUtils.TouchListener;
import com.n0n3m4.q3e.Q3EUtils.UiLoader;
import com.stericson.RootTools.Command;
import com.stericson.RootTools.CommandCapture;
import com.stericson.RootTools.RootTools;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import tv.ouya.console.api.OuyaController;
import java.util.LinkedList;

class Q3EControlView extends GLSurfaceView implements GLSurfaceView.Renderer
{

	public Handler mHandler;
	public boolean usesCSAA=false;


	public class Q3EConfigChooser implements EGLConfigChooser
    {
        int r;
        int g;
        int b;
        int a;
        int msaa;
        boolean gl2;
        EGL10 eglcmp;
        EGLDisplay dspcmp;

        public Q3EConfigChooser(int inr, int ing, int inb, int ina, int inmsaa, boolean gles2)
        {
            r = inr;
            g = ing;
            b = inb;
            a = ina;
            msaa = inmsaa;
            gl2 = gles2;
        }

        /*EGLConfig[] configs=new EGLConfig[1000];
         int numconfigs=0;
         */
        class comprtr implements Comparator<EGLConfig>
        {
			@Override
			public int compare(EGLConfig lhs, EGLConfig rhs)
            {
				int tmp[]=new int[1];
				int lr,lg,lb,la,ld,ls;
				int rr,rg,rb,ra,rd,rs;
				int rat1,rat2;
				eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_RED_SIZE, tmp);lr = tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_GREEN_SIZE, tmp);lg = tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_BLUE_SIZE, tmp);lb = tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_ALPHA_SIZE, tmp);la = tmp[0];
				//eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_DEPTH_SIZE, tmp);ld=tmp[0];
				//eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_STENCIL_SIZE, tmp);ls=tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_RED_SIZE, tmp);rr = tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_GREEN_SIZE, tmp);rg = tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_BLUE_SIZE, tmp);rb = tmp[0];
				eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_ALPHA_SIZE, tmp);ra = tmp[0];
				//eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_DEPTH_SIZE, tmp);rd=tmp[0];
				//eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_STENCIL_SIZE, tmp);rs=tmp[0];
				rat1 = (Math.abs(lr - r) + Math.abs(lg - g) + Math.abs(lb - b));//*1000000-(ld*10000+la*100+ls);
				rat2 = (Math.abs(rr - r) + Math.abs(rg - g) + Math.abs(rb - b));//*1000000-(rd*10000+ra*100+rs);
				return Integer.valueOf(rat1).compareTo(Integer.valueOf(rat2));
			}
        }

        public int[] intListToArr(ArrayList<Integer> integers)
        {
            int[] ret=new int[integers.size()];
            Iterator<Integer> iterator=integers.iterator();
            for (int i=0;i < ret.length;i++)
            {
                ret[i] = iterator.next().intValue();
            }
            return ret;
        }

		@Override
		public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display)
        {
			dspcmp = display;
			eglcmp = egl;

			int[] tmp=new int[1];
			ArrayList<Integer> alst=new ArrayList<Integer>(0);
			alst.add(EGL10.EGL_SAMPLE_BUFFERS);alst.add((msaa > 0) ?1: 0);
			alst.add(EGL10.EGL_SAMPLES);alst.add(msaa);

			//TODO tegra zbuf
		    //alst.add(0x30E2);alst.add(0x30E3);
			alst.add(EGL10.EGL_RED_SIZE);alst.add(r);
			alst.add(EGL10.EGL_GREEN_SIZE);alst.add(g);
			alst.add(EGL10.EGL_BLUE_SIZE);alst.add(b);
			alst.add(EGL10.EGL_ALPHA_SIZE);alst.add(a);
			if (gl2)
			{alst.add(EGL10.EGL_RENDERABLE_TYPE);alst.add(4);}
			//k alst.add(EGL10.EGL_DEPTH_SIZE);alst.add(32);
			alst.add(EGL10.EGL_DEPTH_SIZE);alst.add(r == 8 ? 32 : 16);
			alst.add(EGL10.EGL_STENCIL_SIZE);alst.add(8);
			alst.add(EGL10.EGL_NONE);
			int[] pararr=intListToArr(alst);
			EGLConfig[] configs=new EGLConfig[1000];
			while (tmp[0] == 0)
			{
                egl.eglChooseConfig(display, pararr, configs, 1000, tmp);
                pararr[pararr.length - 4] -= 4;
                if (pararr[pararr.length - 4] < 0)
                {
                    pararr[pararr.length - 4] = 32;
                    pararr[pararr.length - 2] -= 4;
                    if (pararr[pararr.length - 2] < 0)
                    {
                        if (pararr[0] != 0x30E0)
                        {
                            pararr[0] = 0x30E0;
                            pararr[2] = 0x30E1;
                            pararr[pararr.length - 4] = 32;
                            pararr[pararr.length - 2] = 8;
                        }
                        else
                        {
                            //LOLWUT?! Let's crash.
                            return null;
                        }
                    }
                }
			}
            //numconfigs=tmp[0];
            Arrays.sort(configs, 0, tmp[0], new comprtr());
			return configs[0];
		}
    }

	public Q3EControlView(Context context)
    {
		super(context);

		mHandler = new Handler();				

		try
        {					            
            if (Q3EUtils.isOuya)
                OuyaController.init(context);
		}
        catch (Exception e)
        {
    	}

		Q3EUtils.usegles20 = Q3EUtils.q3ei.isD3 || Q3EUtils.q3ei.isQ1 || Q3EUtils.q3ei.isD3BFG;
		
        setEGLConfigChooser(new Q3EConfigChooser(8, 8, 8, 8, 0, Q3EUtils.usegles20));        
        getHolder().setFormat(PixelFormat.RGBA_8888);

		if (Q3EUtils.usegles20)
            setEGLContextClientVersion(2);				

		setRenderer(this);

		setFocusable(true);
		setFocusableInTouchMode(true);
	}

	public static boolean mInit=false;
	private IntBuffer tmpbuf;

	public static int orig_width;
	public static int orig_height;

	public boolean mapvol=false;
	public boolean analog=false;


	//RTCW4A-specific
	Q3EUtils.Button actbutton;
	Q3EUtils.Button kickbutton;

	//MOUSE

	public static String detectedtmp;
	public static String detectedname;
	public static final String detecthnd="Handlers=";
	public static final String detectmouse="mouse";
	public static final String detectrel="REL=";
	public static boolean detectfoundpreferred=false;

	public static String detectmouse()
	{
		try
		{				
            Command command = new Command(0, "cat /proc/bus/input/devices")
            {
		        @Override
		        public void output(int id, String line)
		        {
		        	if (line == null) return;
		        	if (line.contains(detecthnd) && (line.contains(detectmouse) || !detectfoundpreferred))
		        	{
		        		detectedtmp = line.substring(line.indexOf(detecthnd) + detecthnd.length());
		        		detectedtmp = detectedtmp.substring(detectedtmp.indexOf("event"));		        		
		        		if (detectedtmp.contains(" "))
		        			detectedtmp = detectedtmp.substring(0, detectedtmp.indexOf(" "));
		        		detectfoundpreferred = line.contains(detectmouse);
		        	}
		            if (line.contains(detectrel))
		            {
		            	detectedname = "/dev/input/" + detectedtmp;
		            }
		        }
            };		
            RootTools.getShell(true).add(command).waitForFinish();
            return detectedname;
		}
		catch (Throwable t)
        {t.printStackTrace();return null;}		
	}

	private int readmouse_dx=0;
	private int readmouse_dy=0;
	private int readmouse_keycode=0;
	private int readmouse_keystate=0;
	private boolean qevnt_available=true;
	private int mouse_corner=3;
	private boolean hideonscr;

	private String mouse_name=null;

	public Thread readmouse=new Thread(new Runnable() {		
            @Override
            public void run()
            {
                try
                {
                    CommandCapture command = new CommandCapture(0, "chmod 777 " + mouse_name);//insecure =(
                    RootTools.getShell(true).add(command).waitForFinish();				

                    FileInputStream fis=new FileInputStream(mouse_name);//.getChannel();
                    FileOutputStream fout=new FileOutputStream(mouse_name);//.getChannel();
                    final int sizeofstruct=8 + 2 + 2 + 4;
                    byte[] arr=new byte[sizeofstruct];
                    byte xcornr=(mouse_corner % 2 == 0) ?(byte)-127: 127;
                    byte ycornr=(mouse_corner < 2) ?(byte)-127: 127;
                    byte xargs=(xcornr < 0) ?(byte)-1: 0;
                    byte yargs=(ycornr < 0) ?(byte)-1: 0;
                    byte[] narr={0,0,0,0,0,0,0,0,2,0,0,0,xcornr,xargs,xargs,xargs,
                        0,0,0,0,0,0,0,0,2,0,1,0,ycornr,yargs,yargs,yargs,
                        0,0,0,0,0,0,0,0,0,0,0,0,127,0,0,0};
                    Runnable qevnt_runnable=new Runnable() {				
                        @Override
                        public void run()
                        {					
                            Q3EJNI.sendMotionEvent(readmouse_dx, readmouse_dy);
                            readmouse_dx = 0;
                            readmouse_dy = 0;
                            qevnt_available = true;
                        }
                    };
                    Runnable qkeyevnt_runnable=new Runnable() {				
                        @Override
                        public void run()
                        {
                            Q3EJNI.sendKeyEvent(readmouse_keystate, readmouse_keycode, 0);					
                        }
                    };
                    while (fis.read(arr, 0, sizeofstruct) != -1)
                    {										
                        if (!Q3ECallbackObj.reqThreadrunning)
                        {
                            Thread.yield();
                            continue;
                        }

                        if ((arr[sizeofstruct - 4] == 127) || (arr[sizeofstruct - 4] == -127)) continue;

                        if (arr[sizeofstruct - 8] == 0)
                        {
                            if (qevnt_available)
                            {
                                qevnt_available = false;
                                queueEvent(qevnt_runnable);
                            }
                            fout.write(narr);				
                        }
                        if (arr[sizeofstruct - 8] == 1)
                        {
                            readmouse_keycode = 0;
                            if (arr[sizeofstruct - 6] == 16)
                                readmouse_keycode = KeyCodes.K_MOUSE1;
                            if (arr[sizeofstruct - 6] == 17)
                                readmouse_keycode = KeyCodes.K_MOUSE2;
                            if (arr[sizeofstruct - 6] == 18)
                                readmouse_keycode = KeyCodes.K_MOUSE3;
                            if (arr[sizeofstruct - 6] == 19)
                                readmouse_keycode = KeyCodes.K_MOUSE4;
                            if (arr[sizeofstruct - 6] == 20)
                                readmouse_keycode = KeyCodes.K_MOUSE5;

                            readmouse_keystate = arr[sizeofstruct - 4];
                            if (readmouse_keycode != 0)
                                queueEvent(qkeyevnt_runnable);
                        }

                        if (arr[sizeofstruct - 8] == 2)
                        {
                            if ((arr[sizeofstruct - 6]) == 0) readmouse_dx += arr[sizeofstruct - 4];
                            if ((arr[sizeofstruct - 6]) == 1) readmouse_dy += arr[sizeofstruct - 4];
                            if ((arr[sizeofstruct - 6]) == 8) 
                            {
                                if (arr[sizeofstruct - 4] == 1)
                                    readmouse_keycode = KeyCodes.K_MWHEELUP;
                                else
                                    readmouse_keycode = KeyCodes.K_MWHEELDOWN;
                                readmouse_keystate = 1;
                                queueEvent(qkeyevnt_runnable);
                                Thread.sleep(25);
                                readmouse_keystate = 0;
                                queueEvent(qkeyevnt_runnable);
                            }
                        }
                    }

                }
                catch (Throwable e)
                {};
            }
        });		

	long oldtime=0;
	long delta=0;

	@Override
	public void onDrawFrame(GL10 gl)
    {	
		long t=System.currentTimeMillis();		
		delta = t - oldtime;		
		oldtime = t;
		if (delta > 1000)
			delta = 1000;

		if ((last_joystick_x != 0) || (last_joystick_y != 0))
            sendMotionEvent(delta * last_joystick_x, delta * last_joystick_y);

		if (usesCSAA)
		{
			if (!Q3EUtils.usegles20)
				gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
			else
				GLES20.glClear(0x8000);
		}					

		if (Q3EUtils.q3ei.isD3BFG)
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_STENCIL_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

		//Onscreen buttons:		
		//save state

		if (!Q3EUtils.usegles20)
		{		

            //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            //XXXXXXXXXXXXXXXXXXXXXXXX  GL 11  XXXXXXXXXXXXXXXXXXXXXXXXXX
            //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX			

            gl.glGetIntegerv(GL11.GL_MATRIX_MODE, tmpbuf);
            if (tmpbuf.get(0) == GL11.GL_PROJECTION)
            {
                //Do nothing, game is loading.
                //if (Q3EUtils.q3ei.isQ3||Q3EUtils.q3ei.isRTCW)
                return;
            }				

            gl.glGetIntegerv(GL11.GL_TEXTURE_BINDING_2D, tmpbuf);			
            int a=tmpbuf.get(0);		
            ((GL11)gl).glGetTexEnviv(GL11.GL_TEXTURE_ENV, GL11.GL_TEXTURE_ENV_MODE, tmpbuf);
            int b=tmpbuf.get(0);		
            boolean disabletex=!((GL11) gl).glIsEnabled(gl.GL_TEXTURE_COORD_ARRAY);
            boolean enableclr=((GL11) gl).glIsEnabled(gl.GL_COLOR_ARRAY);

            gl.glMatrixMode(gl.GL_PROJECTION);				

            gl.glLoadIdentity();
            gl.glOrthof(0, orig_width, orig_height, 0, -1, 1);

            gl.glDisable(gl.GL_CULL_FACE);
            gl.glDisable(gl.GL_DEPTH_TEST);
            if (Q3EUtils.q3ei.isQ2)
            {
                //������� <3
                gl.glDisable(gl.GL_SCISSOR_TEST);
                gl.glDisable(gl.GL_ALPHA_TEST);
                gl.glEnable(gl.GL_BLEND);
            }
            gl.glDisableClientState(gl.GL_COLOR_ARRAY);

            ((GL11)gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);

            gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);				

            gl.glColor4f(1, 1, 1, 0.3f);					

            for (Paintable p: paint_elements)
                p.Paint((GL11)gl);				

            gl.glColor4f(1, 1, 1, 1);		
            gl.glEnable(gl.GL_CULL_FACE);
            gl.glEnable(gl.GL_DEPTH_TEST);
            //restore
            if (disabletex)
                gl.glDisableClientState(gl.GL_TEXTURE_COORD_ARRAY);
            if (enableclr)
                gl.glEnableClientState(gl.GL_COLOR_ARRAY);
            ((GL11)gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, b);
            gl.glBindTexture(GL10.GL_TEXTURE_2D, a);
		}
		else
		{

            //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            //XXXXXXXXXXXXXXXXXXXXXXXX  GL 20  XXXXXXXXXXXXXXXXXXXXXXXXXX
            //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX				

            GLES20.glDisable(GLES20.GL_CULL_FACE);
            GLES20.glDisable(GLES20.GL_DEPTH_TEST);

            if (Q3EUtils.q3ei.isD3BFG)
            {
                //GLES20.glDisable(GLES20.GL_STENCIL_TEST);		
                //GLES20.glDisable(GLES20.GL_SCISSOR_TEST);

                GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
                GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, 0);
            }

            GLES20.glEnable(GLES20.GL_BLEND);				

            GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

            for (Paintable p: paint_elements)
                p.Paint((GL11)gl);

            if (Q3EUtils.q3ei.isQ1)
            {			
                GLES20.glEnable(GLES20.GL_CULL_FACE);
                GLES20.glEnable(GLES20.GL_DEPTH_TEST);				
            }
            
		}				

	}
    
	@Override
	public void onSurfaceChanged(GL10 gl, int w, int h)
    {
		if (mInit == false)
		{								

			tmpbuf = ByteBuffer.allocateDirect(4).order(ByteOrder.nativeOrder()).asIntBuffer();

			SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(this.getContext());

			hideonscr = mPrefs.getBoolean(Q3EUtils.pref_hideonscr, false);
			mapvol = mPrefs.getBoolean(Q3EUtils.pref_mapvol, false);
            m_mapBack = mPrefs.getInt(Q3EUtils.pref_harm_mapBack, ENUM_BACK_ALL); //k
			analog = mPrefs.getBoolean(Q3EUtils.pref_analog, true);
			boolean detectMouse=mPrefs.getBoolean(Q3EUtils.pref_detectmouse, true);

			mouse_name = hideonscr ?(detectMouse ?detectmouse(): mPrefs.getString(Q3EUtils.pref_eventdev, "/dev/input/event???")): null;
			mouse_corner = mPrefs.getInt(Q3EUtils.pref_mousepos, 3);

			orig_width = w;
			orig_height = h;

			UiLoader uildr=new UiLoader(this, gl, orig_width, orig_height);		

			for (int i=0;i < Q3EUtils.q3ei.UI_SIZE;i++)
			{
				Object o=uildr.LoadElement(i);
				touch_elements.add((TouchListener)o);
				paint_elements.add((Paintable)o);				
			}

			if (Q3EUtils.q3ei.isRTCW)
			{
                actbutton = (Q3EUtils.Button)touch_elements.get(Q3EUtils.q3ei.RTCW4A_UI_ACTION);
                kickbutton = (Q3EUtils.Button)touch_elements.get(Q3EUtils.q3ei.RTCW4A_UI_KICK);
			}
			else
			{
                actbutton = null;kickbutton = null;
			}

			if (hideonscr)
			{			
                touch_elements.clear();
			}
			//must be last
			touch_elements.add(new Q3EUtils.MouseControl(this, false));			
			touch_elements.add(new Q3EUtils.MouseControl(this, mPrefs.getBoolean(Q3EUtils.pref_2fingerlmb, false)));
			touch_elements.add(new Q3EUtils.MouseControl(this, false));

			if (hideonscr)
			{											
                paint_elements.clear();
			}
			for (Paintable p:paint_elements) p.loadtex(gl);

			for (int i=0;i < fingers.length;i++)
				fingers[i] = new Finger(null, i);

			if (mouse_name != null)
			{
                readmouse.setPriority(7);
                readmouse.start();
			}

			mInit = true;
            mHandler.post(new Runnable() {              
                    @Override
                    public void run()
                    {
                        getHolder().setFixedSize(orig_width, orig_height);                
                    }
                });
        }
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {

		if (Q3EUtils.usegles20)
        {
			Q3EUtils.initGL20();
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        }

		if (mInit)
		{
            for (Paintable p:paint_elements) p.loadtex(gl);						
		}

	}

	boolean notinmenu=true;
	public void setState(int st)
	{				
		if (actbutton != null)
            actbutton.alpha = ((st & 1) == 1) ?Math.min(actbutton.initalpha * 2, 1f): actbutton.initalpha;		
		if (kickbutton != null)
            kickbutton.alpha = ((st & 4) == 4) ?Math.min(kickbutton.initalpha * 2, 1f): kickbutton.initalpha;
		notinmenu = ((st & 2) == 2);
	}

	public int getCharacter(int keyCode, KeyEvent event)
	{		
		if (keyCode == KeyEvent.KEYCODE_DEL) return '\b';		
		return event.getUnicodeChar();		
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event)
    {		
		if ((!mapvol) && ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) || (keyCode == KeyEvent.KEYCODE_VOLUME_UP))) return false;		
		if (keyCode == KeyEvent.KEYCODE_BACK && (m_mapBack & ENUM_BACK_ESCAPE) == 0) 
        {
            return true;		   
        }
        int qKeyCode;
        switch(keyCode)
        {
            case KeyEvent.KEYCODE_VOLUME_UP:
                    qKeyCode = Q3EUtils.q3ei.VOLUME_UP_KEY_CODE;
                    break;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                    qKeyCode = Q3EUtils.q3ei.VOLUME_DOWN_KEY_CODE;
                    break;
            default:
                    qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event);
                    break;
        }
		int t=getCharacter(keyCode, event);				
		return sendKeyEvent(true, qKeyCode, t);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event)
    {		
		if ((!mapvol) && ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) || (keyCode == KeyEvent.KEYCODE_VOLUME_UP))) return false;	
		if (keyCode == KeyEvent.KEYCODE_BACK)
        {
            if(m_mapBack == ENUM_BACK_NONE)
                return true;
            if((m_mapBack & ENUM_BACK_EXIT) != 0 && HandleBackPress())
                return true;
        }			
        int qKeyCode;
        switch(keyCode)
        {
            case KeyEvent.KEYCODE_VOLUME_UP:
                qKeyCode = Q3EUtils.q3ei.VOLUME_UP_KEY_CODE;
                break;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                qKeyCode = Q3EUtils.q3ei.VOLUME_DOWN_KEY_CODE;
                break;
            default:
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event);
                break;
        }
		return sendKeyEvent(false, qKeyCode, getCharacter(keyCode, event));
	}


	static float last_joystick_x=0;
	static float last_joystick_y=0;

	@SuppressLint("NewApi")
	private static float getCenteredAxis(MotionEvent event,
                                         int axis)
    {
        final InputDevice.MotionRange range = event.getDevice().getMotionRange(axis, event.getSource());
        if (range != null)
        {
            final float flat = range.getFlat();
            final float value = event.getAxisValue(axis);
            if (Math.abs(value) > flat)
            {
                return value;
            }
        }
        return 0;
    }

	@SuppressLint("NewApi")
	@Override
	public boolean onGenericMotionEvent(MotionEvent event)
    {
		if (((event.getSource() == InputDevice.SOURCE_JOYSTICK) || (event.getSource() == InputDevice.SOURCE_GAMEPAD)) && (event.getAction() == MotionEvent.ACTION_MOVE))
		{
			float x = getCenteredAxis(event, MotionEvent.AXIS_X);
			float y = -getCenteredAxis(event, MotionEvent.AXIS_Y);			
			sendAnalog(((Math.abs(x) > 0.01) || (Math.abs(y) > 0.01)), x, y);			
	        x = getCenteredAxis(event, MotionEvent.AXIS_Z);
	        y = getCenteredAxis(event, MotionEvent.AXIS_RZ);	      	        
	        last_joystick_x = x;
		    last_joystick_y = y;	        	       
			return true;
		}
	    return false;
	}

	public static Finger[] fingers=new Finger[10];
	public static ArrayList<TouchListener> touch_elements=new ArrayList<TouchListener>(0);
	public static ArrayList<Paintable> paint_elements=new ArrayList<Paintable>(0);

	@SuppressLint("NewApi")
	@Override
	public boolean onTouchEvent(MotionEvent event)
    {		
		if (!mInit) return true;

		if ((Build.VERSION.SDK_INT >= 9) && (hideonscr) && (event.getSource() == InputDevice.SOURCE_MOUSE))
        {			
			event.setAction(MotionEvent.ACTION_CANCEL);
			return true;
		}

		if ((event.getActionMasked() == MotionEvent.ACTION_DOWN) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_DOWN))
		{
			int pid=event.getPointerId(event.getActionIndex());
			int x=(int)event.getX(event.getActionIndex());			
			int y=(int)event.getY(event.getActionIndex());
			for (TouchListener tl:touch_elements)
                if (tl.isInside(x, y))
                {
                    fingers[pid].target = tl;
                    break;
                }
		}

		try
		{
            for (Finger f:fingers)
                if (f.target != null) f.onTouchEvent(event);
		}
		catch (Exception e)
        {};

		if ((event.getActionMasked() == MotionEvent.ACTION_UP) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_UP) || (event.getActionMasked() == MotionEvent.ACTION_CANCEL))
		{
			int pid=event.getPointerId(event.getActionIndex());
			fingers[pid].target = null;
		}		

		return true;
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event)
    {
		return sendTrackballEvent(event.getAction() == MotionEvent.ACTION_DOWN, event.getX(), event.getY());
	}

	public boolean sendAnalog(final boolean down, final float x, final float y)
	{			
        queueEvent(new __Runnable(){
                public void __run()
                {            	
                    Q3EJNI.sendAnalog(down ?1: 0, x, y);        	
                }});        
        return true;
	}

	public boolean sendKeyEvent(final boolean down, final int keycode, final int charcode)
	{			
        queueEvent(new __Runnable(){
                public void __run()
                {            	
                    Q3EJNI.sendKeyEvent(down ?1: 0, keycode, charcode);   
                }});        
        return true;
	}

	static float last_trackball_x=0;
	static float last_trackball_y=0;

	public boolean sendMotionEvent(final float deltax, final float deltay)
	{
        queueEvent(new __Runnable(){
                public void __run()
                {            	
                    Q3EJNI.sendMotionEvent(deltax, deltay);      
                }});
        return true;
	}

	private boolean sendTrackballEvent(final boolean down, final float x, final float y)
	{
		queueEvent(new __Runnable(){
                public void __run()
                {
                    if (down)
                    {
                        last_trackball_x = x;
                        last_trackball_y = y;
                    }
                    Q3EJNI.sendMotionEvent(x - last_trackball_x, y - last_trackball_y);
                    last_trackball_x = x;
                    last_trackball_y = y;
                }});
        return true;
	}

    //k 
    // Once Runnable
    private static abstract class __Runnable implements Runnable
    {
        private boolean m_handle = false;

        @Override
        public void run()
        {
            if(m_handle) return;
            __run();
            m_handle = true;
        }
        
        protected abstract void __run();
    }
    
    private LinkedList<Runnable> m_eventQueue = new LinkedList<>();
    private Object m_eventQueueLock = new Object();

    public void queueEvent(Runnable r)
    {
        m_renderView.PushEvent(r);
        synchronized(m_eventQueueLock) {
            m_eventQueue.add(r);
        }
    }
    
    // It is will run on GLThread. Call by DOOM from JNI, for some MessageBox in game.
    public void PullEvent(boolean execCmd)
    {
        synchronized(m_eventQueueLock) {
            if(execCmd)
            {
                while(!m_eventQueue.isEmpty())
                    m_eventQueue.removeFirst().run();   
            }
            else
                m_eventQueue.clear();
        }
    }

    private int m_mapBack = ENUM_BACK_ALL;
    private long m_laskPressBackTime = -1;
    private int m_pressBackCount = 0;
    private static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL = 1000;
    private static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT = 3;
    private static final int ENUM_BACK_NONE = 0;
    private static final int ENUM_BACK_ESCAPE = 1;
    private static final int ENUM_BACK_EXIT = 2;
    private static final int ENUM_BACK_ALL = 0xFF;
    private boolean HandleBackPress()
    {
        if((m_mapBack & ENUM_BACK_EXIT) == 0)
            return false;
        boolean res = false;
        long now = System.currentTimeMillis();
        if(m_laskPressBackTime > 0 && now - m_laskPressBackTime <= CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL)
        {
            m_pressBackCount++;
            res = true;
        }
        else
        {
            m_pressBackCount = 1;
            res = (m_mapBack & ENUM_BACK_ESCAPE) != 0 ? false : true;
        }
        m_laskPressBackTime = now;
        if(m_pressBackCount == CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT - 1)
            Toast.makeText(getContext(), "Click back again to exit......", CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL).show();
        else if(m_pressBackCount == CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT)
        {
            m_renderView.Shutdown(new Runnable() {
                public void run()
                {
                    m_renderView.PushUIEvent(new Runnable() {
                       public void run()
                       {
                           ((Activity)getContext()).finishAffinity();
                           System.exit(0);   
                       }
                    });
                }
            });
            return true;
        }
        return res;
    }
    
    private Q3EView m_renderView;
    public void RenderView(Q3EView view)
    {
        m_renderView = view;
        view.ControlView(this);
    }
}
