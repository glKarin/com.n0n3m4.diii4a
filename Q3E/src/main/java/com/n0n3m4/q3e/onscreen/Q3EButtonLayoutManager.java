package com.n0n3m4.q3e.onscreen;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.preference.PreferenceManager;
import android.view.Surface;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KLog;

public class Q3EButtonLayoutManager
{
    private final static boolean POSITION_USING_RELATIVE_MODE = true;
    private final Activity m_context;
    private       float    m_scale;
    private       boolean  m_friendly;
    private       int      m_opacity;
    private       boolean  m_landscape; // device is landscape
    private       boolean  m_portrait = false; // make portrait layout

    public Q3EButtonLayoutManager(Activity context)
    {
        this.m_context = context;
    }

    public Q3EButtonLayoutManager Scale(float scale)
    {
        if (scale <= 0.0f)
            scale = 1.0f;
        this.m_scale = scale;
        return this;
    }

    public Q3EButtonLayoutManager Friendly(boolean friendly)
    {
        this.m_friendly = friendly;
        return this;
    }

    public Q3EButtonLayoutManager Opacity(int opacity)
    {
        this.m_opacity = opacity;
        return this;
    }

    public Q3EButtonLayoutManager Landscape(boolean landscape)
    {
        this.m_landscape = landscape;
        return this;
    }

    public Q3EButtonLayoutManager Portrait(boolean portrait)
    {
        this.m_portrait = portrait;
        return this;
    }

    private int Dip2px_s(int i)
    {
        final boolean NeedScale = m_scale > 0.0f && m_scale != 1.0f;
        int r = Q3EUtils.dip2px(m_context, i);
        if(NeedScale)
            r = Math.round((float)r * m_scale);
        return r;
    }

    private int Dip2px(int i)
    {
        return Q3EUtils.dip2px(m_context, i);
    }

    private int S(int i)
    {
        return Math.round((float)i * m_scale);
    }

    public int[] GetGeometry()
    {
        KScreenInfo screenInfo = KScreenInfo.FromActivity(m_context);
        screenInfo = screenInfo.ToLandscape();
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        boolean hideNav = preferences.getBoolean(Q3EPreference.HIDE_NAVIGATION_BAR, true);
        boolean coverEdges = preferences.getBoolean(Q3EPreference.COVER_EDGES, true);
        int start = 0;
        int end = 0;
        int Width = screenInfo.FULL_WIDTH;
        int Height = screenInfo.FULL_HEIGHT;
        if(m_portrait)
        {
            if (!hideNav)
                end += screenInfo.NAV_BAR_HEIGHT;
            if (!coverEdges)
                end += screenInfo.EDGE_END_HEIGHT;
        }
        else
        {
            if (m_friendly)
            {
                end += screenInfo.NAV_BAR_HEIGHT;
                if (coverEdges)
                    start = screenInfo.EDGE_START_HEIGHT;
                else
                    end += screenInfo.EDGE_END_HEIGHT;
            }
            else
            {
                if (!hideNav)
                    end += screenInfo.NAV_BAR_HEIGHT;
                if (!coverEdges)
                    end += screenInfo.EDGE_END_HEIGHT;
            }
        }

        //System.err.println(screenInfo.toString());
        return m_portrait ? new int[] { Height, Width, start, end } : new int[] { Width, Height, start, end };
    }

    public String[] Make()
    {
        int[] geometry = GetGeometry();
        final int Width = geometry[0];
        final int Height = geometry[1];
        final int Start = geometry[2];
        final int End = geometry[3];

        if(m_portrait)
            return MakePortrait(Width, Height, Start, End);
        else
            return MakeLandscape(Width, Height, Start, End);
    }

    public String[] MakeLandscape(int Width, int Height, int Start, int End)
    {
        final int EndWidth = Width - End;

        final int LargeButton_Width = Dip2px_s(60); //k 64 // button width/height // 75
        final int MediumButton_Width = Dip2px_s(55); //k 60
        final int SmallButton_Width = Dip2px_s(50); //k 56
        // int rightoffset = 0; // LargeButton_Width * 3 / 4;
        final int Horizontal_Sliders_Width = Dip2px_s(120); // reload bar slider width
        final int Vertical_Sliders_Width = Dip2px_s(110); // save bar slider width
        final int Joystick_Radius = Dip2px_s(75); // half width
        final int Attack_Width = Dip2px_s(80); //k 100
        final int Panel_Radius = Dip2px_s(25); //k 24
        final int Crouch_Width = Dip2px_s(70); //k 80
        final int Horizontal_Space = Dip2px_s(5); //k 4
        final int Vertical_Space = Dip2px_s(5); //k 2
        final int Attack_right_Margin = Dip2px_s(40); //k 20
        final int Attack_Bottom_Margin = Dip2px_s(40); //k 20
        final int JoyStick_Left_Margin = Dip2px_s(35); //k 30
        final int JoyStick_Bottom_Margin = Dip2px_s(40); //k 30
        final int Alpha = m_opacity;

        Q3EButtonGeometry[] layouts = Q3EButtonGeometry.Alloc(Q3EGlobals.UI_SIZE);
        
        layouts[Q3EGlobals.UI_JOYSTICK].Set(Start + Joystick_Radius + JoyStick_Left_Margin, Height - Joystick_Radius - JoyStick_Bottom_Margin, Joystick_Radius, Alpha);
        layouts[Q3EGlobals.UI_SHOOT].Set(EndWidth - Attack_Width / 2 - Crouch_Width / 2 - Attack_right_Margin, Height - Attack_Width / 2 - Crouch_Width / 2 - Attack_Bottom_Margin, Attack_Width, Alpha);

        layouts[Q3EGlobals.UI_SAVE].Set(Start + Vertical_Sliders_Width / 2, Vertical_Sliders_Width / 2, Vertical_Sliders_Width, Alpha);
        layouts[Q3EGlobals.UI_RELOADBAR].Set(EndWidth - Horizontal_Sliders_Width / 2, Horizontal_Sliders_Width / 4, Horizontal_Sliders_Width, Alpha);

        layouts[Q3EGlobals.UI_KBD].Set(Start + Vertical_Sliders_Width + SmallButton_Width / 2 + Horizontal_Space, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_CONSOLE].Set(Start + Vertical_Sliders_Width / 2 + SmallButton_Width / 2 + Horizontal_Space, Vertical_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_JUMP].Set(EndWidth - LargeButton_Width / 2, layouts[Q3EGlobals.UI_SHOOT].y - LargeButton_Width / 2 - Attack_Width / 2 - Horizontal_Space, LargeButton_Width, Alpha);
        layouts[Q3EGlobals.UI_CROUCH].Set(EndWidth - Crouch_Width / 2, Height - Crouch_Width / 2, Crouch_Width, Alpha);

        layouts[Q3EGlobals.UI_PDA].Set(Start + Vertical_Sliders_Width + SmallButton_Width / 2 + SmallButton_Width + Horizontal_Space * 2, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_SCORE].Set(Start + Vertical_Sliders_Width + SmallButton_Width / 2 + SmallButton_Width * 2 + Horizontal_Space * 3, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        int bottomLineRight = layouts[Q3EGlobals.UI_SHOOT].x - Attack_right_Margin + Horizontal_Space * 2;
        layouts[Q3EGlobals.UI_ZOOM].Set(bottomLineRight - MediumButton_Width - Horizontal_Space, Height - MediumButton_Width / 2, MediumButton_Width, Alpha);
        layouts[Q3EGlobals.UI_FLASHLIGHT].Set(bottomLineRight - MediumButton_Width * 2 - Horizontal_Space * 2, Height - MediumButton_Width / 2, MediumButton_Width, Alpha);
        layouts[Q3EGlobals.UI_RUN].Set(bottomLineRight - MediumButton_Width * 3 - Horizontal_Space * 3, Height - MediumButton_Width / 2, MediumButton_Width, Alpha);

        layouts[Q3EGlobals.UI_INTERACT].Set(EndWidth - Horizontal_Sliders_Width - SmallButton_Width / 2 - Horizontal_Space, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_1].Set(EndWidth - SmallButton_Width / 2 - SmallButton_Width * 2 - Horizontal_Space * 2, Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_2].Set(EndWidth - SmallButton_Width / 2 - SmallButton_Width - Horizontal_Space, Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_3].Set(EndWidth - SmallButton_Width / 2, Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_WEAPON_PANEL].Set(EndWidth - Math.max(Horizontal_Sliders_Width + MediumButton_Width / 2 + Horizontal_Space, SmallButton_Width * 3 + Horizontal_Space * 2) - Panel_Radius * 5 / 2 - Horizontal_Space * 4, Panel_Radius + Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, Panel_Radius, Alpha);

        layouts[Q3EGlobals.UI_MINUS].Set(Start + SmallButton_Width / 2, Height - Joystick_Radius * 2 - JoyStick_Bottom_Margin - LargeButton_Width, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_PLUS].Set(Start + JoyStick_Left_Margin + Joystick_Radius + (JoyStick_Left_Margin + Joystick_Radius - SmallButton_Width / 2), Height - Joystick_Radius * 2 - JoyStick_Bottom_Margin - LargeButton_Width, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_Y].Set(bottomLineRight - MediumButton_Width * 2 - MediumButton_Width / 2 - Horizontal_Space * 2 - (MediumButton_Width / 2 - SmallButton_Width / 2), Height - MediumButton_Width - SmallButton_Width / 2, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_N].Set(bottomLineRight - MediumButton_Width * 2 + MediumButton_Width / 2 - Horizontal_Space * 2 + (MediumButton_Width / 2 - SmallButton_Width / 2), Height - MediumButton_Width - SmallButton_Width / 2, SmallButton_Width, Alpha);

        // hide default
        int extraX = 0;
        for (int i = Q3EGlobals.UI_0; i <= Q3EGlobals.UI_9; i++)
            layouts[i].Set(extraX = SmallButton_Width / 2 + SmallButton_Width * (i - Q3EGlobals.UI_0), Height + SmallButton_Width / 2, SmallButton_Width, Alpha);

        extraX += SmallButton_Width;
        layouts[Q3EGlobals.UI_NUM_PANEL].Set(extraX, Height + Panel_Radius, Panel_Radius, Alpha);
        extraX += Panel_Radius * 2;
/*        for (int i = Q3EGlobals.UI_Y; i <= Q3EGlobals.UI_N; i++)
            layouts[i].Set(extraX + SmallButton_Width * (i - Q3EGlobals.UI_Y), Height + SmallButton_Width / 2, SmallButton_Width, Alpha);*/

        return Q3EButtonGeometry.ToStrings(layouts, Width, Height);
    }

    public String[] MakePortrait(int Width, int Height, int Start, int End)
    {
        final int EndHeight = Height - End;

        final int LargeButton_Width = Dip2px_s(60); //k 64 // button width/height // 75
        final int MediumButton_Width = Dip2px_s(55); //k 60
        final int SmallButton_Width = Dip2px_s(50); //k 56
        // int rightoffset = 0; // LargeButton_Width * 3 / 4;
        final int Horizontal_Sliders_Width = Dip2px_s(120); // reload bar slider width
        final int Vertical_Sliders_Width = Dip2px_s(110); // save bar slider width
        final int Joystick_Radius = Dip2px_s(75); // half width
        final int Attack_Width = Dip2px_s(80); //k 100
        final int Panel_Radius = Dip2px_s(25); //k 24
        final int Crouch_Width = Dip2px_s(70); //k 80
        final int Horizontal_Space = Dip2px_s(5); //k 4
        final int Vertical_Space = Dip2px_s(5); //k 2
        final int Attack_right_Margin = Dip2px_s(40); //k 20
        final int Attack_Bottom_Margin = Dip2px_s(140); //k 20
        final int JoyStick_Left_Margin = Dip2px_s(35); //k 30
        final int JoyStick_Bottom_Margin = Dip2px_s(140); //k 30
        final int Alpha = m_opacity;

        Q3EButtonGeometry[] layouts = Q3EButtonGeometry.Alloc(Q3EGlobals.UI_SIZE);

        layouts[Q3EGlobals.UI_JOYSTICK].Set(Joystick_Radius + JoyStick_Left_Margin, EndHeight - Joystick_Radius - JoyStick_Bottom_Margin, Joystick_Radius, Alpha);
        layouts[Q3EGlobals.UI_SHOOT].Set(Width - Attack_Width / 2 - Crouch_Width / 2 - Attack_right_Margin, EndHeight - Attack_Width / 2 - Crouch_Width / 2 - Attack_Bottom_Margin, Attack_Width, Alpha);

        layouts[Q3EGlobals.UI_SAVE].Set(Vertical_Sliders_Width / 2, Start + Vertical_Sliders_Width / 2, Vertical_Sliders_Width, Alpha);
        layouts[Q3EGlobals.UI_RELOADBAR].Set(Width - Horizontal_Sliders_Width / 2, Start + Horizontal_Sliders_Width / 4, Horizontal_Sliders_Width, Alpha);

        layouts[Q3EGlobals.UI_KBD].Set(Vertical_Sliders_Width + SmallButton_Width / 2 + Horizontal_Space, Start + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_CONSOLE].Set(Vertical_Sliders_Width / 2 + SmallButton_Width / 2 + Horizontal_Space, Start + Vertical_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_JUMP].Set(Width - LargeButton_Width / 2, layouts[Q3EGlobals.UI_SHOOT].y - LargeButton_Width - Attack_Width / 2 - Horizontal_Space, LargeButton_Width, Alpha);
        layouts[Q3EGlobals.UI_CROUCH].Set(Width - Crouch_Width / 2, EndHeight - Crouch_Width / 2, Crouch_Width, Alpha);

        layouts[Q3EGlobals.UI_PDA].Set(SmallButton_Width / 2 + Horizontal_Space, Start + Vertical_Sliders_Width + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_SCORE].Set(SmallButton_Width / 2 + SmallButton_Width + Horizontal_Space * 2, Start + Vertical_Sliders_Width + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        int bottomLineRight = Width - Crouch_Width;
        layouts[Q3EGlobals.UI_ZOOM].Set(bottomLineRight - MediumButton_Width / 2 - Horizontal_Space, EndHeight - MediumButton_Width / 2, MediumButton_Width, Alpha);
        layouts[Q3EGlobals.UI_FLASHLIGHT].Set(bottomLineRight - MediumButton_Width / 2 - MediumButton_Width - Horizontal_Space * 2, EndHeight - MediumButton_Width / 2, MediumButton_Width, Alpha);
        layouts[Q3EGlobals.UI_RUN].Set(bottomLineRight - MediumButton_Width / 2 - MediumButton_Width * 2 - Horizontal_Space * 3, EndHeight - MediumButton_Width / 2, MediumButton_Width, Alpha);

        layouts[Q3EGlobals.UI_INTERACT].Set(Width - Horizontal_Sliders_Width - SmallButton_Width / 2 - Horizontal_Space, Start + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_1].Set(Width - SmallButton_Width / 2 - SmallButton_Width * 2 - Horizontal_Space * 2, Start + Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_2].Set(Width - SmallButton_Width / 2 - SmallButton_Width - Horizontal_Space, Start + Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_3].Set(Width - SmallButton_Width / 2, Start + Horizontal_Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_WEAPON_PANEL].Set(Width - Horizontal_Sliders_Width + Panel_Radius / 2, Start + Horizontal_Sliders_Width / 2 + SmallButton_Width * 2 + Panel_Radius + Vertical_Space * 2, Panel_Radius, Alpha);

        layouts[Q3EGlobals.UI_MINUS].Set(SmallButton_Width / 2, EndHeight - Joystick_Radius * 2 - JoyStick_Bottom_Margin - SmallButton_Width * 2, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_PLUS].Set(JoyStick_Left_Margin + Joystick_Radius + (JoyStick_Left_Margin + Joystick_Radius - SmallButton_Width / 2), EndHeight - Joystick_Radius * 2 - JoyStick_Bottom_Margin - SmallButton_Width * 2, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_Y].Set(bottomLineRight - MediumButton_Width * 2 - Horizontal_Space * 2 - (MediumButton_Width / 2 - SmallButton_Width / 2), EndHeight - MediumButton_Width - SmallButton_Width / 2, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_N].Set(bottomLineRight - MediumButton_Width - Horizontal_Space * 2 + (MediumButton_Width / 2 - SmallButton_Width / 2), EndHeight - MediumButton_Width - SmallButton_Width / 2, SmallButton_Width, Alpha);

        // hide default
        int extraX = 0;
        for (int i = Q3EGlobals.UI_0; i <= Q3EGlobals.UI_9; i++)
            layouts[i].Set(extraX = SmallButton_Width / 2 + SmallButton_Width * (i - Q3EGlobals.UI_0), Height + SmallButton_Width / 2, SmallButton_Width, Alpha);

        extraX += SmallButton_Width;
        layouts[Q3EGlobals.UI_NUM_PANEL].Set(extraX, Height + Panel_Radius, Panel_Radius, Alpha);
        extraX += Panel_Radius * 2;
/*        for (int i = Q3EGlobals.UI_Y; i <= Q3EGlobals.UI_N; i++)
            layouts[i].Set(extraX + SmallButton_Width * (i - Q3EGlobals.UI_Y), EndHeight + SmallButton_Width / 2, SmallButton_Width, Alpha);*/

        return Q3EButtonGeometry.ToStrings(layouts, Width, Height);
    }

    public static String[] GetDefaultLayout(Activity context, boolean friendly, float scale, int opacity, boolean landscape, boolean portrait)
    {
        Q3EButtonLayoutManager manager = new Q3EButtonLayoutManager(context);
        return manager.Scale(scale)
                .Friendly(friendly)
                .Opacity(opacity)
                .Landscape(landscape)
                .Portrait(portrait)
                .Make();
    }

    /*
    Relative position: > half and <= max: negative; > max: positive; <= half: positive
     */
    public static Point ToRelPosition(int x, int y, int maxw, int maxh)
    {
        if(!POSITION_USING_RELATIVE_MODE)
        {
            return new Point(x, y);
        }

        int centerX = maxw / 2;
        int centerY = maxh / 2;

        int resX;
        if(x <= centerX || x > maxw)
            resX = x;
        else
            resX = x - maxw;

        int resY;
        if(y <= centerY || y > maxh)
            resY = y;
        else
            resY = y - maxh;

        return new Point(resX, resY);
    }

    public static Point ToAbsPosition(int x, int y, int maxw, int maxh)
    {
        if(!POSITION_USING_RELATIVE_MODE)
        {
            return new Point(x, y);
        }

        int resX;
        if(x < 0)
            resX = maxw + x;
        else
            resX = x;

        int resY;
        if(y < 0)
            resY = maxh + y;
        else
            resY = y;

        //System.err.println(String.format("RRR %d %d | %d %d -> %d %d", x, y, maxw, maxh, resX, resY));
        return new Point(resX, resY);
    }
}
