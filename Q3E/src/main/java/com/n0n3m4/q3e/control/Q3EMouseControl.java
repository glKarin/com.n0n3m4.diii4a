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

package com.n0n3m4.q3e.control;

import android.content.Context;
import android.os.Build;
import android.view.InputDevice;
import android.view.MotionEvent;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.device.Q3EMouseDevice;
import com.n0n3m4.q3e.onscreen.Finger;
import com.n0n3m4.q3e.onscreen.TouchListener;

import java.util.Arrays;

public final class Q3EMouseControl
{
    private final Q3EControlView controlView;

    private boolean m_allowGrabMouse = false;
    private int m_requestGrabMouse = 0;

    private float m_lastTouchPadPosX = -1;
    private float m_lastTouchPadPosY = -1;
    private float m_lastMousePosX = -1;
    private float m_lastMousePosY = -1;

    public Q3EMouseControl(Q3EControlView controlView)
    {
        this.controlView = controlView;
    }

    private Context getContext()
    {
        return controlView.getContext();
    }

    public void GrabMouse()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && Q3E.m_usingMouse)
        {
            Runnable runnable = new Runnable() {
                @Override
                public void run()
                {
                    if (m_allowGrabMouse)
                        controlView.requestPointerCapture();
                    else
                        m_requestGrabMouse = 1;
                }
            };
            controlView.post(runnable);
            //runnable.run();
        }
    }

    public void UnGrabMouse()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && Q3E.m_usingMouse)
        {
            Runnable runnable = new Runnable() {
                @Override
                public void run()
                {
                    if (m_allowGrabMouse)
                        controlView.releasePointerCapture();
                    else
                        m_requestGrabMouse = -1;
                }
            };
            controlView.post(runnable);
            //runnable.run();
        }
    }

    public void OnWindowFocusChanged(boolean hasWindowFocus)
    {
        if(!Q3E.m_usingMouse)
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

    public boolean OnCapturedPointerEvent(MotionEvent event)
    {
        if(Q3E.m_usingMouse)
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
                    Q3E.sendKeyEvent(true, gameMouseButton, 0);
                }
                m_lastTouchPadPosX = event.getAxisValue(MotionEvent.AXIS_X, actionIndex);
                m_lastTouchPadPosY = event.getAxisValue(MotionEvent.AXIS_Y, actionIndex);
            }
            break;
            case MotionEvent.ACTION_BUTTON_RELEASE: {
                int gameMouseButton = ConvMouseButton(event);
                if(gameMouseButton != 0)
                {
                    Q3E.sendKeyEvent(false, gameMouseButton, 0);
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
                Q3E.sendMotionEvent(deltaX, deltaY);
                break;
            case MotionEvent.ACTION_SCROLL:
                // float scrollX = event.getAxisValue(MotionEvent.AXIS_HSCROLL);
                float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, actionIndex);
                if(scrollY > 0)
                {
                    Q3E.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                    Q3E.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                }
                else if(scrollY < 0)
                {
                    Q3E.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                    Q3E.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                }
                break;
        }
        return true;
    }

    public boolean OnGenericMotionEvent(MotionEvent event)
    {
        int source = event.getSource();

        if(Q3E.m_usingMouse && source == InputDevice.SOURCE_MOUSE)
        {
            int action = event.getAction();
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
                        Q3E.sendKeyEvent(true, gameMouseButton, 0);
                    }
                }
                break;
                case MotionEvent.ACTION_BUTTON_RELEASE: {
                    int gameMouseButton = ConvMouseButton(event);
                    if(gameMouseButton != 0)
                    {
                        Q3E.sendKeyEvent(false, gameMouseButton, 0);
                    }
                    m_lastMousePosX = -1;
                    m_lastMousePosY = -1;
                }
                break;
//                case MotionEvent.ACTION_HOVER_ENTER: break;
//                case MotionEvent.ACTION_HOVER_EXIT: break;
                case MotionEvent.ACTION_HOVER_MOVE:
                    Q3E.sendMotionEvent(deltaX, deltaY);
                    Q3E.sendMouseEvent(x, y);
                    break;
                case MotionEvent.ACTION_SCROLL:
                    float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, actionIndex);
                    if(scrollY > 0)
                    {
                        Q3E.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                        Q3E.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELUP, 0);
                    }
                    else if(scrollY < 0)
                    {
                        Q3E.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                        Q3E.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MWHEELDOWN, 0);
                    }
                    break;
            }
            return true;
        }

        return false;
    }

    public boolean OnTouchEvent(MotionEvent event)
    {
        if(Q3E.m_usingMouse/* && source == InputDevice.SOURCE_MOUSE*/) {
            int action = event.getAction();
            int actionIndex = event.getActionIndex();
            float x = event.getX(actionIndex);
            float y = event.getY(actionIndex);
            if (m_lastMousePosX < 0)
                m_lastMousePosX = x;
            if (m_lastMousePosY < 0)
                m_lastMousePosY = y;
            float deltaX = x - m_lastMousePosX;
            float deltaY = y - m_lastMousePosY;
            m_lastMousePosX = x;
            m_lastMousePosY = y;
            if (action == MotionEvent.ACTION_MOVE) {
                Q3E.sendMotionEvent(deltaX, deltaY);
                Q3E.sendMouseEvent(x, y);
            }
            return true;
        }
        return false;
    }

    public void Start()
    {
    }
}
