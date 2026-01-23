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

import android.view.KeyEvent;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.sdl.SDLActivity;

public final class Q3ERawControl extends Q3EEventControl
{
    private SDLActivity activity;

    public Q3ERawControl(Q3EControlView controlView)
    {
        super(controlView);
    }

    @Override
    public boolean OnKeyUp(int keyCode, KeyEvent event, int unicodeChar)
    {
        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, keyCode, unicodeChar);
        return true;
    }

    @Override
    public boolean OnKeyDown(int keyCode, KeyEvent event, int unicodeChar)
    {
        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, keyCode, unicodeChar);
        return true;
    }

}
