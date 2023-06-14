package com.n0n3m4.q3e.onscreen;

import android.view.View;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.GL;

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
        tex_ind = GL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(view.getContext(), tex_androidid));
    }

    @Override
    public void Paint(GL11 gl)
    {
        super.Paint(gl);
        GL.DrawVerts(gl, tex_ind, 6, tex_p, verts_p, inds_p, 0, 0, red, green, blue, alpha);
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        Q3EControlView controlView = (Q3EControlView) (this.view);
        if (act == 1)
        {
            startx = x;
            starty = y;
        }
        if (act == -1)
        {
            if (style == 0)
            {
                if (x - startx < -SLIDE_DIST)
                {
                    controlView.sendKeyEvent(true, lkey, 0);
                    controlView.sendKeyEvent(false, lkey, 0);
                }
                else if (x - startx > SLIDE_DIST)
                {
                    controlView.sendKeyEvent(true, rkey, 0);
                    controlView.sendKeyEvent(false, rkey, 0);
                }
                else
                {
                    controlView.sendKeyEvent(true, ckey, 0);
                    controlView.sendKeyEvent(false, ckey, 0);
                }
            }

            //k
            else if (style == 1)
            {
                if ((y - starty > SLIDE_DIST) || (x - startx > SLIDE_DIST))
                {
                    double ang = Math.abs(Math.atan2(y - starty, x - startx));
                    if (ang > Math.PI / 4 && ang < Math.PI * 3 / 4)
                    {
                        controlView.sendKeyEvent(true, lkey, 0);
                        controlView.sendKeyEvent(false, lkey, 0);
                    }
                    else
                    { //k
                        controlView.sendKeyEvent(true, rkey, 0);
                        controlView.sendKeyEvent(false, rkey, 0);
                    } //k
                }
                else
                {
                    controlView.sendKeyEvent(true, ckey, 0);
                    controlView.sendKeyEvent(false, ckey, 0);
                }
            }
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        if (style == 0)
            return ((2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height));
        else
            return ((2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height)) && (!((y > cy) && (x > cx)));
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
}
