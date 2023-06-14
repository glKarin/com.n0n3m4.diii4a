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
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.preference.PreferenceManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

import com.n0n3m4.q3e.device.Q3EMouseDevice;
import com.n0n3m4.q3e.gl.GL;
import com.n0n3m4.q3e.gl.Q3EConfigChooser;
import com.n0n3m4.q3e.karin.KKeyToolBar;
import com.n0n3m4.q3e.karin.KOnceRunnable;
import com.n0n3m4.q3e.onscreen.Button;
import com.n0n3m4.q3e.onscreen.Disc;
import com.n0n3m4.q3e.onscreen.Finger;
import com.n0n3m4.q3e.onscreen.Joystick;
import com.n0n3m4.q3e.onscreen.MouseControl;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.Slider;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiLoader;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

//import tv.ouya.console.api.OuyaController;

public class Q3EControlView extends GLSurfaceView implements GLSurfaceView.Renderer, SensorEventListener
{

    public boolean usesCSAA = false;
    private boolean m_toolbarActive = true;
    private View m_keyToolbar = null;
    private boolean m_usingMouse = false;
    private float m_lastMousePosX = -1;
    private float m_lastMousePosY = -1;
    private boolean m_usingMouseDevice = false;
    private Q3EMouseDevice m_mouseDevice = null;
    private float m_lastTouchPadPosX = -1;
    private float m_lastTouchPadPosY = -1;
    private int m_requestGrabMouse = 0;
    private boolean m_allowGrabMouse = false;

    public Q3EControlView(Context context)
    {
        super(context);

        /*try
        {
            if (Q3EUtils.isOuya)
                OuyaController.init(context);
        } catch (Exception e)
        {
            e.printStackTrace();
        }*/

        setEGLConfigChooser(new Q3EConfigChooser(8, 8, 8, 8, 0, GL.usegles20));
        getHolder().setFormat(PixelFormat.RGBA_8888);

        if (GL.usegles20)
            setEGLContextClientVersion(2);

        setRenderer(this);

        setFocusable(true);
        setFocusableInTouchMode(true);

        boolean usingMouse = PreferenceManager.getDefaultSharedPreferences(context).getBoolean(Q3EPreference.pref_harm_using_mouse, false);
        if(usingMouse)
        {
            int mouse = Q3EUtils.SupportMouse();
            if(mouse == Q3EGlobals.MOUSE_DEVICE)
                m_usingMouseDevice = true;
            else if(mouse == Q3EGlobals.MOUSE_EVENT)
                m_usingMouse = true;
        }
    }

    public static boolean mInit = false;
    private IntBuffer tmpbuf;

    public static int orig_width;
    public static int orig_height;

    public boolean mapvol = false;
    public boolean analog = false;


    //RTCW4A-specific
    Button actbutton;
    Button kickbutton;

    //MOUSE

    private boolean hideonscr;

    long oldtime = 0;
    long delta = 0;

    @Override
    public void onDrawFrame(GL10 gl)
    {
        long t = System.currentTimeMillis();
        delta = t - oldtime;
        oldtime = t;
        if (delta > 1000)
            delta = 1000;

        if ((last_joystick_x != 0) || (last_joystick_y != 0))
            sendMotionEvent(delta * last_joystick_x, delta * last_joystick_y);

        if (usesCSAA)
        {
            if (!GL.usegles20)
                gl.glClear(0x8000); //Yeah, I know, it doesn't work in 1.1
            else
                GLES20.glClear(0x8000);
        }

        if (Q3EUtils.q3ei.isD3BFG)
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_STENCIL_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        //k: not render in game loading
        if (Q3EUtils.q3ei.callbackObj.inLoading)
            return;

        //Onscreen buttons:
        //save state

        if (!GL.usegles20)
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
            int a = tmpbuf.get(0);
            ((GL11) gl).glGetTexEnviv(GL11.GL_TEXTURE_ENV, GL11.GL_TEXTURE_ENV_MODE, tmpbuf);
            int b = tmpbuf.get(0);
            boolean disabletex = !((GL11) gl).glIsEnabled(gl.GL_TEXTURE_COORD_ARRAY);
            boolean enableclr = ((GL11) gl).glIsEnabled(gl.GL_COLOR_ARRAY);

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

            ((GL11) gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);

            gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

            gl.glColor4f(1, 1, 1, 0.3f);

            for (Paintable p : paint_elements)
                p.Paint((GL11) gl);

            gl.glColor4f(1, 1, 1, 1);
            gl.glEnable(gl.GL_CULL_FACE);
            gl.glEnable(gl.GL_DEPTH_TEST);
            //restore
            if (disabletex)
                gl.glDisableClientState(gl.GL_TEXTURE_COORD_ARRAY);
            if (enableclr)
                gl.glEnableClientState(gl.GL_COLOR_ARRAY);
            ((GL11) gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, b);
            gl.glBindTexture(GL10.GL_TEXTURE_2D, a);
        } else
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

            for (Paintable p : paint_elements)
                p.Paint((GL11) gl);

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
        if (!mInit)
        {

            tmpbuf = ByteBuffer.allocateDirect(4).order(ByteOrder.nativeOrder()).asIntBuffer();

            SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this.getContext());

            hideonscr = mPrefs.getBoolean(Q3EPreference.pref_hideonscr, false);
            mapvol = mPrefs.getBoolean(Q3EPreference.pref_mapvol, false);
            m_mapBack = mPrefs.getInt(Q3EPreference.pref_harm_mapBack, ENUM_BACK_ALL); //k
            analog = mPrefs.getBoolean(Q3EPreference.pref_analog, true);

            if(m_usingMouseDevice)
                m_mouseDevice = new Q3EMouseDevice(this);

            orig_width = w;
            orig_height = h;

            UiLoader uildr = new UiLoader(this, gl, orig_width, orig_height);

            for (int i = 0; i < Q3EUtils.q3ei.UI_SIZE; i++)
            {
                Object o = uildr.LoadElement(i, false);
                touch_elements.add((TouchListener) o);
                paint_elements.add((Paintable) o);
            }

            if (Q3EUtils.q3ei.isRTCW)
            {
                actbutton = (Button) touch_elements.get(Q3EUtils.q3ei.RTCW4A_UI_ACTION);
                kickbutton = (Button) touch_elements.get(Q3EUtils.q3ei.RTCW4A_UI_KICK);
            } else
            {
                actbutton = null;
                kickbutton = null;
            }

            if (hideonscr)
            {
                touch_elements.clear();
            }
            //must be last
            touch_elements.add(new MouseControl(this, false));
            touch_elements.add(new MouseControl(this, mPrefs.getBoolean(Q3EPreference.pref_2fingerlmb, false)));
            touch_elements.add(new MouseControl(this, false));

            SortOnScreenButtons(); //k sort priority

            if (hideonscr)
            {
                paint_elements.clear();
            }
            for (Paintable p : paint_elements) p.loadtex(gl);

            for (int i = 0; i < fingers.length; i++)
                fingers[i] = new Finger(null, i);

            if(null != m_mouseDevice)
                m_mouseDevice.Start();

            mInit = true;
            post(new Runnable()
            {
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

        if (GL.usegles20)
        {
            GL.initGL20();
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        }

        if (mInit)
        {
            for (Paintable p : paint_elements) p.loadtex(gl);
        }

    }

    public void setState(int st)
    {
        if (actbutton != null)
            actbutton.alpha = ((st & 1) == 1) ? Math.min(actbutton.initalpha * 2, 1f) : actbutton.initalpha;
        if (kickbutton != null)
            kickbutton.alpha = ((st & 4) == 4) ? Math.min(kickbutton.initalpha * 2, 1f) : kickbutton.initalpha;
    }

    public int getCharacter(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_DEL) return '\b';
        return event.getUnicodeChar();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if ((!mapvol) && ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) || (keyCode == KeyEvent.KEYCODE_VOLUME_UP)))
            return false;
        if (keyCode == KeyEvent.KEYCODE_BACK && (m_mapBack & ENUM_BACK_ESCAPE) == 0)
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
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event);
                break;
        }
        int t = getCharacter(keyCode, event);
        sendKeyEvent(true, qKeyCode, t);
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if ((!mapvol) && ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) || (keyCode == KeyEvent.KEYCODE_VOLUME_UP)))
            return false;
        if (keyCode == KeyEvent.KEYCODE_BACK)
        {
            if (m_mapBack == ENUM_BACK_NONE)
                return true;
            if ((m_mapBack & ENUM_BACK_EXIT) != 0 && HandleBackPress())
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
                qKeyCode = Q3EKeyCodes.convertKeyCode(keyCode, event);
                break;
        }
        sendKeyEvent(false, qKeyCode, getCharacter(keyCode, event));
        return true;
    }


    static float last_joystick_x = 0;
    static float last_joystick_y = 0;

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

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        int action = event.getAction();
        int source = event.getSource();
        if (((source == InputDevice.SOURCE_JOYSTICK) || (source == InputDevice.SOURCE_GAMEPAD)) && (action == MotionEvent.ACTION_MOVE))
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
                    if(gameMouseButton >= 0)
                    {
                        sendKeyEvent(true, gameMouseButton, 0);
                    }
                }
                    break;
                case MotionEvent.ACTION_BUTTON_RELEASE: {
                    int gameMouseButton = ConvMouseButton(event);
                    if(gameMouseButton >= 0)
                    {
                        sendKeyEvent(false, gameMouseButton, 0);
                    }
                    m_lastMousePosX = -1;
                    m_lastMousePosY = -1;
                }
                    break;
//                case MotionEvent.ACTION_HOVER_ENTER: break;
//                case MotionEvent.ACTION_HOVER_EXIT: break;
                case MotionEvent.ACTION_HOVER_MOVE:
                    sendMotionEvent(deltaX, deltaY);
                    break;
                case MotionEvent.ACTION_SCROLL:
                    float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, actionIndex);
                    if(scrollY > 0)
                    {
                        sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                        sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                    }
                    else if(scrollY < 0)
                    {

                        sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                        sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
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

    public static Finger[] fingers = new Finger[10];
    public static ArrayList<TouchListener> touch_elements = new ArrayList<TouchListener>(0);
    public static ArrayList<Paintable> paint_elements = new ArrayList<Paintable>(0);
    public static TouchListener[] handle_elements = new TouchListener[10]; // handled elements in every touch event

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

        int actionMasked = event.getActionMasked();
        int actionIndex = event.getActionIndex();
        int pid = event.getPointerId(actionIndex);
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
            int handled = 0;
            for (Finger f : fingers)
            {
                if (f.target != null)
                {
                    // check is handled: only once on a button
                    int i = 0;
                    while(i < handled)
                    {
                        if(null == handle_elements[i] || handle_elements[i] == f.target)
                            break;
                        i++;
                    }
                    if(i < handled)
                        continue;
                    handle_elements[handled] = f.target;
                    handled++;

                    if (!f.onTouchEvent(event))
                        f.target = null;
                }
            }
        }
        //k catch (Exception ignored)
        {
        }

        if ((actionMasked == MotionEvent.ACTION_UP) || (actionMasked == MotionEvent.ACTION_POINTER_UP) || (actionMasked == MotionEvent.ACTION_CANCEL))
        {
            fingers[pid].target = null;
        }

        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        sendTrackballEvent(event.getAction() == MotionEvent.ACTION_DOWN, event.getX(), event.getY());
        return true;
    }

    public void sendAnalog(final boolean down, final float x, final float y)
    {
        queueEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendAnalog(down ? 1 : 0, x, y);
            }
        });
    }

    public void sendKeyEvent(final boolean down, final int keycode, final int charcode)
    {
        queueEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendKeyEvent(down ? 1 : 0, keycode, charcode);
            }
        });
    }

    static float last_trackball_x = 0;
    static float last_trackball_y = 0;

    public void sendMotionEvent(final float deltax, final float deltay)
    {
        queueEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendMotionEvent(deltax, deltay);
            }
        });
    }

    private void sendTrackballEvent(final boolean down, final float x, final float y)
    {
        queueEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                if (down)
                {
                    last_trackball_x = x;
                    last_trackball_y = y;
                }
                Q3EJNI.sendMotionEvent(x - last_trackball_x, y - last_trackball_y);
                last_trackball_x = x;
                last_trackball_y = y;
            }
        });
    }

    public void queueEvent(Runnable r)
    {
        Q3EUtils.q3ei.callbackObj.PushEvent(r);
    }

    private int m_mapBack = ENUM_BACK_ALL;
    private long m_lastPressBackTime = -1;
    private int m_pressBackCount = 0;
    private static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL = 1000;
    private static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT = 3;
    private static final int ENUM_BACK_NONE = 0;
    private static final int ENUM_BACK_ESCAPE = 1;
    private static final int ENUM_BACK_EXIT = 2;
    private static final int ENUM_BACK_ALL = 0xFF;

    private boolean HandleBackPress()
    {
        if ((m_mapBack & ENUM_BACK_EXIT) == 0)
            return false;
        boolean res = false;
        long now = System.currentTimeMillis();
        if (m_lastPressBackTime > 0 && now - m_lastPressBackTime <= CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL)
        {
            m_pressBackCount++;
            res = true;
        } else
        {
            m_pressBackCount = 1;
            res = (m_mapBack & ENUM_BACK_ESCAPE) == 0;
        }
        m_lastPressBackTime = now;
        if (m_pressBackCount == CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT - 1)
            Toast.makeText(getContext(), "Click back again to exit......", Toast.LENGTH_LONG).show();
        else if (m_pressBackCount == CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT)
        {
            m_renderView.Shutdown();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
                ((Activity) getContext()).finishAffinity();
            else
                ((Activity) getContext()).finish();
            System.exit(0);
            return true;
        }
        return res;
    }

    private Q3EView m_renderView;

    public void RenderView(Q3EView view)
    {
        m_renderView = view;
    }

    private boolean m_gyroInited = false;
    private SensorManager m_sensorManager = null;
    private Sensor m_gyroSensor = null;
    private boolean m_enableGyro = false;
    public static final float GYROSCOPE_X_AXIS_SENS = 18;
    public static final float GYROSCOPE_Y_AXIS_SENS = 18;
    private float m_xAxisGyroSens = GYROSCOPE_X_AXIS_SENS;
    private float m_yAxisGyroSens = GYROSCOPE_Y_AXIS_SENS;

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
        if (Q3EUtils.q3ei.callbackObj.notinmenu && !Q3EUtils.q3ei.callbackObj.inLoading)
        {
            sendMotionEvent(-event.values[0] * m_xAxisGyroSens, event.values[1] * m_yAxisGyroSens);
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
        ToggleToolbar(false);
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

        m_keyToolbar = null;
        m_toolbarActive = true;
    }

    public View CreateToolbar()
    {
        if (Q3EUtils.q3ei.function_key_toolbar)
        {
            Context context = getContext();
            m_keyToolbar = new KKeyToolBar(context);
            m_keyToolbar.setVisibility(View.GONE);
            try
            {
                String str = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.pref_harm_function_key_toolbar_y, "0");
                if(null == str)
                    str = "0";
                int y = Integer.parseInt(str);
                if (y > 0)
                    m_keyToolbar.setY(y);
            } catch (Exception e)
            {
                e.printStackTrace();
            }
        }
        return m_keyToolbar;
    }

    public View Toolbar()
    {
        return m_keyToolbar;
    }

    public void ToggleToolbar()
    {
        ToggleToolbar(!m_toolbarActive);
    }

    public void ToggleToolbar(boolean b)
    {
        if (null != m_keyToolbar && Q3EUtils.q3ei.function_key_toolbar)
        {
            m_toolbarActive = b;
            if (m_toolbarActive)
                m_keyToolbar.setVisibility(View.VISIBLE);
            else
                m_keyToolbar.setVisibility(View.GONE);
        }
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
        return Q3EGlobals.TYPE_MOUSE;
    }

    private void SortOnScreenButtons()
    {
        TouchListener[] touchListeners = touch_elements.toArray(new TouchListener[0]);
        final List<Integer> Type_Priority = Arrays.asList(
                Q3EGlobals.TYPE_BUTTON,
                Q3EGlobals.TYPE_SLIDER,
                Q3EGlobals.TYPE_DISC,
                Q3EGlobals.TYPE_JOYSTICK,
                Q3EGlobals.TYPE_MOUSE
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
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M)
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
                default:
                    return -1;
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
            else
                return -1;
        }
    }

    public void GrabMouse()
    {
        if(!m_usingMouse)
            return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            Runnable runnable = new Runnable()
            {
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
            Runnable runnable = new Runnable()
            {
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
                if(gameMouseButton >= 0)
                {
                    sendKeyEvent(true, gameMouseButton, 0);
                }
                m_lastTouchPadPosX = event.getAxisValue(MotionEvent.AXIS_X, actionIndex);
                m_lastTouchPadPosY = event.getAxisValue(MotionEvent.AXIS_Y, actionIndex);
            }
            break;
            case MotionEvent.ACTION_BUTTON_RELEASE: {
                int gameMouseButton = ConvMouseButton(event);
                if(gameMouseButton >= 0)
                {
                    sendKeyEvent(false, gameMouseButton, 0);
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
                sendMotionEvent(deltaX, deltaY);
                break;
            case MotionEvent.ACTION_SCROLL:
                // float scrollX = event.getAxisValue(MotionEvent.AXIS_HSCROLL);
                float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, actionIndex);
                if(scrollY > 0)
                {
                    sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                    sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                }
                else if(scrollY < 0)
                {

                    sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                    sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                }
                break;
        }
        return true;
    }
}
