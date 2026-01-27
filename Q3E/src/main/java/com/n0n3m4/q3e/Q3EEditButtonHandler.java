package com.n0n3m4.q3e;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.opengl.GLSurfaceView;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewGroup;

import com.n0n3m4.q3e.gl.KGLBitmapTexture;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.onscreen.Button;
import com.n0n3m4.q3e.onscreen.Disc;
import com.n0n3m4.q3e.onscreen.Finger;
import com.n0n3m4.q3e.onscreen.FingerUi;
import com.n0n3m4.q3e.onscreen.Joystick;
import com.n0n3m4.q3e.onscreen.MenuOverlay;
import com.n0n3m4.q3e.onscreen.MenuOverlayView;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.Q3EButtonLayoutManager;
import com.n0n3m4.q3e.onscreen.Q3EControls;
import com.n0n3m4.q3e.onscreen.Slider;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiElement;
import com.n0n3m4.q3e.onscreen.UiLoader;
import com.n0n3m4.q3e.onscreen.UiViewOverlay;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Q3EEditButtonHandler extends Q3EOnScreenButtonHandler
{
    private       int             m_unit              = 0;
    public        int             step                = 10;
    private final Object          m_gridLock          = new Object();
    private FloatBuffer m_gridBuffer = null;
    private       int             m_numGridLineVertex = 0;
    private       boolean         m_edited            = false;
    private final int[]           drawerTextureId = {0, 0};
    private       int             m_drawerHeight      = 200;
    private final List<Paintable> reloadList      = new ArrayList<>();
    private boolean transparentBackground = false;
    private String m_game = null;
    private boolean portrait = false;
    private boolean writeToDefault = false;
    private boolean saveChanges = false;
    private final ArrayList<TouchListener> touch_elements = new ArrayList<>(0);
    private final ArrayList<Paintable> paint_elements = new ArrayList<>(0);

    UiViewOverlay mover;

    public  int         yoffset      = 0;
    private final FloatBuffer linebuffer = ByteBuffer.allocateDirect(4 * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
    private final FloatBuffer notifybuffer = ByteBuffer.allocateDirect(4 * 8).order(ByteOrder.nativeOrder()).asFloatBuffer();
    private final FloatBuffer notifyTexCoordBuffer = ByteBuffer.allocateDirect(4 * 8).order(ByteOrder.nativeOrder()).asFloatBuffer();

    Q3EEditButtonHandler(GLSurfaceView surfaceView, ArrayList<TouchListener> touch_elements, ArrayList<Paintable> paint_elements, Finger[] fingers)
    {
        super(surfaceView, touch_elements, paint_elements, fingers);
    }

    @Override
    void OnDrawFrame(GL10 gl)
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
                p.PaintInEditor((GL11) gl);
            }
        }

        mover.Paint((GL11) gl);
        gl.glLineWidth(1);
    }

    @Override
    boolean OnKeyUp(int keyCode, KeyEvent event) { return false; }

    @Override
    boolean OnKeyDown(int keyCode, KeyEvent event) { return false; }

    @Override
    boolean OnKeyMultiple(int keyCode, int repeatCount, KeyEvent event) { return false; }

    @Override
    boolean OnTouchEvent(MotionEvent event)
    {
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
                    if (tl.isInside(x, y) && (tl instanceof Paintable))
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

        for (Finger f : fingers)
            if (f.target != null)
            {
                if ((f.target != mover) && (!touch_elements.contains(f.target)))
                    f.target = null;
                else
                    UiOnTouchEvent((FingerUi)f, event);
            }

        if ((actionMasked == MotionEvent.ACTION_UP) || (actionMasked == MotionEvent.ACTION_POINTER_UP) || (actionMasked == MotionEvent.ACTION_CANCEL))
        {
            int pid = event.getPointerId(actionIndex);
            fingers[pid].target = null;
        }

        return true;
    }

    @Override
    boolean OnGenericMotionEvent(MotionEvent event) { return false; }

    @Override
    boolean OnTrackballEvent(MotionEvent event) { return false; }

    @Override
    void OnSensorChanged(SensorEvent event) {}

    @Override
    void OnAccuracyChanged(Sensor sensor, int accuracy) {}

    @Override
    void Pause()
    {
    }

    @Override
    void Resume()
    {
    }

    @Override
    void OnDetachedFromWindow()
    {
        //Q3EGL.usegles20 = true;
    }

    @Override
    void OnWindowFocusChanged(boolean hasWindowFocus) {}

    void OnSurfaceCreated(GL10 gl, EGLConfig config)
    {
        super.OnSurfaceCreated(gl, config);
        mover.loadtex(gl);
    }

    @Override
    void OnSurfaceChanged(GL10 gl, int w, int h)
    {
        super.OnSurfaceChanged(gl, w, h);

        GLBegin();

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

        final int moverWidth = Math.min(width / 4, 360);
        final int moverHeight = Math.min(width / 6, 320);
        if(true)
            mover = new MenuOverlayView(moverWidth, (ViewGroup) surfaceView.getParent(), this);
        else
            mover = new MenuOverlay(0, 0, moverWidth, moverHeight, surfaceView);
        mover.loadtex(gl);

        int[] notifyColor = { 102, 255, 255, 255 };
        int[] notifyOpenTextColor = { 204, 0, 255, 0 };
        int[] notifyCloseTextColor = { 204, 255, 0, 0 };
        drawerTextureId[0] = KGLBitmapTexture.GenRectTexture(gl, width, height / 8, notifyColor, "↑↑↑ " + Q3ELang.tr(getContext(), R.string.open) + " ↑↑↑", height / 16, notifyOpenTextColor);
        drawerTextureId[1] = KGLBitmapTexture.GenRectTexture(gl, width, height / 8, notifyColor, "↓↓↓ " + Q3ELang.tr(getContext(), R.string.close) + " ↓↓↓", height / 16, notifyCloseTextColor);

        if(m_unit <= 0)
            m_unit = GetPerfectGridSize();
        MakeGrid();
    }

    @Override
    void GLBegin()
    {
        // onSurfaceCreated

        // onSurfaceChanged
        gl.glViewport(0, 0, width, height);
/*        float ratio = (float) width / height;
        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glFrustumf(-ratio, ratio, -1, 1, 1, 10);*/
        gl.glEnable(GL10.GL_TEXTURE_2D);
        gl.glEnable(GL10.GL_BLEND);
        gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
        gl.glClearColor(0, 0, 0, transparentBackground ? 0 : 1);
    }

    @Override
    void GLEnd()
    {
    }

    @Override
    void Begin()
    {
        UpdateUsedTouches();
        if(null != mover)
            mover.hide();
        yoffset = 0;
    }

    @Override
    void End()
    {
        if(null != mover)
            mover.hide();
        yoffset = 0;
        if(IsModified())
        {
            Runnable runnable = new Runnable() {
                @Override
                public void run() {
                    SaveAll();
                    if(writeToDefault)
                    {
                        Q3E.q3ei.LoadLayoutTablePreference(getContext(), portrait);
                        KLog.I("Setup layout");
                    }
                    KLog.I("Save game buttons");
                }
            };

            if(saveChanges)
            {
                runnable.run();
            }
            else
            {
                DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which)
                    {
                        switch(which)
                        {
                            case DialogInterface.BUTTON_POSITIVE:
                                runnable.run();
                                dialog.dismiss();
                                break;
                            case DialogInterface.BUTTON_NEGATIVE:
                            default:
                                Recover();
                                dialog.dismiss();
                                break;
                        }
                    }
                };
                AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                builder.setTitle(R.string.warning);
                builder.setMessage(R.string.button_setting_has_changed_can_you_save_it);
                builder.setCancelable(false);
                builder.setPositiveButton(R.string.yes, listener);
                builder.setNegativeButton(R.string.no, listener);
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        }
    }

    @Override
    void OnCreate(Context context)
    {
        Q3EGL.usegles20 = false;
        step = Q3EContextUtils.dip2px(getContext(), 5);
        m_drawerHeight = Q3EContextUtils.dip2px(getContext(), 50) * 3 + Q3EContextUtils.dip2px(getContext(), 5);

        String unit = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.CONTROLS_CONFIG_POSITION_UNIT, "0");
        if (null != unit)
        {
            m_unit = Integer.parseInt(unit);
        }
    }

    @Override
    void GrabMouse() {}

    @Override
    void UnGrabMouse() {}

    @Override
    boolean onCapturedPointerEvent(MotionEvent event) { return false; }

    void UiOnTouchEvent(FingerUi fn, MotionEvent event)
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

    public void SetModified()
    {
        m_edited = true;
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

    void RefreshTgt(FingerUi fn)
    {
        SetModified();

        //PrintInfo(fn);
    }

    void ModifyTgt(FingerUi fn, int dx, int dy)
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

    int downtostep(int a, int la)
    {
        return a - la;
		/*int k=Math.round((float)(a-la)/(float)step);
		//int k=(a-la)/step;
		return k*step;*/
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

    void UpdateGrid(int unit)
    {
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

    public void ReloadButton(Paintable paintable)
    {
        synchronized(paint_elements) {
            reloadList.add(paintable);
        }
    }

    public void ResetOnScreenButtonPosition(TouchListener tgt)
    {
        if (null == tgt)
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

    public void ResetOnScreenButtonSize(TouchListener tgt)
    {
        if (null == tgt)
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
                    tmp.Resize(width, height);
                    ReloadButton(tmp);
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    float aspect = Button.CalcAspect(tmp.Style());
                    int width = size;
                    int height = (int) (aspect * width + 0.5f);
                    tmp.Resize(width, height);
                    ReloadButton(tmp);
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    int radius = size * 2;
                    tmp.Resize(radius / 2);
                    ReloadButton(tmp);
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    int radius = size * 2;
                    tmp.Resize(radius / 2);
                    ReloadButton(tmp);
                }

                m_edited = true;
            }
        }

        //requestRender();
    }

    public void UpdateOnScreenButtonsSize(float scale)
    {
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
                    tmp.Resize(width, height);
                    ReloadButton(tmp);
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    float aspect = Button.CalcAspect(tmp.Style());
                    int width = (int) (size * scale);
                    int height = (int) (aspect * width + 0.5f);
                    tmp.Resize(width, height);
                    ReloadButton(tmp);
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    int radius = (int) (size * scale) * 2;
                    tmp.Resize(radius / 2);
                    ReloadButton(tmp);
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    int radius = (int) (size * scale) * 2;
                    tmp.Resize(radius / 2);
                    ReloadButton(tmp);
                }

                m_edited = true;
            }
        }

        //requestRender();
    }

    public void UpdateOnScreenButtonsOpacity(float alpha)
    {
        synchronized (paint_elements)
        {
            for (Paintable p : paint_elements)
            {
                p.alpha = alpha;
            }
            m_edited = true;
        }
    }

    void SaveAll()
    {
        SharedPreferences.Editor mEdtr = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
        String[] list = new String[Q3EGlobals.UI_SIZE];
        for (int i = 0; i < touch_elements.size(); i++)
        {
            TouchListener touchListener = touch_elements.get(i);
            if (touchListener instanceof Button)
            {
                Button tmp = (Button) touchListener;
                list[i] = new UiElement(tmp.cx, tmp.cy, tmp.width, (int) (tmp.alpha * 100)).SaveToString(width, height);
            }
            else if (touchListener instanceof Slider)
            {
                Slider tmp = (Slider) touchListener;
                list[i] = new UiElement(tmp.cx, tmp.cy, tmp.width, (int) (tmp.alpha * 100)).SaveToString(width, height);
            }
            else if (touchListener instanceof Joystick)
            {
                Joystick tmp = (Joystick) touchListener;
                list[i] = new UiElement(tmp.cx, tmp.cy, tmp.size / 2, (int) (tmp.alpha * 100)).SaveToString(width, height);
            }
            //k
            else if (touchListener instanceof Disc)
            {
                Disc tmp = (Disc) touchListener;
                list[i] = new UiElement(tmp.cx, tmp.cy, tmp.size / 2, (int) (tmp.alpha * 100)).SaveToString(width, height);
            }
        }

        if(null == m_game)
        {
            for(int i = 0; i < list.length; i++)
            {
                mEdtr.putString(Q3EPreference.pref_controlprefix + i, list[i]);
            }
        }
        else
        {
            mEdtr.putString(Q3EInterface.ControlPreference(m_game), Q3EUtils.Join(",", list));
        }
        mEdtr.commit();
        m_edited = false;
    }

    void UpdateOnScreenButtonsPosition(boolean friendly, float scale)
    {
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

    public void post(Runnable runnable)
    {
        surfaceView.post(runnable);
    }

    boolean IsModified()
    {
        return m_edited;
    }

    void TransparentBackground(boolean on)
    {
        transparentBackground = on;
    }

    public void SetEditGame(String game)
    {
        m_game = game;
    }

    public void SetPortrait(boolean portrait)
    {
        this.portrait = portrait;
    }

    public void SetWriteToDefault(boolean writeToDefault)
    {
        this.writeToDefault = writeToDefault;
    }

    public void SetSaveChanges(boolean saveChanges)
    {
        this.saveChanges = saveChanges;
    }

    void UpdateUsedTouches()
    {
        if(NoTouchElements())
            return;

        List<TouchListener> touchs = new ArrayList<>(0);
        List<Paintable> paints = new ArrayList<>(0);

        for (int i = 0; i < Q3E.q3ei.UI_SIZE; i++)
        {
            TouchListener touchListener = total_touch_elements.get(i);
            if(!(touchListener instanceof Paintable))
                continue;
            touchs.add(touchListener);
            paints.add(total_paint_elements.get(i));
        }

        touch_elements.clear();
        paint_elements.clear();
        touch_elements.addAll(touchs);
        paint_elements.addAll(paints);

        KLog.I("Total buttons = %d, edit buttons = %d, non-paint buttons = %d", total_paint_elements.size(), paint_elements.size(), total_paint_elements.size() - paint_elements.size());
    }

    void Recover()
    {
        synchronized (paint_elements)
        {
            List<Paintable> updateList = new ArrayList<>();
            for (int i = 0; i < paint_elements.size(); i++)
            {
                String def = Q3E.q3ei.defaults_table[i];
                UiElement uiElement = new UiElement(def, width, height);

                Paintable p = paint_elements.get(i);

                // recover position
                int x = uiElement.cx;
                int y = uiElement.cy;
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

                // recover alpha
                p.alpha = (float)uiElement.alpha / 100.0f;

                // recover size
                if (p instanceof Slider)
                {
                    Slider tmp = (Slider) p;
                    int width = uiElement.size;
                    int height = Slider.HeightForWidth(width, Q3E.q3ei.arg_table[i * 4 + 3]);
                    tmp.Resize(width, height);
                    updateList.add(p);
                }
                else if (p instanceof Button)
                {
                    Button tmp = (Button) p;
                    int width = uiElement.size;
                    int height = width;
                    tmp.Resize(width, height);
                    updateList.add(p);
                }
                else if (p instanceof Joystick)
                {
                    Joystick tmp = (Joystick) p;
                    int radius = uiElement.size;
                    tmp.Resize(radius);
                    updateList.add(p);
                }
                else if (p instanceof Disc)
                {
                    Disc tmp = (Disc) p;
                    int radius = uiElement.size;
                    tmp.Resize(radius);
                    updateList.add(p);
                }
            }

            surfaceView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    for (Paintable p : updateList)
                    {
                        p.AsBuffer((GL11) gl);
                    }
                }
            });

            m_edited = false;
        }
    }
}
