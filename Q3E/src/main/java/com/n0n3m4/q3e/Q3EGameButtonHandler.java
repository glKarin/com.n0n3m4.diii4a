package com.n0n3m4.q3e;

import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.opengl.GLES11;
import android.opengl.GLSurfaceView;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Toast;

import com.n0n3m4.q3e.control.Q3EControllerControl;
import com.n0n3m4.q3e.control.Q3EGyroscopeControl;
import com.n0n3m4.q3e.control.Q3EMouseControl;
import com.n0n3m4.q3e.control.Q3ETrackballControl;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.onscreen.Finger;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiLoader;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

class Q3EGameButtonHandler extends Q3EOnScreenButtonHandler
{
    private final TouchListener[] handle_elements = new TouchListener[10]; // handled elements in every touch event
    private final ArrayList<TouchListener> touch_elements = new ArrayList<>(0);
    private final ArrayList<Paintable> paint_elements = new ArrayList<>(0);

    // map volume key function
    private boolean mapvol = false;

    // map back key function
    private int m_mapBack = Q3EGlobals.ENUM_BACK_ALL;

    private long m_lastPressBackTime = -1;
    private int m_pressBackCount = 0;

    /// gyroscope function
    private Q3EGyroscopeControl  gyroscopeControl  = null;
    /// MOUSE
    private Q3EMouseControl      mouseControl      = null;
    /// MOUSE
    private Q3EControllerControl controllerControl = null;
    private Q3ETrackballControl  trackballControl;

    //private boolean usesCSAA = false;

    private final Q3EControlView controlView;

    public Q3EGameButtonHandler(GLSurfaceView surfaceView, ArrayList<TouchListener> touch_elements, ArrayList<Paintable> paint_elements, Finger[] fingers)
    {
        super(surfaceView, touch_elements, paint_elements, fingers);
        this.controlView = (Q3EControlView)surfaceView;
    }

    @Override
    void OnDrawFrame(GL10 gl)
    {
        controllerControl.Update();

/*        if (usesCSAA)
        {
            if (!Q3EGL.usegles20)
                gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
            else
                GLES20.glClear(0x8000);
        }
        else
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);*/
//        if (usesCSAA)
//            gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
//        else
            gl.glClear(GLES11.GL_COLOR_BUFFER_BIT);

        //k: not render in game loading
        if (Q3EUtils.q3ei.callbackObj.inLoading)
            return;

        //Onscreen buttons:
        //save state

/*        if (Q3EGL.usegles20)
        {
            //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            //XXXXXXXXXXXXXXXXXXXXXXXX  GL 20  XXXXXXXXXXXXXXXXXXXXXXXXXX
            //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            for (Paintable p : paint_elements)
                p.Paint((GL11) gl);
        }
        else
        {*/
        //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        //XXXXXXXXXXXXXXXXXXXXXXXX  GL 11  XXXXXXXXXXXXXXXXXXXXXXXXXX
        //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

            /*gl.glMatrixMode(gl.GL_PROJECTION);
            gl.glLoadIdentity();
            gl.glOrthof(0, orig_width, orig_height, 0, -1, 1);*/
        for (Paintable p : paint_elements)
            p.Paint((GL11) gl);
        /*        }*/
    }

    @Override
    boolean OnKeyUp(int keyCode, KeyEvent event)
    {
        if ((!mapvol) && ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) || (keyCode == KeyEvent.KEYCODE_VOLUME_UP)))
            return false;
        if (keyCode == KeyEvent.KEYCODE_BACK)
        {
            if (m_mapBack == Q3EGlobals.ENUM_BACK_NONE)
                return true;
            if ((m_mapBack & Q3EGlobals.ENUM_BACK_EXIT) != 0 && HandleBackPress())
                return true;
            Q3EUtils.ToggleToolbar(false);
        }
        int qKeyCode;
        switch (keyCode)
        {
            case KeyEvent.KEYCODE_VOLUME_UP:
                qKeyCode = Q3EUtils.q3ei.VOLUME_UP_KEY_CODE;
                break;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                qKeyCode = Q3EUtils.q3ei.VOLUME_DOWN_KEY_CODE;
                break;
            default:
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event.getUnicodeChar(0), event);
                break;
        }
        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, qKeyCode, getCharacter(keyCode, event));
        return true;
    }

    @Override
    boolean OnKeyDown(int keyCode, KeyEvent event)
    {
        if ((!mapvol) && ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) || (keyCode == KeyEvent.KEYCODE_VOLUME_UP)))
            return false;
        if (keyCode == KeyEvent.KEYCODE_BACK && (m_mapBack & Q3EGlobals.ENUM_BACK_ESCAPE) == 0)
        {
            return true;
        }
        int qKeyCode;
        switch (keyCode)
        {
            case KeyEvent.KEYCODE_VOLUME_UP:
                qKeyCode = Q3EUtils.q3ei.VOLUME_UP_KEY_CODE;
                break;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                qKeyCode = Q3EUtils.q3ei.VOLUME_DOWN_KEY_CODE;
                break;
            default:
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event.getUnicodeChar(0), event);
                break;
        }
        int t = getCharacter(keyCode, event);
        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, qKeyCode, t);
        return true;
    }

    @Override
    boolean OnKeyMultiple(int keyCode, int repeatCount, KeyEvent event)
    {
        if(keyCode == KeyEvent.KEYCODE_UNKNOWN && event.getAction() == KeyEvent.ACTION_MULTIPLE)
        {
            String characters = event.getCharacters();
            if(null != characters)
            {
                int length = characters.length();
                if(length == 1)
                    Q3EUtils.q3ei.callbackObj.sendCharEvent(characters.charAt(0));
                else if(length > 1)
                    Q3EUtils.q3ei.callbackObj.sendTextEvent(characters);
            }
        }
        return controlView.onKeyMultiple(keyCode, repeatCount, event);
    }

    @Override
    boolean OnTouchEvent(MotionEvent event)
    {
        if(null != mouseControl && event.getSource() == InputDevice.SOURCE_MOUSE && (mouseControl.IsUsingMouse()/* || hideonscr*/))
        {
            event.setAction(MotionEvent.ACTION_CANCEL);
            return true;
        }

        int actionIndex = event.getActionIndex();
        int pid = event.getPointerId(actionIndex);
        if(pid >= fingers.length)
        {
            return true;
        }

        int actionMasked = event.getActionMasked();
        if ((actionMasked == MotionEvent.ACTION_DOWN) || (actionMasked == MotionEvent.ACTION_POINTER_DOWN))
        {
            int x = (int) event.getX(actionIndex);
            int y = (int) event.getY(actionIndex);
            for (TouchListener tl : touch_elements)
            {
                if (tl.isInside(x, y))
                {
                    fingers[pid].target = tl;
                    break;
                }
            }
        }

        //k try
        {
            Arrays.fill(handle_elements, null);
            int handledIndexOfElements = 0;
            for (Finger f : fingers)
            {
                if (f.target != null)
                {
                    // check is handled: only once on a button
                    int i = 0;
                    while(i < handledIndexOfElements)
                    {
                        if(null == handle_elements[i] || handle_elements[i] == f.target)
                            break;
                        i++;
                    }
                    if(i < handledIndexOfElements)
                    {
                        // if(!f.target.SupportMultiTouch())
                        continue;
                    }
                    else
                    {
                        handle_elements[handledIndexOfElements] = f.target;
                        handledIndexOfElements++;
                    }

                    if (!f.onTouchEvent(event))
                        f.target = null;
                }
            }
        }
        //k catch (Exception ignored) { }

        if ((actionMasked == MotionEvent.ACTION_UP) || (actionMasked == MotionEvent.ACTION_POINTER_UP) || (actionMasked == MotionEvent.ACTION_CANCEL))
        {
            fingers[pid].target = null;
        }

        return true;
    }

    @Override
    boolean OnGenericMotionEvent(MotionEvent event)
    {
        if(null != mouseControl && mouseControl.OnGenericMotionEvent(event))
            return true;
        return (null != controllerControl && controllerControl.OnGenericMotionEvent(event));
    }

    @Override
    boolean OnTrackballEvent(MotionEvent event)
    {
        return trackballControl.OnTrackballEvent(event);
    }

    @Override
    void OnSensorChanged(SensorEvent event)
    {
        if (event.sensor.getType() == Sensor.TYPE_GYROSCOPE)
        {
            if(null != gyroscopeControl)
                gyroscopeControl.OnSensorChanged(event);
        }
    }

    @Override
    boolean onCapturedPointerEvent(MotionEvent event)
    {
        return null != mouseControl && mouseControl.OnCapturedPointerEvent(event);
    }

    @Override
    void OnAccuracyChanged(Sensor sensor, int accuracy)
    {

    }

    @Override
    void Pause()
    {
        if(null != gyroscopeControl)
            gyroscopeControl.Pause();
    }

    @Override
    void Resume()
    {
        if(null != gyroscopeControl)
            gyroscopeControl.Resume();
    }

    @Override
    void OnDetachedFromWindow()
    {

    }

    @Override
    void OnWindowFocusChanged(boolean hasWindowFocus)
    {
        if(null != mouseControl)
            mouseControl.OnWindowFocusChanged(hasWindowFocus);
    }

    @Override
    void OnSurfaceCreated(GL10 gl, EGLConfig config)
    {
        super.OnSurfaceCreated(gl, config);
    }

    @Override
    void OnSurfaceChanged(GL10 gl, int w, int h)
    {
        super.OnSurfaceChanged(gl, w, h);
        if(null != mouseControl)
            mouseControl.Start();
        GLBegin();
    }

    @Override
    void GLBegin()
    {
        // onSurfaceCreated
/*        if (Q3EGL.usegles20)
        {
            Q3EGL.initGL20();
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

            GLES20.glDepthMask(false);
            GLES20.glDisable(GLES20.GL_CULL_FACE);
            GLES20.glDisable(GLES20.GL_DEPTH_TEST);
            GLES20.glEnable(GLES20.GL_BLEND);
            GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {*/
        gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        //gl.glDisable(gl.GL_CULL_FACE);
        gl.glDisable(gl.GL_DEPTH_TEST);
        gl.glDisable(gl.GL_ALPHA_TEST);
        gl.glEnable(gl.GL_BLEND);
        gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);

        ((GL11) gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);
        gl.glEnableClientState(gl.GL_VERTEX_ARRAY);
        gl.glEnableClientState(gl.GL_TEXTURE_COORD_ARRAY);
        /*        }*/

        // onSurfaceChanged
/*            if(!Q3EGL.usegles20)
            {*/
        gl.glViewport(0, 0, Q3E.orig_width, Q3E.orig_height);
        gl.glMatrixMode(gl.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glOrthof(0, Q3E.orig_width, Q3E.orig_height, 0, -1, 1);
        /*            }*/
    }

    @Override
    void GLEnd()
    {
    }

    @Override
    void Begin()
    {
        UpdateUsedTouches();
    }

    @Override
    void End()
    {
    }

    @Override
    void OnCreate(Context context)
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(context);

        mapvol = mPrefs.getBoolean(Q3EPreference.pref_mapvol, false);
        m_mapBack = mPrefs.getInt(Q3EPreference.pref_harm_mapBack, Q3EGlobals.ENUM_BACK_ALL); //k

        SetupGyroscope();
        SetupMouse();
        SetupController();
        trackballControl = new Q3ETrackballControl(controlView);
    }

    private boolean HandleBackPress()
    {
        if ((m_mapBack & Q3EGlobals.ENUM_BACK_EXIT) == 0)
            return false;
        boolean res;
        long now = System.currentTimeMillis();
        if (m_lastPressBackTime > 0 && now - m_lastPressBackTime <= Q3EGlobals.CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL)
        {
            m_pressBackCount++;
            res = true;
        } else
        {
            m_pressBackCount = 1;
            res = (m_mapBack & Q3EGlobals.ENUM_BACK_ESCAPE) == 0;
        }
        m_lastPressBackTime = now;
        if (m_pressBackCount == Q3EGlobals.CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT - 1)
            Toast.makeText(getContext(), R.string.click_back_again_to_exit, Toast.LENGTH_LONG).show();
        else if (m_pressBackCount == Q3EGlobals.CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT)
        {
            Q3E.activity.Quit();
            return true;
        }
        return res;
    }

    public int getCharacter(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_DEL) return '\b';
        return event.getUnicodeChar();
    }

    private void SetupGyroscope()
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this.getContext());

        if(Q3EUtils.q3ei.view_motion_control_gyro)
        {
            gyroscopeControl = new Q3EGyroscopeControl(controlView);
            gyroscopeControl.EnableGyroscopeControl(true);
            float gyroXSens = mPrefs.getFloat(Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EGlobals.GYROSCOPE_X_AXIS_SENS);
            float gyroYSens = mPrefs.getFloat(Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EGlobals.GYROSCOPE_Y_AXIS_SENS);
            if(gyroXSens != 0.0f || gyroYSens != 0.0f)
                gyroscopeControl.SetGyroscopeSens(gyroXSens, gyroYSens);

            KLog.I("Enable gyroscope control: x=%f, y=%f", gyroscopeControl.XAxisGyroSens(), gyroscopeControl.YAxisGyroSens());
        }
    }

    private void SetupMouse()
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this.getContext());

        boolean usingMouse = mPrefs.getBoolean(Q3EPreference.pref_harm_using_mouse, false);

        if(usingMouse)
        {
            mouseControl = new Q3EMouseControl(controlView);
            if(!mouseControl.Init())
                mouseControl = null;
            else
                KLog.I("Enable mouse control: " + (mouseControl.IsUsingMouseEvent() ? "mouse event" : "mouse device"));
        }
    }

    private void SetupController()
    {
        controllerControl = new Q3EControllerControl(controlView);
        controllerControl.Init();
        KLog.I("Enable controller");
    }

    void EnableGyroscopeControl(boolean b)
    {
        if(b)
        {
            if (null == gyroscopeControl)
                gyroscopeControl = new Q3EGyroscopeControl(controlView);
            gyroscopeControl.EnableGyroscopeControl(true);
        }
        else
        {
            if (null != gyroscopeControl)
                gyroscopeControl.EnableGyroscopeControl(false);
        }
    }

    void GrabMouse()
    {
        if(null != mouseControl)
        {
            mouseControl.GrabMouse();
        }
    }

    void UnGrabMouse()
    {
        if(null != mouseControl)
        {
            mouseControl.UnGrabMouse();
        }
    }

    boolean IsUsingMouse()
    {
        return null != mouseControl && mouseControl.IsUsingMouse();
    }

    boolean IsUsingMouseEvent()
    {
        return null != mouseControl && mouseControl.IsUsingMouseEvent();
    }

    void UpdateUsedTouches()
    {
        if(NoTouchElements())
            return;

        List<TouchListener> touchs = new ArrayList<>(0);
        List<Paintable> paints = new ArrayList<>(0);

        UiLoader uildr = new UiLoader(controlView, gl, Q3E.orig_width, Q3E.orig_height, Q3EUtils.q3ei.defaults_table);

        for (int i = 0; i < Q3EUtils.q3ei.UI_SIZE; i++)
        {
            boolean visible = uildr.CheckVisible(i);
            Log.i("Q3EControlView", "On-screen button " + i + " -> " + (visible ? "show" : "hide"));
            if(!visible)
                continue;
            touchs.add(total_touch_elements.get(i));
            paints.add(total_paint_elements.get(i));
        }

        for(int i = Q3EUtils.q3ei.UI_SIZE; i < total_touch_elements.size(); i++)
        {
            TouchListener touchListener = total_touch_elements.get(i);
            touchs.add(touchListener);
        }

        // sort
        TouchListener[] touchListeners = touchs.toArray(new TouchListener[0]);
        Arrays.sort(touchListeners, new TouchListener.TouchListenerCmp());

        touch_elements.clear();
        paint_elements.clear();
        touch_elements.addAll(Arrays.asList(touchListeners));
        paint_elements.addAll(paints);

        KLog.I("Total buttons = %d, paint buttons = %d, invisible buttons = %d", total_paint_elements.size(), paint_elements.size(), total_paint_elements.size() - paint_elements.size());
    }
}
