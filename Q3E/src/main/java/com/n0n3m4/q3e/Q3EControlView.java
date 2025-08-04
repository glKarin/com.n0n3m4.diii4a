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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLES11;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Display;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import com.n0n3m4.q3e.device.Q3EMouseDevice;
import com.n0n3m4.q3e.gl.Q3EConfigChooser;
import com.n0n3m4.q3e.karin.KKeyToolBar;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.onscreen.Button;
import com.n0n3m4.q3e.onscreen.Disc;
import com.n0n3m4.q3e.onscreen.Finger;
import com.n0n3m4.q3e.onscreen.Joystick;
import com.n0n3m4.q3e.onscreen.MouseControl;
import com.n0n3m4.q3e.onscreen.MouseButton;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.Slider;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiLoader;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Q3EControlView extends GLSurfaceView implements GLSurfaceView.Renderer, SensorEventListener
{
    public static final float GYROSCOPE_X_AXIS_SENS = 18;
    public static final float GYROSCOPE_Y_AXIS_SENS = 18;

    // render
    private boolean mInit = false;
    private boolean usesCSAA = false;
    private boolean hideonscr;

    // mouse function
    private boolean m_usingMouse = false;
    private float m_lastMousePosX = -1;
    private float m_lastMousePosY = -1;
    private boolean m_usingMouseDevice = false;
    private Q3EMouseDevice m_mouseDevice = null;
    private int m_requestGrabMouse = 0;
    private boolean m_allowGrabMouse = false;
    private float m_lastTouchPadPosX = -1;
    private float m_lastTouchPadPosY = -1;

    // map volume key function
    private boolean mapvol = false;

    // map back key function
    private int m_mapBack = Q3EGlobals.ENUM_BACK_ALL;
    private long m_lastPressBackTime = -1;
    private int m_pressBackCount = 0;
    private boolean m_portrait = false;

    // controller
    private final boolean[] directionPressed = { false, false, false, false }; // up down left right
    private boolean dpadAsArrowKey = false;
    private float leftJoystickDeadRange = 0.01f;
    private float rightJoystickDeadRange = 0.0f;
    private float rightJoystickSensitivity = 1.0f;


    //RTCW4A-specific
    /*
    private Button actbutton;
    private Button kickbutton;
    */
    private Q3EView m_renderView;

    //MOUSE
    private long oldtime = 0;


    // other controls function
    private float last_joystick_x = 0;
    private float last_joystick_y = 0;

    private final Finger[] fingers = new Finger[10];
    private final ArrayList<TouchListener> touch_elements = new ArrayList<TouchListener>(0);
    private final ArrayList<Paintable> paint_elements = new ArrayList<Paintable>(0);
    private final TouchListener[] handle_elements = new TouchListener[10]; // handled elements in every touch event

    private float last_trackball_x = 0;
    private float last_trackball_y = 0;

    /// gyroscope function
    private boolean m_gyroInited = false;
    private SensorManager m_sensorManager = null;
    private Sensor m_gyroSensor = null;
    private boolean m_enableGyro = false;
    private float m_xAxisGyroSens = GYROSCOPE_X_AXIS_SENS;
    private float m_yAxisGyroSens = GYROSCOPE_Y_AXIS_SENS;
    private Display m_display;

    public Q3EControlView(Context context)
    {
        super(context);

        setEGLConfigChooser(new Q3EConfigChooser(8, 8, 8, 8, 0, /*Q3EGL.usegles20*/false));
        getHolder().setFormat(PixelFormat.RGBA_8888);

/*        if (Q3EGL.usegles20)
            setEGLContextClientVersion(2);*/

        setRenderer(this);

        setFocusable(true);
        setFocusableInTouchMode(true);

        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this.getContext());

        hideonscr = mPrefs.getBoolean(Q3EPreference.pref_hideonscr, false);
        mapvol = mPrefs.getBoolean(Q3EPreference.pref_mapvol, false);
        m_mapBack = mPrefs.getInt(Q3EPreference.pref_harm_mapBack, Q3EGlobals.ENUM_BACK_ALL); //k
        m_portrait = mPrefs.getBoolean(Q3EPreference.pref_harm_portrait, false); //k
        // controller
        dpadAsArrowKey = mPrefs.getBoolean(Q3EPreference.pref_harm_dpad_as_arrow_key, false);
        leftJoystickDeadRange = Q3EPreference.GetFloatFromString(mPrefs, Q3EPreference.pref_harm_left_joystick_deadzone, 0.01f);
        rightJoystickDeadRange = Q3EPreference.GetFloatFromString(mPrefs, Q3EPreference.pref_harm_right_joystick_deadzone, 0.0f);
        rightJoystickSensitivity = Q3EPreference.GetFloatFromString(mPrefs, Q3EPreference.pref_harm_right_joystick_sensitivity, 1.0f);

        boolean usingMouse = mPrefs.getBoolean(Q3EPreference.pref_harm_using_mouse, false);

        KLog.I("Controller DPad as arrow keys: " + dpadAsArrowKey);
        KLog.I("Controller left joystick dead zone: " + leftJoystickDeadRange);
        KLog.I("Controller right joystick dead zone: " + rightJoystickDeadRange);
        KLog.I("Controller right joystick sensitivity: " + rightJoystickSensitivity);

        if(usingMouse)
        {
            int mouse = Q3EUtils.SupportMouse();
            if(mouse == Q3EGlobals.MOUSE_DEVICE)
                m_usingMouseDevice = true;
            else if(mouse == Q3EGlobals.MOUSE_EVENT)
                m_usingMouse = true;
        }
    }
    @Override
    public void onDrawFrame(GL10 gl)
    {
        long t = System.currentTimeMillis();
        float delta = t - oldtime;
        oldtime = t;
        if (delta > 1000)
            delta = 1000;

        delta *= rightJoystickSensitivity;
        if ((last_joystick_x != 0) || (last_joystick_y != 0))
            Q3EUtils.q3ei.callbackObj.sendMotionEvent(delta * last_joystick_x, delta * last_joystick_y);

/*        if (usesCSAA)
        {
            if (!Q3EGL.usegles20)
                gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
            else
                GLES20.glClear(0x8000);
        }
        else
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);*/
        if (usesCSAA)
            gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
        else
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
    public void onSurfaceChanged(GL10 gl, int w, int h)
    {
        if (!mInit)
        {
            KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Control view: %d x %d", w, h);

            if(m_usingMouseDevice)
                m_mouseDevice = new Q3EMouseDevice(this);

            Q3E.orig_width = w;
            Q3E.orig_height = h;
            Q3E.activity.SetupGameViewSize(w, h, false);

            UiLoader uildr = new UiLoader(this, gl, Q3E.orig_width, Q3E.orig_height, m_portrait);

            for (int i = 0; i < Q3EUtils.q3ei.UI_SIZE; i++)
            {
                boolean visible = uildr.CheckVisible(i);
                Log.i("Q3EControlView", "On-screen button " + i + " -> " + (visible ? "show" : "hide"));
                if(!visible)
                    continue;
                Object o = uildr.LoadElement(i, false);
                touch_elements.add((TouchListener) o);
                paint_elements.add((Paintable) o);
            }

            if (hideonscr)
            {
                touch_elements.clear();
            }
            //must be last
            //touch_elements.add(new MouseControl(this, false));
            touch_elements.add(new MouseControl(this));
            SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this.getContext());
            if(mPrefs.getBoolean(Q3EPreference.pref_2fingerlmb, false))
                touch_elements.add(new MouseButton(this));
            //touch_elements.add(new MouseControl(this, false));

            SortOnScreenButtons(); //k sort priority

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
                fingers[i] = new Finger(null, i);

            if(null != m_mouseDevice)
                m_mouseDevice.Start();

/*            if(!Q3EGL.usegles20)
            {*/
            gl.glMatrixMode(gl.GL_PROJECTION);
            gl.glLoadIdentity();
            gl.glOrthof(0, Q3E.orig_width, Q3E.orig_height, 0, -1, 1);
/*            }*/

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
            gl.glDisable(gl.GL_CULL_FACE);
            gl.glDisable(gl.GL_DEPTH_TEST);
            gl.glDisable(gl.GL_ALPHA_TEST);
            gl.glEnable(gl.GL_BLEND);
            gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);

            ((GL11) gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);
            gl.glEnableClientState(gl.GL_VERTEX_ARRAY);
            gl.glEnableClientState(gl.GL_TEXTURE_COORD_ARRAY);
/*        }*/

        if (mInit)
        {
            for (Paintable p : paint_elements)
            {
                p.loadtex(gl);
                p.AsBuffer((GL11) gl);
            }
        }

    }

    public void setState(int st)
    {
        /*
        if (actbutton != null)
            actbutton.alpha = ((st & 1) == 1) ? Math.min(actbutton.initalpha * 2, 1f) : actbutton.initalpha;
        if (kickbutton != null)
            kickbutton.alpha = ((st & 4) == 4) ? Math.min(kickbutton.initalpha * 2, 1f) : kickbutton.initalpha;
            */
    }

    public int getCharacter(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_DEL) return '\b';
        return event.getUnicodeChar();
    }

    //@Override
    public boolean OnKeyDown(int keyCode, KeyEvent event)
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
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event.getUnicodeChar(0));
                break;
        }
        int t = getCharacter(keyCode, event);
        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, qKeyCode, t);
        return true;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
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
        return super.onKeyMultiple(keyCode, repeatCount, event);
    }

    //@Override
    public boolean OnKeyUp(int keyCode, KeyEvent event)
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
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event.getUnicodeChar(0));
                break;
        }
        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, qKeyCode, getCharacter(keyCode, event));
        return true;
    }

    private static float getCenteredAxis(MotionEvent event, InputDevice device, int axis)
    {
        final InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());

        // A joystick at rest does not always report an absolute position of
        // (0,0). Use the getFlat() method to determine the range of values
        // bounding the joystick axis center.
        if (range != null) {
            final float flat = range.getFlat();
            final float value = event.getAxisValue(axis);

            // Ignore axis values that are within the 'flat' region of the
            // joystick axis center.
            if (Math.abs(value) > flat) {
                return value;
            }
        }
        return 0;
    }

    private static float getCenteredAxis(MotionEvent event, InputDevice device, int axis, int historyPos) {
        final InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());

        // A joystick at rest does not always report an absolute position of
        // (0,0). Use the getFlat() method to determine the range of values
        // bounding the joystick axis center.
        if (range != null) {
            final float flat = range.getFlat();
            final float value = historyPos < 0 ? event.getAxisValue(axis): event.getHistoricalAxisValue(axis, historyPos);

            // Ignore axis values that are within the 'flat' region of the
            // joystick axis center.
            if (Math.abs(value) > flat) {
                return value;
            }
        }
        return 0;
    }

    private void HandleDPadMotionEvent(MotionEvent event)
    {
        float xaxis = event.getAxisValue(MotionEvent.AXIS_HAT_X);
        float yaxis = event.getAxisValue(MotionEvent.AXIS_HAT_Y);

        // Check if the AXIS_HAT_X value is -1 or 1, and set the D-pad
        // LEFT and RIGHT direction accordingly.
        boolean leftPressed = Float.compare(xaxis, -1.0f) == 0;
        boolean rightPressed = Float.compare(xaxis, 1.0f) == 0;
        // Check if the AXIS_HAT_Y value is -1 or 1, and set the D-pad
        // UP and DOWN direction accordingly.
        boolean upPressed = Float.compare(yaxis, -1.0f) == 0;
        boolean downPressed = Float.compare(yaxis, 1.0f) == 0;

        if(leftPressed != directionPressed[2])
        {
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(leftPressed, Q3EKeyCodes.KeyCodes.K_LEFTARROW, 0);
            directionPressed[2] = leftPressed;
        }
        if(rightPressed != directionPressed[3])
        {
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(rightPressed, Q3EKeyCodes.KeyCodes.K_RIGHTARROW, 0);
            directionPressed[3] = rightPressed;
        }
        if(upPressed != directionPressed[0])
        {
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(upPressed, Q3EKeyCodes.KeyCodes.K_UPARROW, 0);
            directionPressed[0] = upPressed;
        }
        if(downPressed != directionPressed[1])
        {
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(downPressed, Q3EKeyCodes.KeyCodes.K_DOWNARROW, 0);
            directionPressed[1] = downPressed;
        }
    }

    private void HandleJoyStickMotionEvent(MotionEvent event, InputDevice inputDevice/*, int historyPos*/) {
        float x = getCenteredAxis(event, inputDevice, MotionEvent.AXIS_X/*, historyPos*/);
        float y = getCenteredAxis(event, inputDevice, MotionEvent.AXIS_Y/*, historyPos*/);

        if(dpadAsArrowKey || !Q3EUtils.q3ei.callbackObj.notinmenu)
        {
            HandleDPadMotionEvent(event);
        }
        else
        {
            if(x == 0.0f)
                x = getCenteredAxis(event, inputDevice, MotionEvent.AXIS_HAT_X/*, historyPos*/);
            if(y == 0.0f)
                y = getCenteredAxis(event, inputDevice, MotionEvent.AXIS_HAT_Y/*, historyPos*/);
        }

        Q3EUtils.q3ei.callbackObj.sendAnalog((Math.abs(x) > leftJoystickDeadRange) || (Math.abs(y) > leftJoystickDeadRange), x, -y);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        int action = event.getAction();
        int source = event.getSource();

        if (action == MotionEvent.ACTION_MOVE)
        {
            InputDevice inputDevice = event.getDevice();
            if (((source == InputDevice.SOURCE_JOYSTICK) || (source == InputDevice.SOURCE_GAMEPAD)) && (action == MotionEvent.ACTION_MOVE));
            if ((source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)
            {
                // left as movement event
                // Process all historical movement samples in the batch
//                final int historySize = event.getHistorySize();

                // Process the movements starting from the
                // earliest historical position in the batch
//                for (int i = 0; i < historySize; i++) {
//                    // Process the event at historical position i
//                    HandleJoyStickMotionEvent(event, inputDevice, i);
//                }

                // Process the current movement sample in the batch (position -1)
                HandleJoyStickMotionEvent(event, inputDevice/*, -1*/);

                // right as view event
                float x = getCenteredAxis(event, inputDevice, MotionEvent.AXIS_Z);
                float y = getCenteredAxis(event, inputDevice, MotionEvent.AXIS_RZ);

                if((Math.abs(x) < rightJoystickDeadRange))
                    x = 0.0f;
                if((Math.abs(y) < rightJoystickDeadRange))
                    y = 0.0f;
                
                last_joystick_x = x;
                last_joystick_y = y;

                return true;
            }
            else if ((source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
            {
                if(dpadAsArrowKey || !Q3EUtils.q3ei.callbackObj.notinmenu)
                {
                    HandleDPadMotionEvent(event);
                    return true;
                }
            }
        }

        if(m_usingMouse && source == InputDevice.SOURCE_MOUSE)
        {
            int actionIndex = event.getActionIndex();
            float x = event.getX(actionIndex);
            float y = event.getY(actionIndex);
            if(m_lastMousePosX < 0)
                m_lastMousePosX = x;
            if(m_lastMousePosY < 0)
                m_lastMousePosY = y;
            float deltaX = x - m_lastMousePosX;
            float deltaY = y - m_lastMousePosY;
            m_lastMousePosX = x;
            m_lastMousePosY = y;
            switch (action)
            {
                case MotionEvent.ACTION_BUTTON_PRESS: {
                    int gameMouseButton = ConvMouseButton(event);
                    if(gameMouseButton != 0)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, gameMouseButton, 0);
                    }
                }
                    break;
                case MotionEvent.ACTION_BUTTON_RELEASE: {
                    int gameMouseButton = ConvMouseButton(event);
                    if(gameMouseButton != 0)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, gameMouseButton, 0);
                    }
                    m_lastMousePosX = -1;
                    m_lastMousePosY = -1;
                }
                    break;
//                case MotionEvent.ACTION_HOVER_ENTER: break;
//                case MotionEvent.ACTION_HOVER_EXIT: break;
                case MotionEvent.ACTION_HOVER_MOVE:
                    Q3EUtils.q3ei.callbackObj.sendMotionEvent(deltaX, deltaY);
                    Q3EUtils.q3ei.callbackObj.sendMouseEvent(Q3E.PhysicsToLogicalX(x), Q3E.PhysicsToLogicalY(y));
                    break;
                case MotionEvent.ACTION_SCROLL:
                    float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, actionIndex);
                    if(scrollY > 0)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                    }
                    else if(scrollY < 0)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                    }
                    break;
            }
            return true;
        }
        return false;
    }

    @Override
    public boolean onCapturedPointerEvent(MotionEvent event)
    {
        if(m_usingMouse)
        {
            switch (event.getSource())
            {
                case InputDevice.SOURCE_MOUSE_RELATIVE:
                    return HandleCapturedPointerEvent(event, true);
                case InputDevice.SOURCE_TOUCHPAD:
                    return HandleCapturedPointerEvent(event, false);
            }
        }
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (!mInit) return true;

        int source = event.getSource();
        if(source == InputDevice.SOURCE_MOUSE && (m_usingMouse || m_usingMouseDevice/* || hideonscr*/))
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
    public boolean onTrackballEvent(MotionEvent event)
    {
        float x = event.getX();
        float y = event.getY();
        if (event.getAction() == MotionEvent.ACTION_DOWN)
        {
            last_trackball_x = x;
            last_trackball_y = y;
        }
        final float deltaX = x - last_trackball_x;
        final float deltaY = y - last_trackball_y;
        Q3EJNI.sendMotionEvent(deltaX, deltaY);
        last_trackball_x = x;
        last_trackball_y = y;
        return true;
    }

    public void queueEvent(Runnable r)
    {
        Q3EUtils.q3ei.callbackObj.PushEvent(r);
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
            AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
            builder.setTitle(R.string.exit_game);
            builder.setMessage(R.string.are_you_sure_exit_game);
            builder.setCancelable(true);
            builder.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int v)
                {
                    dialog.dismiss();
                    Q3E.Shutdown();
                }
            });
            builder.setNegativeButton(R.string.cancel, null);
            builder.create().show();
            return true;
        }
        return res;
    }

    public void RenderView(Q3EView view)
    {
        m_renderView = view;
    }

    public boolean EnableGyroscopeControl(boolean... b)
    {
        if (null != b && b.length > 0)
            m_enableGyro = b[0];
        return m_enableGyro;
    }

    public float XAxisSens(float... f)
    {
        if (null != f && f.length > 0)
            m_xAxisGyroSens = f[0];
        return m_xAxisGyroSens;
    }

    public float yAxisSens(float... f)
    {
        if (null != f && f.length > 0)
            m_yAxisGyroSens = f[0];
        return m_yAxisGyroSens;
    }

    public void SetGyroscopeSens(float x, float y)
    {
        XAxisSens(x);
        yAxisSens(y);
    }

    private boolean InitGyroscopeSensor()
    {
        if (m_gyroInited)
            return null != m_gyroSensor;
        m_gyroInited = true;
        m_sensorManager = (SensorManager) getContext().getSystemService(Context.SENSOR_SERVICE);
        if (null == m_sensorManager)
            return false;
        m_gyroSensor = m_sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        m_display = ((WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        return null != m_gyroSensor;
    }

    public void StartGyroscope()
    {
        InitGyroscopeSensor();
        if (null != m_gyroSensor)
            m_sensorManager.registerListener(this, m_gyroSensor, SensorManager.SENSOR_DELAY_GAME);
    }

    public void StopGyroscope()
    {
        if (null != m_gyroSensor)
            m_sensorManager.unregisterListener(this, m_gyroSensor);
    }

    @Override
    public void onSensorChanged(SensorEvent event)
    {
        if (event.sensor.getType() == Sensor.TYPE_GYROSCOPE)
        {
            if (Q3EUtils.q3ei.callbackObj.notinmenu && !Q3EUtils.q3ei.callbackObj.inLoading)
            {
                //if(event.values[0] != 0.0f || event.values[1] != 0.0f)
                {
                    float x, y;
                    switch (m_display.getRotation()) {
                        case Surface.ROTATION_270: // invert
                            x = -event.values[0];
                            y = -event.values[1];
                            break;
                        case Surface.ROTATION_90:
                        default:
                            x = event.values[0];
                            y = event.values[1];
                            break;
                    }

                    Q3EUtils.q3ei.callbackObj.sendMotionEvent(-x * m_xAxisGyroSens, y * m_yAxisGyroSens);
                }
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {

    }

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
        if (m_enableGyro)
            StopGyroscope();
    }

    public void Resume()
    {
        if (m_enableGyro)
            StartGyroscope();
    }

    @Override
    protected void onDetachedFromWindow()
    {
        super.onDetachedFromWindow();

        if(null != Q3E.activity)
            Q3E.activity.GetKeyboard().onDetachedFromWindow();
    }

    private int GetOnScreenType(TouchListener touchListener)
    {
        if(touchListener instanceof Button)
            return Q3EGlobals.TYPE_BUTTON;
        if(touchListener instanceof Slider)
            return Q3EGlobals.TYPE_SLIDER;
        if(touchListener instanceof Joystick)
            return Q3EGlobals.TYPE_JOYSTICK;
        if(touchListener instanceof Disc)
            return Q3EGlobals.TYPE_DISC;
        if(touchListener instanceof MouseControl)
            return Q3EGlobals.TYPE_MOUSE;
        return Q3EGlobals.TYPE_MOUSE_BUTTON;
    }

    private void SortOnScreenButtons()
    {
        TouchListener[] touchListeners = touch_elements.toArray(new TouchListener[0]);
        final List<Integer> Type_Priority = Arrays.asList(
                Q3EGlobals.TYPE_BUTTON,
                Q3EGlobals.TYPE_SLIDER,
                Q3EGlobals.TYPE_DISC,
                Q3EGlobals.TYPE_JOYSTICK,
                Q3EGlobals.TYPE_MOUSE,
                Q3EGlobals.TYPE_MOUSE_BUTTON
        );

        Arrays.sort(touchListeners, new Comparator<TouchListener>() {
            @Override
            public int compare(TouchListener a, TouchListener b)
            {
                int ai = Type_Priority.indexOf(GetOnScreenType(a));
                int bi = Type_Priority.indexOf(GetOnScreenType(b));
                return ai - bi;
            }
        });
        touch_elements.clear();
        touch_elements.addAll(Arrays.asList(touchListeners));
    }

    private int ConvMouseButton(MotionEvent event)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            int actionButton = event.getActionButton();
            switch (actionButton)
            {
                case MotionEvent.BUTTON_PRIMARY:
                case MotionEvent.BUTTON_STYLUS_PRIMARY:
                    return Q3EKeyCodes.KeyCodes.K_MOUSE1;
                case MotionEvent.BUTTON_SECONDARY:
                case MotionEvent.BUTTON_STYLUS_SECONDARY:
                    return Q3EKeyCodes.KeyCodes.K_MOUSE2;
                case MotionEvent.BUTTON_TERTIARY:
                    return Q3EKeyCodes.KeyCodes.K_MOUSE3;
                case MotionEvent.BUTTON_BACK:
                    return Q3EKeyCodes.KeyCodes.K_MOUSE4;
                case MotionEvent.BUTTON_FORWARD:
                    return Q3EKeyCodes.KeyCodes.K_MOUSE5;
                default:
                    return 0;
            }
        }
        else
        {
            int buttonState = event.getButtonState();
            if((buttonState & MotionEvent.BUTTON_PRIMARY) == MotionEvent.BUTTON_PRIMARY || (buttonState & MotionEvent.BUTTON_STYLUS_PRIMARY) == MotionEvent.BUTTON_STYLUS_PRIMARY)
                return Q3EKeyCodes.KeyCodes.K_MOUSE1;
            else if((buttonState & MotionEvent.BUTTON_SECONDARY) == MotionEvent.BUTTON_SECONDARY || (buttonState & MotionEvent.BUTTON_STYLUS_SECONDARY) == MotionEvent.BUTTON_STYLUS_SECONDARY)
                return Q3EKeyCodes.KeyCodes.K_MOUSE2;
            else if((buttonState & MotionEvent.BUTTON_TERTIARY) == MotionEvent.BUTTON_TERTIARY)
                return Q3EKeyCodes.KeyCodes.K_MOUSE3;
            else if((buttonState & MotionEvent.BUTTON_BACK) == MotionEvent.BUTTON_BACK)
                return Q3EKeyCodes.KeyCodes.K_MOUSE4;
            else if((buttonState & MotionEvent.BUTTON_FORWARD) == MotionEvent.BUTTON_FORWARD)
                return Q3EKeyCodes.KeyCodes.K_MOUSE5;
            else
                return 0;
        }
    }

    public void GrabMouse()
    {
        if(!m_usingMouse)
            return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            Runnable runnable = new Runnable() {
                @Override
                public void run()
                {
                    if (m_allowGrabMouse)
                        requestPointerCapture();
                    else
                        m_requestGrabMouse = 1;
                }
            };
            post(runnable);
            //runnable.run();
        }
    }

    public void UnGrabMouse()
    {
        if(!m_usingMouse)
            return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            Runnable runnable = new Runnable() {
                @Override
                public void run()
                {
                    if (m_allowGrabMouse)
                        releasePointerCapture();
                    else
                        m_requestGrabMouse = -1;
                }
            };
            post(runnable);
            //runnable.run();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus)
    {
        super.onWindowFocusChanged(hasWindowFocus);

        if(!m_usingMouse)
            return;
        m_allowGrabMouse = hasWindowFocus;
        if(hasWindowFocus)
        {
            if(m_requestGrabMouse > 0)
            {
                GrabMouse();
                m_requestGrabMouse = 0;
            }
            else if(m_requestGrabMouse < 0)
            {
                UnGrabMouse();
                m_requestGrabMouse = 0;
            }
        }
    }

    private boolean HandleCapturedPointerEvent(MotionEvent event, boolean absolute)
    {
        int action = event.getAction();
        int actionIndex = event.getActionIndex();
        switch (action)
        {
            case MotionEvent.ACTION_BUTTON_PRESS: {
                int gameMouseButton = ConvMouseButton(event);
                if(gameMouseButton != 0)
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, gameMouseButton, 0);
                }
                m_lastTouchPadPosX = event.getAxisValue(MotionEvent.AXIS_X, actionIndex);
                m_lastTouchPadPosY = event.getAxisValue(MotionEvent.AXIS_Y, actionIndex);
            }
            break;
            case MotionEvent.ACTION_BUTTON_RELEASE: {
                int gameMouseButton = ConvMouseButton(event);
                if(gameMouseButton != 0)
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, gameMouseButton, 0);
                }
                m_lastTouchPadPosX = -1;
                m_lastTouchPadPosY = -1;
            }
            break;
            case MotionEvent.ACTION_MOVE:
                float deltaX;
                float deltaY;
                if(absolute)
                {
                    deltaX = event.getX(actionIndex);
                    deltaY = event.getY(actionIndex);
                }
                else
                {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
                    {
                        deltaX = event.getAxisValue(MotionEvent.AXIS_RELATIVE_X, actionIndex);
                        deltaY = event.getAxisValue(MotionEvent.AXIS_RELATIVE_Y, actionIndex);
                    }
                    else
                    {
                        float x = event.getAxisValue(MotionEvent.AXIS_X, actionIndex);
                        float y = event.getAxisValue(MotionEvent.AXIS_Y, actionIndex);
                        deltaX = x - m_lastTouchPadPosX;
                        deltaY = y - m_lastTouchPadPosY;
                        m_lastTouchPadPosX = x;
                        m_lastTouchPadPosY = y;
                    }
                }
                Q3EUtils.q3ei.callbackObj.sendMotionEvent(deltaX, deltaY);
                Q3EUtils.q3ei.callbackObj.sendMouseEvent(deltaX, deltaY);
                break;
            case MotionEvent.ACTION_SCROLL:
                // float scrollX = event.getAxisValue(MotionEvent.AXIS_HSCROLL);
                float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, actionIndex);
                if(scrollY > 0)
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                }
                else if(scrollY < 0)
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                }
                break;
        }
        return true;
    }

    public boolean IsUsingMouse()
    {
        return m_usingMouse || m_usingMouseDevice;
    }

    public void ShowCursor(boolean on)
    {
        if(!m_usingMouse)
            return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            if(on)
                setPointerIcon(null);
            else
                setPointerIcon(PointerIcon.getSystemIcon(getContext(), PointerIcon.TYPE_NULL));
        }
    }

    private boolean IsGamePadDevice(int deviceId) {
        InputDevice device = InputDevice.getDevice(deviceId);
        if ((device == null) || (deviceId < 0)) {
            return false;
        }
        int sources = device.getSources();

        return ((sources & InputDevice.SOURCE_CLASS_JOYSTICK) != 0 ||
                ((sources & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) ||
                ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
        );
    }
}
