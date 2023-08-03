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

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.n0n3m4.q3e.karin.KOnceRunnable;

class Q3EView extends SurfaceView implements SurfaceHolder.Callback
{
    public static boolean mInit=false;

	public Q3EView(Context context)
    {
		super(context);

        getHolder().setFormat(GetPixelFormat());

        getHolder().addCallback(this);

        getHolder().setKeepScreenOn(true);
		setFocusable(false);
		setFocusableInTouchMode(false);
	}
    
    public void PushUIEvent(Runnable event)
    {
        post(event);
    }
    
    public void Shutdown(final Runnable andThen)
    {
        Q3EUtils.q3ei.callbackObj.PushEvent(new Runnable() {
            public void run()
            {
                if(mInit)
                    Q3EJNI.shutdown();
                if(null != andThen)
                    andThen.run();
            }
        });
    }

    public void Shutdown()
    {
        if(mInit)
            Q3EJNI.shutdown();
    }

    public void Pause()
    {
        if(!mInit)
            return;
        Runnable runnable = new KOnceRunnable() {
            @Override public void Run() {
                Q3EJNI.OnPause();
            }
        };
        Q3EUtils.q3ei.callbackObj.PushEvent(runnable);
    }

    public void Resume()
    {
        if(!mInit)
            return;
        Runnable runnable = new KOnceRunnable() {
            @Override public void Run() {
                Q3EJNI.OnResume();
            }
        };
        Q3EUtils.q3ei.callbackObj.PushEvent(runnable);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if(mInit)
            Q3EJNI.SetSurface(getHolder().getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        if(!mInit)
        {
            int orig_width = w;
            int orig_height = h;

            int[] size = GetFrameSize(w, h);
            int width = size[0];
            int height = size[1];
            int msaa = GetMSAA();
            int pixelFormat = GetPixelFormat();
            int glFormat = 0x8888;
            switch (pixelFormat) {
                case PixelFormat.RGBA_4444:
                    glFormat = 0x4444;
                    break;
                case PixelFormat.RGBA_5551:
                    glFormat = 0x5551;
                    break;
                case PixelFormat.RGB_565:
                    glFormat = 0x565;
                    break;
                case PixelFormat.RGBA_8888:
                    glFormat = 0x8888;
                    break;
            }

            String lib_dir = Q3EUtils.GetGameLibDir(getContext());
            String cmd = Q3EUtils.q3ei.cmd;
            SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this.getContext());
            boolean redirectOutputToFile = preferences.getBoolean(Q3EPreference.REDIRECT_OUTPUT_TO_FILE, true);
            boolean noHandleSignals = preferences.getBoolean(Q3EPreference.NO_HANDLE_SIGNALS, false);
            int runBackground = Q3EUtils.parseInt_s(preferences.getString(Q3EPreference.RUN_BACKGROUND, "0"), 0);

            Q3EJNI.init(lib_dir + "/" + Q3EUtils.q3ei.libname, width, height, Q3EMain.datadir, cmd, getHolder().getSurface(), glFormat, msaa, redirectOutputToFile, noHandleSignals, Q3EUtils.q3ei.multithread, runBackground > 0);

            mInit = true;

            getHolder().setFixedSize(orig_width, orig_height);
        }
    }

    private int GetPixelFormat()
    {
        if (PreferenceManager.getDefaultSharedPreferences(this.getContext()).getBoolean(Q3EPreference.pref_32bit, false))
        {
            return PixelFormat.RGBA_8888;
        }
        else
        {
            //if (Q3EUtils.q3ei.isD3)
            //k setEGLConfigChooser(new Q3EConfigChooser(5, 6, 5, 0, msaa, Q3EUtils.usegles20));
            //k getHolder().setFormat(PixelFormat.RGB_565);

            int harm16Bit = PreferenceManager.getDefaultSharedPreferences(this.getContext()).getInt(Q3EPreference.pref_harm_16bit, 0);
            switch (harm16Bit)
            {
                case 1: // RGBA4444
                    return PixelFormat.RGBA_4444;
                case 2: // RGBA5551
                    return PixelFormat.RGBA_5551;
                case 0: // RGB565
                default:
                    return PixelFormat.RGB_565;
            }
        }
    }

    private int GetMSAA()
    {
        int msaa = PreferenceManager.getDefaultSharedPreferences(this.getContext()).getInt(Q3EPreference.pref_msaa, 0);
        switch (msaa)
        {
            case 0: msaa = 0;break;
            case 1: msaa = 4;break;
            case 2: msaa = 16;break;
        }
        return msaa;
    }

    private int[] GetFrameSize(int w, int h)
    {
        int width;
        int height;
        SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(this.getContext());
        boolean scaleByScreenArea = mPrefs.getBoolean(Q3EPreference.pref_harm_scale_by_screen_area, false);

        switch (mPrefs.getInt(Q3EPreference.pref_scrres, 0))
        {
            case 0:
                width = w;
                height = h;
                break;
            case 1:
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, 0.5f);
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 2;
                    height = h / 2;
                }
                break;
            case 2:
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, 2);
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w * 2;
                    height = h * 2;
                }
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
                    String str = mPrefs.getString(Q3EPreference.pref_resx, "0");
                    if(null == str)
                        str = "0";
                    width = Integer.parseInt(str);
                }
                catch (Exception e)
                {
                    width = 0;
                }
                try
                {
                    String str = mPrefs.getString(Q3EPreference.pref_resy, "0");
                    if(null == str)
                        str = "0";
                    height = Integer.parseInt(str);
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
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, 1.0f / 3.0f);
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 3;
                    height = h / 3;
                }
                break;
            case 9: // 1/4
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, 1.0f / 4.0f);
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 4;
                    height = h / 4;
                }
                break;
            //k
            default:
                width = w;
                height = h;
                break;
        }
        Log.i("Q3EView", "FrameSize: (" + width + ", " + height + ")");
        return new int[] { width, height };
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        if(mInit)
            Q3EJNI.SetSurface(null);
		/*Q3EJNI.shutdown();
        super.surfaceDestroyed(holder);*/
    }
}
