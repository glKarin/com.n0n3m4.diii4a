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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.view.SurfaceHolder;

class Q3EView extends GLSurfaceView implements GLSurfaceView.Renderer
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

	public Q3EView(Context context)
    {
		super(context);

		mHandler = new Handler();

		Q3EUtils.usegles20 = Q3EUtils.q3ei.isD3 || Q3EUtils.q3ei.isQ1 || Q3EUtils.q3ei.isD3BFG;
		int msaa=PreferenceManager.getDefaultSharedPreferences(this.getContext()).getInt(Q3EUtils.pref_msaa, 0);
		switch (msaa)
        {
            case 0: msaa = 0;break;
            case 1: msaa = 4;break;
            case 2: msaa = 16;break;
		}
		if (PreferenceManager.getDefaultSharedPreferences(this.getContext()).getBoolean(Q3EUtils.pref_32bit, false))
		{						
            setEGLConfigChooser(new Q3EConfigChooser(8, 8, 8, 8, msaa, Q3EUtils.usegles20));		
            getHolder().setFormat(PixelFormat.RGBA_8888);
		}
		else
		{
            //if (Q3EUtils.q3ei.isD3) 
            //k setEGLConfigChooser(new Q3EConfigChooser(5, 6, 5, 0, msaa, Q3EUtils.usegles20));
            //k getHolder().setFormat(PixelFormat.RGB_565);

            int harm16Bit = PreferenceManager.getDefaultSharedPreferences(this.getContext()).getInt(Q3EUtils.pref_harm_16bit, 0);
            switch (harm16Bit)
            {
                case 1: // RGBA4444
                    setEGLConfigChooser(new Q3EConfigChooser(4, 4, 4, 4, msaa, Q3EUtils.usegles20));
                    getHolder().setFormat(PixelFormat.RGBA_4444);	
                    break;
                case 2: // RGBA5551
                    setEGLConfigChooser(new Q3EConfigChooser(5, 5, 5, 1, msaa, Q3EUtils.usegles20));
                    getHolder().setFormat(PixelFormat.RGBA_5551);	

                    break;
                case 0: // RGB565
                default:
                    setEGLConfigChooser(new Q3EConfigChooser(5, 6, 5, 0, msaa, Q3EUtils.usegles20));
                    getHolder().setFormat(PixelFormat.RGB_565);
                    break;
            }
		}

		if (Q3EUtils.usegles20)
            setEGLContextClientVersion(2);				

		setRenderer(this);

		setFocusable(false);
		setFocusableInTouchMode(false);
	}

	public static boolean mInit=false;
	public static int width;
	public static int height;

	public static int orig_width;
	public static int orig_height;


	@Override
	public void onDrawFrame(GL10 gl)
    {
        //k
        if(!Q3EUtils.q3ei.multithread)
        Q3EUtils.q3ei.callbackObj.PullEvent(false);
        
		if (usesCSAA)
		{
			if (!Q3EUtils.usegles20)
				gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
			else
				GLES20.glClear(0x8000);
		}					

		if (Q3EUtils.q3ei.isD3BFG)
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_STENCIL_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

		Q3EJNI.drawFrame();
        
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

        if (Q3EUtils.q3ei.isQ1)
        {           
            GLES20.glEnable(GLES20.GL_CULL_FACE);
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);              
        }

	}				

    public String GetGameLibDir()
    {
        Context context = getContext();
        try
        {
            ApplicationInfo ainfo = context.getApplicationContext().getPackageManager().getApplicationInfo
            (
                context.getPackageName(),
                PackageManager.GET_SHARED_LIBRARY_FILES
            );
            return ainfo.nativeLibraryDir; //k for arm64-v8a apk install
        }
        catch(Exception e)
        {
            e.printStackTrace();
			return context.getCacheDir().getAbsolutePath().replace("cache", "lib");		//k old, can work with armv5 and armv7-a
        }
    }
    
	@Override
	public void onSurfaceChanged(GL10 gl, int w, int h)
    {
		if (mInit == false)
		{
			String lib_dir = GetGameLibDir();		

			SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(this.getContext());

			orig_width = w;
			orig_height = h;

			switch (mPrefs.getInt(Q3EUtils.pref_scrres, 0))
            {
                case 0:
                    width = w;
                    height = h;
                    break;
                case 1:	
                    width = w / 2;
                    height = h / 2;
                    break;
                case 2:
                    width = w * 2;
                    height = h * 2;
                    break;
                case 3:
                    width = 1920;
                    height = 1080;
                    break;
                case 4:
                    //k width=Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resx, "640"));
                    //k height=Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resy, "480"));
                    try
                    {
                        width = Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resx, "0"));
                    }
                    catch (Exception e)
                    {
                        width = 0;
                    }
                    try
                    {
                        height = Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resy, "0"));   
                    }
                    catch (Exception e)
                    {
                        height = 0;
                    }
                    if (width <= 0 && height <= 0)
                    {
                        width = w;
                        height = h;
                    }
                    if (width <= 0)
                    {
                        width = (int)((float)height * (float)w / (float)h);
                    }
                    else if (height <= 0)
                    {
                        height = (int)((float)width * (float)h / (float)w);
                    }
                    break;

                    //k
                case 5: // 720p
                    width = 1280;
                    height = 720;
                    break;
                case 6: // 480p
                    width = 720;
                    height = 480;
                    break;
                case 7: // 360p
                    width = 640;
                    height = 360;
                    break;
                case 8: // 1/3
                    width = w / 3;
                    height = h / 3;
                    break;
                case 9: // 1/4
                    width = w / 4;
                    height = h / 4;
                    break;
                    //k
			} 			

			String cmd = Q3EMain.datadir + "/" + mPrefs.getString(Q3EUtils.pref_params, Q3EUtils.q3ei.libname) + " " + Q3EUtils.q3ei.start_temporary_extra_command/* + " +set harm_fs_gameLibDir " + lib_dir*/;
			Q3EJNI.init(lib_dir + "/" + Q3EUtils.q3ei.libname, width, height, Q3EMain.datadir, cmd);

			mInit = true;
			mHandler.post(new Runnable() {				
                    @Override
                    public void run()
                    {
                        if ((orig_width != width) || (orig_height != height))
                            getHolder().setFixedSize(width, height);					
                    }
                });
		}
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {

		if (Q3EUtils.usegles20)
			Q3EUtils.initGL20();

		if (mInit)
		{						
            Q3EJNI.vidRestart();
		}

	}
    
    public void PushUIEvent(Runnable event)
    {
        mHandler.post(event);
    }

    public void PushEvent(Runnable event)
    {
        queueEvent(event);
    }
    
    private Q3EControlView m_controlView;
    
    public void ControlView(Q3EControlView view)
    {
        m_controlView = view;
    }
    
    public void Shutdown(final Runnable andThen)
    {
        if(Q3EUtils.q3ei.multithread)
        {
            Q3EJNI.shutdown();
            if(null != andThen)
                andThen.run();
            return;
        }
        queueEvent(new Runnable() {
            public void run()
            {
                Q3EJNI.shutdown();    
                if(null != andThen)
                    andThen.run();
            }
        });
    }

    /*
    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
		Q3EJNI.shutdown();    
        super.surfaceDestroyed(holder);
    }
    */
}
