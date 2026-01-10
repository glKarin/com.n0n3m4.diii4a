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
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorManager;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

final class Q3EGyroscopeControl
{
    private final Q3EControlView controlView;

    /// gyroscope function
    private boolean m_gyroInited = false;
    private SensorManager m_sensorManager = null;
    private Sensor m_gyroSensor = null;
    private boolean m_enableGyro = false;
    float m_xAxisGyroSens = Q3EGlobals.GYROSCOPE_X_AXIS_SENS;
    float m_yAxisGyroSens = Q3EGlobals.GYROSCOPE_Y_AXIS_SENS;
    private Display m_display;

    Q3EGyroscopeControl(Q3EControlView controlView)
    {
        this.controlView = controlView;
    }

    private Context getContext()
    {
        return controlView.getContext();
    }

    void EnableGyroscopeControl(boolean b)
    {
        m_enableGyro = b;
    }

    void SetGyroscopeSens(float x, float y)
    {
        m_xAxisGyroSens = x;
        m_yAxisGyroSens = y;
    }

    private boolean InitGyroscopeSensor()
    {
        if (m_gyroInited)
            return null != m_gyroSensor;
        m_gyroInited = true;
        m_sensorManager = (SensorManager) controlView.getContext().getSystemService(Context.SENSOR_SERVICE);
        if (null == m_sensorManager)
            return false;
        m_gyroSensor = m_sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        m_display = ((WindowManager)controlView.getContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        return null != m_gyroSensor;
    }

    void StartGyroscope()
    {
        InitGyroscopeSensor();
        if (null != m_gyroSensor)
            m_sensorManager.registerListener(controlView, m_gyroSensor, SensorManager.SENSOR_DELAY_GAME);
    }

    void StopGyroscope()
    {
        if (null != m_gyroSensor)
            m_sensorManager.unregisterListener(controlView, m_gyroSensor);
    }

    void OnSensorChanged(SensorEvent event)
    {
        if (m_enableGyro && Q3EUtils.q3ei.callbackObj.notinmenu && !Q3EUtils.q3ei.callbackObj.inLoading)
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

    void OnAccuracyChanged(Sensor sensor, int accuracy) {}

    void Pause()
    {
        if (m_enableGyro)
            StopGyroscope();
    }

    void Resume()
    {
        if (m_enableGyro)
            StartGyroscope();
    }
}
