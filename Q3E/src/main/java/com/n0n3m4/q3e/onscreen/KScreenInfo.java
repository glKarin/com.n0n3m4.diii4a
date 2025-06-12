package com.n0n3m4.q3e.onscreen;

import static com.n0n3m4.q3e.Q3E.activity;

import android.app.Activity;

import com.n0n3m4.q3e.Q3EUtils;

public class KScreenInfo
{
    public final boolean PORTRAIT; // 0 or 180
    public final boolean LANDSCAPE; // 90 or 270
    public final boolean INVERT; // 180 or 270
    public final boolean FULLSCREEN; // full size == display size
    public final int     FULL_WIDTH; // w > h if landscape
    public final int     FULL_HEIGHT; // w > h if landscape
    public final int     DISPLAY_WIDTH; // w > h if landscape
    public final int     DISPLAY_HEIGHT; // w > h if landscape
    public final int     NAV_BAR_HEIGHT;
    // swap if invert
    public final int     EDGE_START_HEIGHT; // always is 0 if not cover edge
    public final int     EDGE_END_HEIGHT; // always is 0 if not cover edge

    private KScreenInfo(Activity activity)
    {
        LANDSCAPE = Q3EUtils.ActiveIsLandscape(activity);
        PORTRAIT = Q3EUtils.ActiveIsPortrait(activity);
        INVERT = Q3EUtils.ActiveIsInvert(activity);
        int[] fullSize = Q3EUtils.GetFullScreenSize(activity);
        int[] size = Q3EUtils.GetNormalScreenSize(activity);
        FULL_WIDTH = fullSize[0];
        FULL_HEIGHT = fullSize[1];
        DISPLAY_WIDTH = size[0];
        DISPLAY_HEIGHT = size[1];
        EDGE_START_HEIGHT = Q3EUtils.GetEdgeHeight(activity, LANDSCAPE);
        EDGE_END_HEIGHT = Q3EUtils.GetEndEdgeHeight(activity, LANDSCAPE);
        if(LANDSCAPE)
            NAV_BAR_HEIGHT = fullSize[0] - size[0] - EDGE_START_HEIGHT - EDGE_END_HEIGHT;
        else
            NAV_BAR_HEIGHT = fullSize[1] - size[1] - EDGE_START_HEIGHT - EDGE_END_HEIGHT;
        FULLSCREEN = FULL_WIDTH == DISPLAY_WIDTH && FULL_HEIGHT == DISPLAY_HEIGHT;
    }

    private KScreenInfo(boolean PORTRAIT, boolean LANDSCAPE, boolean INVERT, boolean FULLSCREEN, int FULL_WIDTH, int FULL_HEIGHT, int DISPLAY_WIDTH, int DISPLAY_HEIGHT, int NAV_BAR_HEIGHT, int EDGE_START_HEIGHT, int EDGE_END_HEIGHT)
    {
        this.PORTRAIT = PORTRAIT;
        this.LANDSCAPE = LANDSCAPE;
        this.INVERT = INVERT;
        this.FULLSCREEN = FULLSCREEN;
        this.FULL_WIDTH = FULL_WIDTH;
        this.FULL_HEIGHT = FULL_HEIGHT;
        this.DISPLAY_WIDTH = DISPLAY_WIDTH;
        this.DISPLAY_HEIGHT = DISPLAY_HEIGHT;
        this.NAV_BAR_HEIGHT = NAV_BAR_HEIGHT;
        this.EDGE_START_HEIGHT = EDGE_START_HEIGHT;
        this.EDGE_END_HEIGHT = EDGE_END_HEIGHT;
    }

    public KScreenInfo ToPortrait()
    {
        KScreenInfo portrait = new KScreenInfo(
                true, false, false, FULLSCREEN,
                FULL_WIDTH > FULL_HEIGHT ? FULL_HEIGHT : FULL_WIDTH,
                FULL_WIDTH > FULL_HEIGHT ? FULL_WIDTH : FULL_HEIGHT,
                DISPLAY_WIDTH > DISPLAY_HEIGHT ? DISPLAY_HEIGHT : DISPLAY_WIDTH,
                DISPLAY_WIDTH > DISPLAY_HEIGHT ? DISPLAY_WIDTH : DISPLAY_HEIGHT,
                NAV_BAR_HEIGHT, INVERT ? EDGE_END_HEIGHT : EDGE_START_HEIGHT, INVERT ? EDGE_START_HEIGHT : EDGE_END_HEIGHT
        );

        return portrait;
    }

    public KScreenInfo ToLandscape()
    {
        KScreenInfo landscape = new KScreenInfo(
                false, true, false, FULLSCREEN,
                FULL_WIDTH < FULL_HEIGHT ? FULL_HEIGHT : FULL_WIDTH,
                FULL_WIDTH < FULL_HEIGHT ? FULL_WIDTH : FULL_HEIGHT,
                DISPLAY_WIDTH < DISPLAY_HEIGHT ? DISPLAY_HEIGHT : DISPLAY_WIDTH,
                DISPLAY_WIDTH < DISPLAY_HEIGHT ? DISPLAY_WIDTH : DISPLAY_HEIGHT,
                NAV_BAR_HEIGHT, INVERT ? EDGE_END_HEIGHT : EDGE_START_HEIGHT, INVERT ? EDGE_START_HEIGHT : EDGE_END_HEIGHT
        );

        return landscape;
    }

    public int StartX()
    {
        return LANDSCAPE ? EDGE_START_HEIGHT : 0;
    }

    public int StartY()
    {
        return LANDSCAPE ? 0 : EDGE_START_HEIGHT;
    }

    public int EndX()
    {
        return LANDSCAPE ? EDGE_END_HEIGHT + NAV_BAR_HEIGHT : 0;
    }

    public int EndY()
    {
        return LANDSCAPE ? 0 : EDGE_END_HEIGHT + NAV_BAR_HEIGHT;
    }

    @Override
    public String toString()
    {
        return "KScreenInfo{" +
                "PORTRAIT=" + PORTRAIT +
                ", LANDSCAPE=" + LANDSCAPE +
                ", INVERT=" + INVERT +
                ", FULL_WIDTH=" + FULL_WIDTH +
                ", FULL_HEIGHT=" + FULL_HEIGHT +
                ", DISPLAY_WIDTH=" + DISPLAY_WIDTH +
                ", DISPLAY_HEIGHT=" + DISPLAY_HEIGHT +
                ", NAV_BAR_HEIGHT=" + NAV_BAR_HEIGHT +
                ", EDGE_START_HEIGHT=" + EDGE_START_HEIGHT +
                ", EDGE_END_HEIGHT=" + EDGE_END_HEIGHT +
                '}';
    }

    public static KScreenInfo FromActivity(Activity activity)
    {
        KScreenInfo screenInfo = new KScreenInfo(activity);
        //System.err.println(screenInfo.toString());
        return screenInfo;
    }
}
