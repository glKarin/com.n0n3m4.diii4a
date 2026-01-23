package com.n0n3m4.q3e;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.opengl.GLSurfaceView;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.onscreen.Finger;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.TouchListener;

import java.util.ArrayList;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

abstract class Q3EOnScreenButtonHandler
{
    public final GLSurfaceView surfaceView;

    protected final ArrayList<TouchListener> total_touch_elements;
    protected final ArrayList<Paintable> total_paint_elements;
    protected final Finger[] fingers;

    protected GL10 gl;
    public int width;
    public int height;

    public Q3EOnScreenButtonHandler(GLSurfaceView surfaceView, ArrayList<TouchListener> total_touch_elements, ArrayList<Paintable> total_paint_elements, Finger[] fingers)
    {
        this.surfaceView = surfaceView;
        this.total_touch_elements = total_touch_elements;
        this.total_paint_elements = total_paint_elements;
        this.fingers = fingers;
    }

    public Context getContext()
    {
        return surfaceView.getContext();
    }

    protected boolean NoTouchElements()
    {
        return total_touch_elements.isEmpty() || total_paint_elements.isEmpty();
    }

    abstract void OnDrawFrame(GL10 gl);

    abstract boolean OnKeyUp(int keyCode, KeyEvent event);
    abstract boolean OnKeyDown(int keyCode, KeyEvent event);
    abstract boolean OnKeyMultiple(int keyCode, int repeatCount, KeyEvent event);
    abstract boolean OnTouchEvent(MotionEvent event);
    abstract boolean OnGenericMotionEvent(MotionEvent event);
    abstract boolean OnTrackballEvent(MotionEvent event);
    abstract void OnSensorChanged(SensorEvent event);
    abstract void OnAccuracyChanged(Sensor sensor, int accuracy);
    abstract void Pause();
    abstract void Resume();
    abstract void OnDetachedFromWindow();
    abstract void OnWindowFocusChanged(boolean hasWindowFocus);

    void OnSurfaceCreated(GL10 gl, EGLConfig config)
    {
        this.gl = gl;
    }
    void OnSurfaceChanged(GL10 gl, int w, int h)
    {
        this.gl = gl;
        width = w;
        height = h;
    }
    abstract void OnCreate(Context context);
    abstract void GrabMouse();
    abstract void UnGrabMouse();
    abstract boolean onCapturedPointerEvent(MotionEvent event);
    abstract void GLBegin();
    abstract void GLEnd();
    abstract void Begin();
    abstract void End();
    abstract void UpdateUsedTouches();
}
