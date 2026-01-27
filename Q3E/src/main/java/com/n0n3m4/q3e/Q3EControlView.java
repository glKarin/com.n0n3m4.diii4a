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
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.preference.PreferenceManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;

import com.n0n3m4.q3e.gl.Q3EConfigChooser;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.onscreen.FingerUi;
import com.n0n3m4.q3e.onscreen.MouseControl;
import com.n0n3m4.q3e.onscreen.MouseButton;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiLoader;

import java.util.ArrayList;
import java.util.Arrays;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Q3EControlView extends GLSurfaceView implements GLSurfaceView.Renderer, SensorEventListener
{
    private boolean gameMode = true;
    private Q3EOnScreenButtonHandler handler;
    private final Q3EGameButtonHandler gameHandler;
    private Q3EEditButtonHandler editHandler;

    // render
    private boolean mInit = false;

    private Runnable afterToGameMode;
    private Runnable afterToEditMode;


    //RTCW4A-specific
    /*
    private Button actbutton;
    private Button kickbutton;
    */

    // render
    private final boolean hideonscr;

    private final FingerUi[]               fingers        = new FingerUi[10];
    private final ArrayList<TouchListener> touch_elements = new ArrayList<TouchListener>(0);
    private final ArrayList<Paintable> paint_elements = new ArrayList<Paintable>(0);


    public Q3EControlView(Context context)
    {
        super(context);

        setEGLConfigChooser(new Q3EConfigChooser(8, 8, 8, 8, 0, /*Q3EGL.usegles20*/false));
        getHolder().setFormat(PixelFormat.RGBA_8888);

/*        if (Q3EGL.usegles20)
            setEGLContextClientVersion(2);*/

        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        hideonscr = mPrefs.getBoolean(Q3EPreference.pref_hideonscr, false);

        gameHandler = new Q3EGameButtonHandler(this, touch_elements, paint_elements, fingers);
        handler = gameHandler;

        setRenderer(this);

        setFocusable(true);
        setFocusableInTouchMode(true);

        gameHandler.OnCreate(context);
    }

    @Override
    public void onDrawFrame(GL10 gl)
    {
        handler.OnDrawFrame(gl);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int w, int h)
    {
        if (!mInit)
        {
            KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Control view: %d x %d", w, h);

            Q3E.orig_width = w;
            Q3E.orig_height = h;
            //Q3E.activity.SetupGameViewSize(w, h, false);

            UiLoader uildr = new UiLoader(this, gl, Q3E.orig_width, Q3E.orig_height, Q3E.q3ei.defaults_table);

            for (int i = 0; i < Q3E.q3ei.UI_SIZE; i++)
            {
/*                boolean visible = uildr.CheckVisible(i);
                Log.i("Q3EControlView", "On-screen button " + i + " -> " + (visible ? "show" : "hide"));
                if(!visible)
                    continue;*/
                Object o = uildr.LoadElement(i, false);
                touch_elements.add((TouchListener) o);
                paint_elements.add((Paintable) o);
            }

            SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this.getContext());
            if (hideonscr)
            {
                touch_elements.clear();
            }
            //must be last
            //touch_elements.add(new MouseControl(this, false));
            touch_elements.add(new MouseControl(this));
            if(mPrefs.getBoolean(Q3EPreference.pref_2fingerlmb, false))
                touch_elements.add(new MouseButton(this));
            //touch_elements.add(new MouseControl(this, false));

            if (hideonscr)
            {
                paint_elements.clear();
            }
            for (Paintable p : paint_elements)
            {
                p.loadtex(gl);
                p.AsBuffer((GL11) gl);
            }

            for (int i = 0; i < fingers.length; i++)
                fingers[i] = new FingerUi(null, i);

            gameHandler.OnSurfaceChanged(gl, w, h);
            if (!hideonscr)
                gameHandler.UpdateUsedTouches();

            mInit = true;
            post(new Runnable() {
                @Override
                public void run() {
                    getHolder().setFixedSize(Q3E.orig_width, Q3E.orig_height);
                }
            });
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        if (mInit)
        {
            for (Paintable p : paint_elements)
            {
                p.loadtex(gl);
                p.AsBuffer((GL11) gl);
            }

            gameHandler.OnSurfaceCreated(gl, config);
        }
    }

    //@Override
    public boolean OnKeyDown(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_BACK && !gameMode)
        {
            return true;
        }
        return handler.OnKeyDown(keyCode, event);
    }

    //@Override
    public boolean OnKeyUp(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_BACK && !gameMode)
        {
            ToggleMode(afterToGameMode, afterToEditMode);
            return true;
        }
        return handler.OnKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event)
    {
        return handler.OnKeyMultiple(keyCode, repeatCount, event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        return handler.OnGenericMotionEvent(event) || super.onGenericMotionEvent(event);
    }

    @Override
    public boolean onCapturedPointerEvent(MotionEvent event)
    {
        return handler.onCapturedPointerEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (!mInit) return true;
        handler.OnTouchEvent(event);
        return true; // super.onTouchEvent(event);
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        return handler.OnTrackballEvent(event);
    }

    public void QueueEvent(Runnable r)
    {
        Q3E.callbackObj.PushEvent(r);
    }

    @Override
    public void onSensorChanged(SensorEvent event)
    {
        handler.OnSensorChanged(event);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    @Override
    public void onResume()
    {
        super.onResume();
        Resume();
    }

    @Override
    public void onPause()
    {
        super.onPause();
        Pause();
    }

    public void Pause()
    {
        handler.Pause();
    }

    public void Resume()
    {
        handler.Resume();
    }

    @Override
    protected void onDetachedFromWindow()
    {
        super.onDetachedFromWindow();

        if(null != Q3E.activity)
            Q3E.keyboard.onDetachedFromWindow();
    }

    private void SortOnScreenButtons()
    {
        TouchListener[] touchListeners = touch_elements.toArray(new TouchListener[0]);
        Arrays.sort(touchListeners, new TouchListener.TouchListenerCmp());
        touch_elements.clear();
        touch_elements.addAll(Arrays.asList(touchListeners));
    }

    public void GrabMouse()
    {
        handler.GrabMouse();
    }

    public void UnGrabMouse()
    {
        handler.UnGrabMouse();
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus)
    {
        super.onWindowFocusChanged(hasWindowFocus);
        handler.OnWindowFocusChanged(hasWindowFocus);
    }

    public void ShowCursor(boolean on)
    {
        if(!Q3E.m_usingMouse)
            return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            if(on)
                setPointerIcon(null);
            else
                setPointerIcon(PointerIcon.getSystemIcon(getContext(), PointerIcon.TYPE_NULL));
        }
    }

    private void EditMode(Runnable after)
    {
        boolean isCreated = null == editHandler;
        if(isCreated)
        {
            editHandler = new Q3EEditButtonHandler(this, touch_elements, paint_elements, fingers);
            editHandler.SetEditGame(Q3E.q3ei.game);
            editHandler.OnCreate(getContext());
            editHandler.TransparentBackground(true);
            editHandler.SetPortrait(Q3E.activity.IsPortrait());
            editHandler.SetWriteToDefault(true);
        }
        editHandler.SetSaveChanges(true);
        queueEvent(new Runnable() {
            @Override
            public void run()
            {
                if(isCreated)
                {
                    if(handler != editHandler)
                        handler.GLEnd();
                    editHandler.OnSurfaceChanged(gameHandler.gl, gameHandler.width, gameHandler.height);
                    editHandler.OnSurfaceCreated(gameHandler.gl, null);
                    editHandler.GLBegin();
                }
                else
                {
                    if(handler != editHandler)
                    {
                        handler.GLEnd();
                        editHandler.GLBegin();
                    }
                }
                post(new Runnable() {
                    @Override
                    public void run() {
                        if(handler != editHandler)
                        {
                            handler.End();
                            editHandler.Begin();
                        }
                        handler = editHandler;
                        gameMode = false;
                        if(null != after)
                            after.run();
                        KLog.I("Switch to edit mode");
                    }
                });
            }
        });
    }

    private void GameMode(Runnable after)
    {
        queueEvent(new Runnable() {
            @Override
            public void run()
            {
                if(handler != gameHandler)
                {
                    handler.GLEnd();
                    gameHandler.GLBegin();
                }
                post(new Runnable() {
                    @Override
                    public void run() {
                        if(handler != gameHandler)
                        {
                            handler.End();
                            gameHandler.Begin();
                        }
                        handler = gameHandler;
                        gameMode = true;
                        if(null != after)
                            after.run();
                        KLog.I("Switch to game mode");
                    }
                });
            }
        });
    }

    public boolean ToggleMode(Runnable gameAfter, Runnable editAfter)
    {
        if(hideonscr || Q3E.activity.IsPortrait())
            return false;
        afterToGameMode = gameAfter;
        afterToEditMode = editAfter;
        if(gameMode)
            EditMode(new Runnable() {
                @Override
                public void run() {
                    if(null != afterToEditMode)
                    {
                        afterToEditMode.run();
                        afterToEditMode = null;
                    }
                }
            });
        else
            GameMode(new Runnable() {
                @Override
                public void run() {
                    if(null != afterToGameMode)
                    {
                        afterToGameMode.run();
                        afterToGameMode = null;
                    }
                }
            });
        return true;
    }

    public void ExitEditMode(boolean save)
    {
        if(hideonscr || Q3E.activity.IsPortrait())
            return;

        if(gameMode)
            return;

        if(null == editHandler)
            return;

        editHandler.SetSaveChanges(save);
        GameMode(new Runnable() {
            @Override
            public void run() {
                if(null != afterToGameMode)
                {
                    afterToGameMode.run();
                    afterToGameMode = null;
                }
            }
        });
    }

    public boolean IsEditMode()
    {
        return !gameMode;
    }

    public void EnableSDL()
    {
        gameHandler.EnableSDL();
    }
}
