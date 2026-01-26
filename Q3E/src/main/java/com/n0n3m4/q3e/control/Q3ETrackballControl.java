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
import android.view.MotionEvent;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3EUtils;

public final class Q3ETrackballControl
{
    private final Q3EControlView controlView;

    private float last_trackball_x = 0;
    private float last_trackball_y = 0;

    public Q3ETrackballControl(Q3EControlView controlView)
    {
        this.controlView = controlView;
    }

    private Context getContext()
    {
        return controlView.getContext();
    }

    public boolean OnTrackballEvent(MotionEvent event)
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
        Q3E.sendMotionEvent(deltaX, deltaY);
        last_trackball_x = x;
        last_trackball_y = y;
        return true;
    }
}
