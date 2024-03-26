package com.n0n3m4.q3e.onscreen;

import android.util.Log;
import android.view.View;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGL;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Slider extends Paintable implements TouchListener
{
    public View view;
    public int cx;
    public int cy;
    public int width;
    public int height;
    float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
    FloatBuffer verts_p;
    float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
    FloatBuffer tex_p;
    byte[] indices = {0, 1, 2, 0, 2, 3};
    ByteBuffer inds_p;
    public int tex_ind;
    public int lkey, ckey, rkey;
    int startx, starty;
    int SLIDE_DIST;
    public int style;
    private int m_lastKey;
    public int tex1_ind, tex2_ind;

    public Slider(View vw, GL10 gl, int center_x, int center_y, int w, int h, String texid,
                  int leftkey, int centerkey, int rightkey, int stl, float a)
    {
        view = vw;
        cx = center_x;
        cy = center_y;
        style = stl;
        alpha = a;
        width = w;
        height = h;
        SLIDE_DIST = w / 3;
        lkey = leftkey;
        ckey = centerkey;
        rkey = rightkey;
        for (int i = 0; i < verts.length; i += 2)
        {
            verts[i] = verts[i] * width + cx;
            verts[i + 1] = verts[i + 1] * height + cy;
        }

        verts_p = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        verts_p.put(verts);
        verts_p.position(0);

        inds_p = ByteBuffer.allocateDirect(indices.length);
        inds_p.put(indices);
        inds_p.position(0);

        tex_p = ByteBuffer.allocateDirect(4 * texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        tex_p.put(texcoords);
        tex_p.position(0);

        tex_androidid = texid;
    }

    @Override
    public void loadtex(GL10 gl)
    {
        tex_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(view.getContext(), tex_androidid));
    }

    @Override
    public void Paint(GL11 gl)
    {
        super.Paint(gl);
        Q3EGL.DrawVerts(gl, tex_ind, 6, tex_p, verts_p, inds_p, 0, 0, red, green, blue, alpha);
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        if (act == 1)
        {
            startx = x;
            starty = y;
            if(IsClickable())
            {
                int key = KeyInPosition(x, y);
                if(key > 0)
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, key, 0);
                    m_lastKey = key;
                }
            }
        }
        else if (act == -1)
        {
            switch (style)
            {
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT: {
                    if (x - startx < -SLIDE_DIST)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, lkey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, lkey, 0);
                    }
                    else if (x - startx > SLIDE_DIST)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, rkey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, rkey, 0);
                    }
                    else
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, ckey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, ckey, 0);
                    }
                }
                break;
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT: {
                    if ((y - starty > SLIDE_DIST) || (x - startx > SLIDE_DIST))
                    {
                        double ang = Math.abs(Math.atan2(y - starty, x - startx));
                        if (ang > Math.PI / 4 && ang < Math.PI * 3 / 4)
                        {
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, lkey, 0);
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, lkey, 0);
                        }
                        else
                        { //k
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, rkey, 0);
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, rkey, 0);
                        } //k
                    }
                    else
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, ckey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, ckey, 0);
                    }
                }
                break;
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK:
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK:
                default: {
                    if(m_lastKey > 0)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, m_lastKey, 0);
                        m_lastKey = 0;
                    }
                }
                break;
            }
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        if (IsClickable())
            return ((2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height));
        else
            return ((2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height)) && (!((y > cy) && (x > cx)));
    }

    private int KeyInPosition(int x, int y)
    {
        if (IsClickable())
        {
            int deltaX = x - cx;
            int slide_dist_2 = SLIDE_DIST / 2;
            if (deltaX < -slide_dist_2)
                return lkey;
            else if (deltaX > slide_dist_2)
                return rkey;
            else
                return ckey;
        }
        else
        {
            int deltaX = x - cx;
            int deltaY = y - cy;
            if (deltaX > 0 && deltaY < 0)
                return rkey;
            else if (deltaX < 0 && deltaY > 0)
                return lkey;
            else if (deltaX <= 0 && deltaY <= 0)
                return ckey;
            else
                return 0;
        }
    }

    public static Slider Move(Slider tmp, GL10 gl)
    {
        Slider news = new Slider(tmp.view, gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid, tmp.lkey, tmp.ckey, tmp.rkey, tmp.style, tmp.alpha);
        news.tex_ind = tmp.tex_ind;
        return news;
    }

    public void Translate(int dx, int dy)
    {
        cx += dx;
        cy += dy;
    }

    public void SetPosition(int x, int y)
    {
        cx = x;
        cy = y;
    }

    public boolean IsClickable()
    {
        return style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK || style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK;
    }
}
