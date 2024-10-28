package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.Arrays;

public class MouseButton implements TouchListener
{
    private final Q3EControlView view;

    public MouseButton(Q3EControlView vw)
    {
        view = vw;
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        if (act == 1)
        {
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MOUSE1, 0);//Can be sent twice, unsafe.
        }
        else if (act == -1)
        {
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MOUSE1, 0);//Can be sent twice, unsafe.
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return x <= Q3EControlView.orig_width / 2;
        // return true;
    }
}
