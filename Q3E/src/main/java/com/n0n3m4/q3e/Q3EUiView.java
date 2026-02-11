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

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.view.MotionEvent;

import com.n0n3m4.q3e.onscreen.FingerUi;
import com.n0n3m4.q3e.onscreen.Joystick;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.Q3EControls;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiLoader;

import java.util.ArrayList;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Q3EUiView extends GLSurfaceView implements GLSurfaceView.Renderer
{
    public final Q3EEditButtonHandler handler;

    private final Handler mHandler = new Handler();

    private final FingerUi[] fingers = new FingerUi[10];
    private final ArrayList<TouchListener> touch_elements = new ArrayList<>(0);
    private final ArrayList<Paintable> paint_elements = new ArrayList<>(0);

    private boolean mInit = false;

    public Q3EUiView(Context context)
    {
        super(context);

        handler = new Q3EEditButtonHandler(this, touch_elements, paint_elements, fingers);

        setRenderer(this);

        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override
    public void onDrawFrame(GL10 gl)
    {
        handler.OnDrawFrame(gl);
    }

    public void SaveAll()
    {
        handler.SaveAll();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int w, int h)
    {
        if (!mInit)
        {
            UiLoader uildr = new UiLoader(this, gl, w, h, Q3E.q3ei.defaults_table);

            for (int i = 0; i < Q3E.q3ei.UI_SIZE; i++)
            {
                Object o = uildr.LoadElement(i, true);
                touch_elements.add((TouchListener) o);
                paint_elements.add((Paintable) o);
            }

            for (Paintable p : paint_elements)
            {
                p.loadtex(gl);
                p.AsBuffer((GL11) gl);
            }

            for (int i = 0; i < fingers.length; i++)
                fingers[i] = new FingerUi(null, i);

            handler.OnSurfaceChanged(gl, w, h);
            handler.UpdateUsedTouches();

            mInit = true;
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (!mInit) return true;

        return handler.OnTouchEvent(event);
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
            handler.OnSurfaceCreated(gl, config);
        }
    }

    @Override
    protected void onDetachedFromWindow()
    {
        super.onDetachedFromWindow();

        handler.OnDetachedFromWindow();
    }

    public void Post(Runnable runnable, int... delayed)
    {
        if (null != delayed && delayed.length > 0)
            mHandler.postDelayed(runnable, delayed[0]);
        else
            mHandler.post(runnable);
    }

    public void UpdateOnScreenButtonsOpacity(float alpha)
    {
        if (!mInit)
            return;

        handler.UpdateOnScreenButtonsOpacity(alpha);
    }

    public void UpdateOnScreenButtonsSize(float scale)
    {
        if (!mInit)
            return;

        handler.UpdateOnScreenButtonsSize(scale);
    }

    public void ResetOnScreenButtonSize(TouchListener tgt)
    {
        if (!mInit)
            return;

        handler.ResetOnScreenButtonSize(tgt);
    }

    public void UpdateOnScreenButtonsPosition(boolean friendly)
    {
        UpdateOnScreenButtonsPosition(friendly, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE);
    }

    public void UpdateOnScreenButtonsPosition(boolean friendly, float scale)
    {
        if (!mInit)
            return;

        handler.UpdateOnScreenButtonsPosition(friendly, scale);
    }

    public void ResetOnScreenButtonPosition(TouchListener tgt)
    {
        if (!mInit)
            return;

        handler.ResetOnScreenButtonPosition(tgt);
    }

    public void UpdateJoystick(float range, float dz)
    {
        if (!mInit)
            return;

        if(range <= 1.0f)
            range = 1.0f;
        if(dz < 0.0f || dz >= 1.0f)
            dz = 0.0f;

        synchronized (paint_elements)
        {
            Paintable p = paint_elements.get(Q3EGlobals.UI_JOYSTICK);
            Joystick tmp = (Joystick) p;

            tmp.SetupFullZoneRadiusInEditMode(range);
            tmp.SetupDeadZoneRadiusInEditMode(dz);
        }

        //requestRender();
    }

    public void UpdateGrid(int unit)
    {
        if (!mInit)
            return;

        handler.UpdateGrid(unit);
    }

    public boolean IsModified()
    {
        return handler.IsModified();
    }

    public void SetModified()
    {
        handler.SetModified();
    }

    public void SetEditGame(String game)
    {
        handler.SetEditGame(game);
    }

    public int Width()
    {
        return handler.width;
    }

    public int Height()
    {
        return handler.height;
    }
}
