package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3EEditButtonHandler;
import com.n0n3m4.q3e.Q3EUiView;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public interface UiViewOverlay extends TouchListener
{
    public static final int ON_SCREEN_BUTTON_MIN_SIZE = 0;

    public void show(int x, int y, FingerUi fn);
    public void loadtex(GL10 gl);
    public boolean isInside(int x, int y);
    public void hide();
    public void Paint(GL11 gl);

    public boolean tgtresize(boolean dir);
    public boolean tgtalpha(boolean dir);

    public static boolean SetupAlpha(boolean dir, FingerUi fngr, Q3EEditButtonHandler view)
    {
        if(null == fngr || !(fngr.target instanceof Paintable))
            return false;

        Paintable target = (Paintable) fngr.target;
        target.alpha += dir ? 0.1 : -0.1;
        view.SetModified();
        if (target.alpha < 0.01)
        {
            target.alpha = 0.1f;
            return false;
        }
        if (target.alpha > 1)
        {
            target.alpha = 1.0f;
            return false;
        }
        return true;
    }

    public static boolean Resize(boolean dir, int step, FingerUi fngr, Q3EEditButtonHandler view)
    {
        if(null == fngr)
            return false;

        Object o = fngr.target;
        int st = dir ? step : -step;
        view.SetModified();
        if (o instanceof Button)
        {
            Button tmp = (Button) o;
            if (tmp.width <= 0)
                return false;
            if (tmp.width + st > step)
            {
                float aspect = Button.CalcAspect(tmp.Style());
                int width = tmp.width + st;
                int height = (int) (aspect * width + 0.5f);
                view.post(new Runnable() {
                    @Override
                    public void run() {
                        tmp.Resize(width, height);
                        view.ReloadButton(tmp);
                    }
                });
            }
            else if (tmp.width + st <= ON_SCREEN_BUTTON_MIN_SIZE)
                return false;
        }
        else if (o instanceof Slider)
        {
            Slider tmp = (Slider) o;
            if (tmp.width <= 0)
                return false;
            if (tmp.width + st > step)
            {
                float aspect = Slider.CalcAspect(tmp.Style());
                int width = tmp.width + st;
                int height = (int) (aspect * width + 0.5f);
                view.post(new Runnable() {
                    @Override
                    public void run() {
                        tmp.Resize(width, height);
                        view.ReloadButton(tmp);
                    }
                });
            }
            else if (tmp.width + st <= ON_SCREEN_BUTTON_MIN_SIZE)
                return false;
        }
        else if (o instanceof Joystick)
        {
            Joystick tmp = (Joystick) o;
            if (tmp.size <= 0)
                return false;
            if (tmp.size + st > step)
            {
                int size = tmp.size + st;
                view.post(new Runnable() {
                    @Override
                    public void run() {
                        tmp.Resize(size / 2);
                        view.ReloadButton(tmp);
                    }
                });
            }
            else if (tmp.size + st <= ON_SCREEN_BUTTON_MIN_SIZE)
                return false;
        }
        //k
        else if (o instanceof Disc)
        {
            Disc tmp = (Disc) o;
            if (tmp.size <= 0)
                return false;
            if (tmp.size + st > step)
            {
                int size = tmp.size + st;
                view.post(new Runnable() {
                    @Override
                    public void run() {
                        tmp.Resize(size / 2);
                        view.ReloadButton(tmp);
                    }
                });
            }
            else if (tmp.size + st <= ON_SCREEN_BUTTON_MIN_SIZE)
                return false;
        }
        // else return false;
        //view.RefreshTgt(fngr);
        return true;
    }
}
