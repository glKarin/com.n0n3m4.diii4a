package com.n0n3m4.q3e.device;

import android.graphics.RectF;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EMain;

/**
 * Virtual mouse
 */
public class Q3EVirtualMouse
{
    private final Q3EMain activity;

    private       Q3EMouseCursor mouseCursor;
    private final RectF physicalRect = new RectF(0, 0, 640, 480);
    private boolean relativeMode = false;
    private float posX;
    private float posY;
    private float deltaX;
    private float deltaY;
    private float width = 640;
    private float height = 480;
    private boolean disableCursor = false;

    private final Runnable show = new Runnable() {
        @Override
        public void run() {
            InitCursor();
            mouseCursor.SetVisible(true);
        }
    };
    private final Runnable hide = new Runnable() {
        @Override
        public void run() {
            InitCursor();
            mouseCursor.SetVisible(false);
        }
    };
    private final Runnable updatePosition = new Runnable() {
        @Override
        public void run() {
            InitCursor();
            if(Q3E.IsOriginalSize())
                mouseCursor.SetPosition((int)(posX + physicalRect.left), (int)(posY + physicalRect.top));
            else
                mouseCursor.SetPosition((int)(Q3E.LogicalToPhysicsX((int)posX) + physicalRect.left), (int)(Q3E.LogicalToPhysicsY((int)posY) + physicalRect.top));
        }
    };

    public Q3EVirtualMouse(Q3EMain activity)
    {
        this.activity = activity;
    }

    public void SetPhysicalGeometry(float offX, float offY, float physicalWidth, float physicalHeight)
    {
        physicalRect.offsetTo(offX, offY);
        physicalRect.right = physicalRect.left + physicalWidth;
        physicalRect.bottom = physicalRect.top + physicalHeight;
    }

    public void SetLogicalSize(float w, float h)
    {
        width = w;
        height = h;
        deltaX = width / 2;
        deltaY = height / 2;
    }

    public void InitCursor()
    {
        if(null == mouseCursor)
        {
            RelativeLayout mainLayout = activity.GetMainLayout();
            mouseCursor = new Q3EMouseCursor(activity);
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(Q3EMouseCursor.WIDTH, Q3EMouseCursor.HEIGHT);
            mainLayout.addView(mouseCursor, params);
        }
    }

    public void SetCursorVisible(boolean visible)
    {
        if(disableCursor)
            return;
        activity.runOnUiThread(visible ? show : hide);
    }

    public void SetCursorPosition(float x, float y)
    {
        this.posX = x;
        this.posY = y;
        UpdateCursorPosition();
    }

    public void UpdateCursorPosition()
    {
        if(disableCursor)
            return;
        activity.runOnUiThread(updatePosition);
    }

    private void RelativeMotion(float dx, float dy)
    {
        float x = dx;
        float y = dy;

        float hw = width / 2;
        if(x < -hw) {
            x = -hw;
        } else if(x > hw) {
            x = hw;
        }

        float hh = height / 2;
        if(y < -hh) {
            y = -hh;
        } else if(y > hh) {
            y = hh;
        }

        deltaX = x;
        deltaY = y;
    }

    private void AbsolutionMotion(float dx, float dy)
    {
        float x = posX + dx;
        float y = posY + dy;

        if(x < 0) {
            x = 0;
        } else if(x >= width) {
            x = width - 1;
        }

        if(y < 0) {
            y = 0;
        } else if(y >= height) {
            y = height - 1;
        }

        posX = x;
        posY = y;

        UpdateCursorPosition();
    }

    public void Motion(float dx, float dy)
    {
        if(relativeMode)
        {
            RelativeMotion(dx, dy);
        }
        else
        {
            AbsolutionMotion(dx, dy);
        }
    }

    public void SetRelativeMode(boolean on)
    {
        relativeMode = on;
        if(on)
        {
            deltaX = width / 2;
            deltaY = height / 2;
            SetCursorVisible(false);
        }
        else
        {
            //posX = 0;
            //posY = 0;
            UpdateCursorPosition();
            SetCursorVisible(true);
        }
    }

    public void SetPosition(float x, float y)
    {
        if(relativeMode)
        {
            deltaX = x;
            deltaY = y;
        }
        else
        {
            posX = x;
            posY = y;
        }
    }

    public void SetAbsPosition(float x, float y)
    {
        if(x < physicalRect.left) {
            x = 0;
        } else if(x >= physicalRect.right) {
            x = physicalRect.width() - 1;
        } else {
            x = x - physicalRect.left;
        }

        if(y < physicalRect.top) {
            y = 0;
        } else if(y >= physicalRect.bottom) {
            y = physicalRect.height() - 1;
        } else {
            y = y - physicalRect.top;
        }
        SetPosition(Q3E.PhysicsToLogicalX(x), Q3E.PhysicsToLogicalY(y));
    }

    public boolean IsRelativeMode()
    {
        return relativeMode;
    }

    public float GetX()
    {
        return relativeMode ? deltaX : posX;
    }

    public float GetY()
    {
        return relativeMode ? deltaY : posY;
    }

    public void DisableCursor(boolean d)
    {
        disableCursor = d;
    }
}
