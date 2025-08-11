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

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences.Editor;
import android.graphics.Point;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewGroup;

import com.n0n3m4.q3e.gl.KGLBitmapTexture;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.onscreen.Button;
import com.n0n3m4.q3e.onscreen.Disc;
import com.n0n3m4.q3e.onscreen.FingerUi;
import com.n0n3m4.q3e.onscreen.Joystick;
import com.n0n3m4.q3e.onscreen.MenuOverlay;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.Q3EButtonGeometry;
import com.n0n3m4.q3e.onscreen.Q3EButtonLayoutManager;
import com.n0n3m4.q3e.onscreen.Q3EControls;
import com.n0n3m4.q3e.onscreen.Slider;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.MenuOverlayView;
import com.n0n3m4.q3e.onscreen.UiElement;
import com.n0n3m4.q3e.onscreen.UiLoader;
import com.n0n3m4.q3e.onscreen.UiViewOverlay;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Q3EUiView extends GLSurfaceView implements GLSurfaceView.Renderer
{
    private       int          m_unit              = 0;
    public  int          step                = 10;
    private       FloatBuffer  m_gridBuffer        = null;
    private       int          m_numGridLineVertex = 0;
    private final Object       m_gridLock          = new Object();
    private       boolean      m_edited            = false;
    private final int[]        drawerTextureId     = {0, 0};
    private       ViewGroup    layout;
    private  int          m_drawerHeight                = 200;
    private final List<Paintable> reloadList = new ArrayList<>();

    public Q3EUiView(Context context)
    {
        super(context);

        Q3EGL.usegles20 = false;
        step = Q3EUtils.dip2px(getContext(), 5);
        m_drawerHeight = Q3EUtils.dip2px(getContext(), 50) * 3 + Q3EUtils.dip2px(getContext(), 5);

        setRenderer(this);

        setFocusable(true);
        setFocusableInTouchMode(true);

        String unit = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.CONTROLS_CONFIG_POSITION_UNIT, "0");
        if (null != unit)
        {
            m_unit = Integer.parseInt(unit);
        }
    }

    @Override
    public void onDrawFrame(GL10 gl)
    {
        gl.glClear(gl.GL_COLOR_BUFFER_BIT | gl.GL_DEPTH_BUFFER_BIT);

        gl.glMatrixMode(gl.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glOrthof(0, width, yoffset + height, yoffset, -1, 1);
        gl.glMatrixMode(gl.GL_MODELVIEW);

        ((GL11) gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);

        synchronized (m_gridLock)
        {
            if (null != m_gridBuffer && m_numGridLineVertex > 0)
            {
                gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
                gl.glLineWidth(1);
                gl.glBindTexture(gl.GL_TEXTURE_2D, 0);
                gl.glColor4f(1, 1, 1, 0.382f);
                gl.glVertexPointer(2, gl.GL_FLOAT, 0, m_gridBuffer);
                gl.glDrawArrays(gl.GL_LINES, 0, m_numGridLineVertex);
            }
        }
        gl.glLineWidth(4);

        gl.glBindTexture(gl.GL_TEXTURE_2D, 0);
        gl.glColor4f(1, 0, 0, 0.7f);
        gl.glVertexPointer(2, gl.GL_FLOAT, 0, linebuffer);
        gl.glDrawArrays(gl.GL_LINES, 0, 2);

        gl.glColor4f(1, 1, 1, 1); // 0.2f
        gl.glVertexPointer(2, gl.GL_FLOAT, 0, notifybuffer);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
        gl.glTexCoordPointer(2, gl.GL_FLOAT, 0, notifyTexCoordBuffer);
        gl.glPushMatrix();
        {
            if (yoffset == 0)
            {
                gl.glBindTexture(gl.GL_TEXTURE_2D, drawerTextureId[0]);
                gl.glTranslatef(0, height - height / 8, 0);
                gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4);
                //gl.glTranslatef(0, -(height-height/8), 0);
            }
            else
            {
                gl.glBindTexture(gl.GL_TEXTURE_2D, drawerTextureId[1]);
                gl.glTranslatef(0, yoffset, 0);
                gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4);
                //gl.glTranslatef(0, -yoffset, 0);
            }
        }
        gl.glPopMatrix();
        gl.glBindTexture(gl.GL_TEXTURE_2D, 0);

        synchronized (paint_elements) {
            if(!reloadList.isEmpty())
            {
                for (Paintable p : reloadList)
                {
                    p.AsBuffer((GL11) gl);
                }
                reloadList.clear();
            }

            for (Paintable p : paint_elements)
            {
                if(p instanceof Joystick)
                    ((Joystick)p).UpdateTexture((GL11) gl);
                p.Paint((GL11) gl);
            }
        }

        mover.Paint((GL11) gl);
    }

    Handler mHandler = new Handler();

    public FingerUi[] fingers = new FingerUi[10];
    public ArrayList<TouchListener> touch_elements = new ArrayList<>(0);
    public final ArrayList<Paintable> paint_elements = new ArrayList<>(0);

    boolean mInit = false;
    public int width;
    public int height;

    public int downtostep(int a, int la)
    {
        return a - la;
		/*int k=Math.round((float)(a-la)/(float)step);
		//int k=(a-la)/step;
		return k*step;*/
    }

    public void RefreshTgt(FingerUi fn)
    {
        SetModified();
        /*if (fn.target instanceof Button)
        {
            final Button tmp = (Button) fn.target;
            final Button newb = Button.Move(tmp, uildr.gl);
            fn.target = newb;
            touch_elements.set(touch_elements.indexOf(tmp), newb);
            paint_elements.set(paint_elements.indexOf(tmp), newb);
            m_edited = true;
        }
        else if (fn.target instanceof Joystick)
        {
            final Joystick tmp = (Joystick) fn.target;
            final Joystick newj = Joystick.Move(tmp, uildr.gl, Q3EUtils.q3ei.joystick_release_range, Q3EUtils.q3ei.joystick_inner_dead_zone);
            fn.target = newj;
            touch_elements.set(touch_elements.indexOf(tmp), newj);
            paint_elements.set(paint_elements.indexOf(tmp), newj);
            m_edited = true;
        }
        else if (fn.target instanceof Slider)
        {
            final Slider tmp = (Slider) fn.target;
            final Slider news = Slider.Move(tmp, uildr.gl);
            fn.target = news;
            touch_elements.set(touch_elements.indexOf(tmp), news);
            paint_elements.set(paint_elements.indexOf(tmp), news);
            m_edited = true;
        }
        //k
        else if (fn.target instanceof Disc)
        {
            final Disc tmp = (Disc) fn.target;
            final Disc newd = Disc.Move(tmp, uildr.gl);
            fn.target = newd;
            touch_elements.set(touch_elements.indexOf(tmp), newd);
            paint_elements.set(paint_elements.indexOf(tmp), newd);
            m_edited = true;
        }*/

        //PrintInfo(fn);
    }

    private boolean NormalizeTgtPosition(FingerUi fn)
    {
        if (m_unit <= 1)
            return false;

        if (fn.target instanceof Slider)
        {
            Slider tmp = (Slider) fn.target;
            float halfw = (float) tmp.width / 2.0f;
            float halfh = (float) tmp.height / 2.0f;
            tmp.cx = Math.round(((float) tmp.cx - halfw) / (float) m_unit) * m_unit + Math.round(halfw);
            tmp.cy = Math.round(((float) tmp.cy - halfh) / (float) m_unit) * m_unit + Math.round(halfh);
            return true;
        }
        else if (fn.target instanceof Button)
        {
            Button tmp = (Button) fn.target;
            float halfw = (float) tmp.width / 2.0f;
            float halfh = (float) tmp.height / 2.0f;
            tmp.cx = Math.round(((float) tmp.cx - halfw) / (float) m_unit) * m_unit + Math.round(halfw);
            tmp.cy = Math.round(((float) tmp.cy - halfh) / (float) m_unit) * m_unit + Math.round(halfh);
            return true;
        }
        else if (fn.target instanceof Joystick)
        {
            Joystick tmp = (Joystick) fn.target;
            float halfw = (float) tmp.size / 2.0f;
            int cx = Math.round(((float) tmp.cx - halfw) / (float) m_unit) * m_unit + Math.round(halfw);
            int cy = Math.round(((float) tmp.cy - halfw) / (float) m_unit) * m_unit + Math.round(halfw);
            tmp.SetPosition(cx, cy);
            return true;
        }
        //k
        else if (fn.target instanceof Disc)
        {
            Disc tmp = (Disc) fn.target;
            float halfw = (float) tmp.size / 2.0f;
            int cx = Math.round(((float) tmp.cx - halfw) / (float) m_unit) * m_unit + Math.round(halfw);
            int cy = Math.round(((float) tmp.cy - halfw) / (float) m_unit) * m_unit + Math.round(halfw);
            tmp.SetPosition(cx, cy);
            return true;
        }
        return false;
    }

    public void ModifyTgt(FingerUi fn, int dx, int dy)
    {
        if (fn.target instanceof Slider)
        {
            Slider tmp = (Slider) fn.target;
            tmp.Translate(dx, dy);
        }
        else if (fn.target instanceof Button)
        {
            Button tmp = (Button) fn.target;
            tmp.Translate(dx, dy);
        }
        else if (fn.target instanceof Joystick)
        {
            Joystick tmp = (Joystick) fn.target;
            tmp.Translate(dx, dy);
        }
        //k
        else if (fn.target instanceof Disc)
        {
            Disc tmp = (Disc) fn.target;
            tmp.Translate(dx, dy);
        }
    }

    public void UiOnTouchEvent(FingerUi fn, MotionEvent event)
    {
        int act = 0;
        if (event.getPointerId(event.getActionIndex()) == fn.id)
        {
            if ((event.getActionMasked() == MotionEvent.ACTION_DOWN) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_DOWN))
                act = 1;
            if ((event.getActionMasked() == MotionEvent.ACTION_UP) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_UP) || (event.getActionMasked() == MotionEvent.ACTION_CANCEL))
                act = -1;
        }

        int x = (int) event.getX(event.findPointerIndex(fn.id));
        int y = (int) event.getY(event.findPointerIndex(fn.id));

        if (fn.target == mover)
        {
            fn.onTouchEvent(event);
            return;
        }

        if (act == 1)
        {
            fn.lastx = x;
            fn.lasty = y;
            fn.movd = false;
        }

        int dx = downtostep(x, fn.lastx);
        fn.lastx += dx;
        int dy = downtostep(y, fn.lasty);
        fn.lasty += dy;
        if ((dx != 0) || (dy != 0))
        {
            fn.movd = true;
            ModifyTgt(fn, dx, dy);
            RefreshTgt(fn);
        }

        if ((act == -1) && (!fn.movd))
        {
            mover.show(x, y, fn);
        }

        //k: renormalize position
        if (act == -1 && fn.movd)
        {
            if (NormalizeTgtPosition(fn))
                RefreshTgt(fn);
        }
    }

    public void SaveAll()
    {
        Editor mEdtr = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
        for (int i = 0; i < touch_elements.size(); i++)
        {
            TouchListener touchListener = touch_elements.get(i);
            if (touchListener instanceof Button)
            {
                Button tmp = (Button) touchListener;
                mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.width, (int) (tmp.alpha * 100)).SaveToString(width, height));
            }
            else if (touchListener instanceof Slider)
            {
                Slider tmp = (Slider) touchListener;
                mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.width, (int) (tmp.alpha * 100)).SaveToString(width, height));
            }
            else if (touchListener instanceof Joystick)
            {
                Joystick tmp = (Joystick) touchListener;
                mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.size / 2, (int) (tmp.alpha * 100)).SaveToString(width, height));
            }
            //k
            else if (touchListener instanceof Disc)
            {
                Disc tmp = (Disc) touchListener;
                mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.size / 2, (int) (tmp.alpha * 100)).SaveToString(width, height));
            }
        }
        mEdtr.commit();
        m_edited = false;
    }

    UiLoader      uildr;
    UiViewOverlay mover;

    public int yoffset = 0;
    FloatBuffer linebuffer = ByteBuffer.allocateDirect(4 * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
    FloatBuffer notifybuffer = ByteBuffer.allocateDirect(4 * 8).order(ByteOrder.nativeOrder()).asFloatBuffer();
    private FloatBuffer notifyTexCoordBuffer = ByteBuffer.allocateDirect(4 * 8).order(ByteOrder.nativeOrder()).asFloatBuffer();

    @Override
    public void onSurfaceChanged(GL10 gl, int w, int h)
    {
        if (!mInit)
        {
            width = w;
            height = h;

            gl.glViewport(0, 0, width, height);
            float ratio = (float) width / height;
            gl.glMatrixMode(GL10.GL_PROJECTION);
            gl.glLoadIdentity();
            gl.glFrustumf(-ratio, ratio, -1, 1, 1, 10);
            gl.glEnable(GL10.GL_TEXTURE_2D);
            gl.glEnable(GL10.GL_BLEND);
            gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
            gl.glClearColor(0, 0, 0, 1);

            float[] line = {0, height, width, height};
            linebuffer.put(line);
            linebuffer.position(0);

            gl.glLineWidth(4);

            float[] notifyquad = {0, 0, width, 0, 0, height / 8, width, height / 8};
            notifybuffer.put(notifyquad);
            notifybuffer.position(0);

            float[] notifyquadTexCoord = {0, 0, 1, 0, 0, 1, 1, 1};
            notifyTexCoordBuffer.put(notifyquadTexCoord);
            notifyTexCoordBuffer.position(0);

            uildr = new UiLoader(this, gl, width, height, false);

            final int moverWidth = Math.min(width / 4, 360);
            final int moverHeight = Math.min(width / 6, 320);
            if(true)
                mover = new MenuOverlayView(moverWidth, layout, this);
            else
                mover = new MenuOverlay(0, 0, moverWidth, moverHeight, this);
            mover.loadtex(gl);

            for (int i = 0; i < Q3EUtils.q3ei.UI_SIZE; i++)
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


            int[] notifyColor = { 102, 255, 255, 255 };
            int[] notifyOpenTextColor = { 204, 0, 255, 0 };
            int[] notifyCloseTextColor = { 204, 255, 0, 0 };
            drawerTextureId[0] = KGLBitmapTexture.GenRectTexture(gl, width, height / 8, notifyColor, "↑↑↑ " + Q3ELang.tr(getContext(), R.string.open) + " ↑↑↑", height / 16, notifyOpenTextColor);
            drawerTextureId[1] = KGLBitmapTexture.GenRectTexture(gl, width, height / 8, notifyColor, "↓↓↓ " + Q3ELang.tr(getContext(), R.string.close) + " ↓↓↓", height / 16, notifyCloseTextColor);

            if(m_unit <= 0)
                m_unit = GetPerfectGridSize();
            MakeGrid();

            mInit = true;
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (!mInit) return true;

        event.offsetLocation(0, yoffset);
        int actionMasked = event.getActionMasked();
        int actionIndex = event.getActionIndex();

        if ((actionMasked == MotionEvent.ACTION_DOWN) || (actionMasked == MotionEvent.ACTION_POINTER_DOWN))
        {
            int pid = event.getPointerId(actionIndex);
            int x = (int) event.getX(actionIndex);
            int y = (int) event.getY(actionIndex);

            if (mover.isInside(x, y))
            {
                fingers[pid].target = mover;
            }
            else
            {
                for (TouchListener tl : touch_elements)
                {
                    if (tl.isInside(x, y))
                    {
                        fingers[pid].target = tl;
                        break;
                    }
                }
            }

            if (fingers[pid].target == null)
            {
                mover.hide();

                if ((yoffset == 0) && (y > height - height / 6))
                {
                    yoffset = Math.max(height / 3, m_drawerHeight);
                }
                else
                {
                    if (y < yoffset + height / 6)
                        yoffset = 0;
                }

            }
        }

        for (FingerUi f : fingers)
            if (f.target != null)
            {
                if ((f.target != mover) && (!touch_elements.contains(f.target)))
                    f.target = null;
                else
                    UiOnTouchEvent(f, event);
            }

        if ((actionMasked == MotionEvent.ACTION_UP) || (actionMasked == MotionEvent.ACTION_POINTER_UP) || (actionMasked == MotionEvent.ACTION_CANCEL))
        {
            int pid = event.getPointerId(actionIndex);
            fingers[pid].target = null;
        }

        return true;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        if (mInit)
        {
            mover.loadtex(gl);
            for (Paintable p : paint_elements)
            {
                p.loadtex(gl);
                p.AsBuffer((GL11) gl);
            }
        }
    }

    private void MakeGrid()
    {
        if (m_unit <= 1)
            return;

        final int countX = width / m_unit + (width % m_unit != 0 ? 1 : 0);
        final int countY = height / m_unit + (height % m_unit != 0 ? 1 : 0);
        if (countX <= 0 || countY <= 0)
            return;

        final int w = countX * m_unit;
        final int h = countY * m_unit;
        m_numGridLineVertex = ((countX + 1) + (countY + 1)) * 2;
        final int numFloats = 2 * m_numGridLineVertex;
        final int sizeof_float = 4;
        m_gridBuffer = ByteBuffer.allocateDirect(numFloats * sizeof_float).order(ByteOrder.nativeOrder()).asFloatBuffer();
        float[] vertexBuf = new float[numFloats];
        int ptr = 0;
        for (int i = 0; i <= countX; i++)
        {
            vertexBuf[ptr * 2 * 2] = m_unit * i; // [(x,), ()]
            vertexBuf[ptr * 2 * 2 + 1] = 0; // [(, y), ()]
            vertexBuf[ptr * 2 * 2 + 2] = m_unit * i; // [(), (x, )]
            vertexBuf[ptr * 2 * 2 + 3] = h; // [(), (, y)]
            ptr++;
        }
        for (int i = 0; i <= countY; i++)
        {
            vertexBuf[ptr * 2 * 2] = 0; // [(x,), ()]
            vertexBuf[ptr * 2 * 2 + 1] = m_unit * i; // [(, y), ()]
            vertexBuf[ptr * 2 * 2 + 2] = w; // [(), (x, )]
            vertexBuf[ptr * 2 * 2 + 3] = m_unit * i; // [(), (, y)]
            ptr++;
        }
        m_gridBuffer.put(vertexBuf);
        m_gridBuffer.position(0);
    }

    @Override
    protected void onDetachedFromWindow()
    {
        super.onDetachedFromWindow();
        //Q3EGL.usegles20 = true;
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

        synchronized (paint_elements)
        {
            for (Paintable p : paint_elements)
            {
                p.alpha = alpha;
            }
            m_edited = true;
        }
    }

    public void UpdateOnScreenButtonsSize(float scale)
    {
        if (!mInit)
            return;
        if (scale <= 0.0f)
            return;

        synchronized (paint_elements)
        {
            int[] defSizes = Q3EControls.GetDefaultSize((Activity) getContext(), true);
            for (int i = 0; i < paint_elements.size(); i++)
            {
                Paintable p = paint_elements.get(i);
                float size = defSizes[i];

                if (p instanceof Slider)
                {
                    Slider tmp = (Slider) p;
                    float aspect = Slider.CalcAspect(tmp.Style());
                    int width = (int) (size * scale);
                    int height = (int) (aspect * width + 0.5f);
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(width, height);
                            reloadList.add(tmp);
                        }
                    });
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    float aspect = Button.CalcAspect(tmp.Style());
                    int width = (int) (size * scale);
                    int height = (int) (aspect * width + 0.5f);
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(width, height);
                            reloadList.add(tmp);
                        }
                    });
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    int radius = (int) (size * scale) * 2;
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(radius / 2);
                            reloadList.add(tmp);
                        }
                    });
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    int radius = (int) (size * scale) * 2;
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(radius / 2);
                            reloadList.add(tmp);
                        }
                    });
                }

                m_edited = true;
            }
        }

        //requestRender();
    }

    public void ResetOnScreenButtonSize(TouchListener tgt)
    {
        if (!mInit || null == tgt)
            return;

        synchronized (paint_elements)
        {
            for (int i = 0; i < paint_elements.size(); i++)
            {
                TouchListener tl = touch_elements.get(i);
                if(tl != tgt)
                    continue;

                int[] defSizes = Q3EControls.GetDefaultSize((Activity) getContext(), true);
                Paintable p = paint_elements.get(i);
                int size = defSizes[i];

                if (p instanceof Slider)
                {
                    Slider tmp = (Slider) p;
                    float aspect = Slider.CalcAspect(tmp.Style());
                    int width = size;
                    int height = (int) (aspect * width + 0.5f);
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(width, height);
                            reloadList.add(tmp);
                        }
                    });
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    float aspect = Button.CalcAspect(tmp.Style());
                    int width = size;
                    int height = (int) (aspect * width + 0.5f);
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(width, height);
                            reloadList.add(tmp);
                        }
                    });
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    int radius = size * 2;
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(radius / 2);
                            reloadList.add(tmp);
                        }
                    });
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    int radius = size * 2;
                    post(new Runnable() {
                        @Override
                        public void run() {
                            tmp.Resize(radius / 2);
                            reloadList.add(tmp);
                        }
                    });
                }

                m_edited = true;
            }
        }

        //requestRender();
    }

    public void UpdateOnScreenButtonsPosition(boolean friendly)
    {
        UpdateOnScreenButtonsPosition(friendly, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE);
    }

    public void UpdateOnScreenButtonsPosition(boolean friendly, float scale)
    {
        if (!mInit)
            return;

        synchronized (paint_elements)
        {
            Context context = getContext();
            Point[] points = Q3EControls.GetDefaultPosition((Activity) context, friendly, scale, true);
            for (int i = 0; i < paint_elements.size(); i++)
            {
                Paintable p = paint_elements.get(i);
                Point point = points[i];
                Point absXY = Q3EButtonLayoutManager.ToAbsPosition(point.x, point.y, width, height);
                int x = absXY.x;
                int y = absXY.y;
                if (p instanceof Slider)
                {
                    Slider tmp = (Slider) p;
                    tmp.SetPosition(x, y);
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    tmp.SetPosition(x, y);
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    tmp.SetPosition(x, y);
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    tmp.SetPosition(x, y);
                }
            }
        }

        //requestRender();
    }

    public void ResetOnScreenButtonPosition(TouchListener tgt)
    {
        if (!mInit || null == tgt)
            return;

        synchronized (paint_elements)
        {
            Q3EUiConfig uiConfig = (Q3EUiConfig) getContext();
            Point[] points = Q3EControls.GetDefaultPosition(uiConfig, uiConfig.FriendlyEdge(), -1.0f, true);

            for (int i = 0; i < paint_elements.size(); i++)
            {
                TouchListener tl = touch_elements.get(i);
                if(tl != tgt)
                    continue;

                Paintable p = paint_elements.get(i);
                Point point = points[i];
                Point absXY = Q3EButtonLayoutManager.ToAbsPosition(point.x, point.y, width, height);
                int x = absXY.x;
                int y = absXY.y;
                if (p instanceof Slider)
                {
                    Slider tmp = (Slider) p;
                    tmp.SetPosition(x, y);
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    tmp.SetPosition(x, y);
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    tmp.SetPosition(x, y);
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    tmp.SetPosition(x, y);
                }

                m_edited = true;
            }
        }

        //requestRender();
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

        if(unit < 0)
            unit = 0;

        synchronized (m_gridLock)
        {
            m_numGridLineVertex = 0;
            m_gridBuffer = null;
            if(unit <= 0)
                m_unit = GetPerfectGridSize();
            else
                m_unit = unit;
            MakeGrid();
        }

        //requestRender();
    }

    private int GetPerfectGridSize(int PERFECT, int UNIT, int P)
    {
        int res = UNIT;

        int w = width;
        int h = height;
        if(w % UNIT != 0)
            w -= (w % UNIT);
        if(h % UNIT != 0)
            h -= (h % UNIT);
        int len = Math.min(w, h);

        for(int i = res; i < len; i += P)
        {
            if(w % i != 0)
                continue;
            if(h % i != 0)
                continue;

            int diffa = i - PERFECT;
            int diffb = res - PERFECT;
            int diffabsa = Math.abs(diffa);
            int diffabsb = Math.abs(diffb);
            if(diffabsa < diffabsb)
                res = i;
            else if(diffabsa == diffabsb)
            {
                if(diffa > diffb)
                    res = i;
            }
        }
        Log.i("Q3EUiView", "Get perfected unit(min unit=" + UNIT + ", perfect unit=" + PERFECT + ") -> " + res);
        return res;
    }

    private int GetPerfectGridSize()
    {
        final int PERFECT = 50;
        int res = GetPerfectGridSize(PERFECT, 2, 2);

        if(res <= 2 * 5)
        {
            Log.i("Q3EUiView", "Unable get perfected unit, try to multiple of 5");
            res = GetPerfectGridSize(PERFECT, 5, 5);
            if(res <= 2 * 10)
            {
                Log.i("Q3EUiView", "Unable get perfected unit, try to multiple of 10");
                res = GetPerfectGridSize(PERFECT, 10, 2);
            }
        }

        Log.i("Q3EUiView", "GetPerfectGridSize -> " + res);
        return res;
    }

    public void SetModified()
    {
        m_edited = true;
    }

    public boolean IsModified()
    {
        return m_edited;
    }

    public void SetLayout(ViewGroup layout)
    {
        this.layout = layout;
    }

    public void ReloadButton(Paintable paintable)
    {
        synchronized(paint_elements) {
            reloadList.add(paintable);
        }
    }
}
