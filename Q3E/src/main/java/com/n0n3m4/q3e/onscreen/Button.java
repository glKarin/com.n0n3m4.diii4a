package com.n0n3m4.q3e.onscreen;

import android.view.View;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.gl.Q3EGLVertexBuffer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Button extends Paintable implements TouchListener
{
    static ArrayList<Integer> heldarr = new ArrayList<>(0);

    public View view;
    public int cx;
    public int cy;
    public int width;
    public int height;

    private final FloatBuffer verts_p;
    private final FloatBuffer tex_p;
    private final ByteBuffer inds_p;
    private int tex_ind;
    private final int keycode;
    private final int style;
    private final float initalpha;
    private final boolean canbeheld;
    private int m_width_2;

    private int lx;
    private int ly;

    private int vertexBuffer = 0;
    private int indexBuffer = 0;
    private boolean allowMouseButtonMotion = true;

    public Button(View vw, GL10 gl, int center_x, int center_y, int w, int h, String texid, int keyc, int stl, boolean canbheld, boolean mouseMotion, float a)
    {
        view = vw;
        cx = center_x;
        cy = center_y;
        alpha = a;
        initalpha = alpha;
        width = w;
        height = h;
        keycode = keyc;
        style = stl;
        canbeheld = canbheld;
        allowMouseButtonMotion = mouseMotion;
        float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
        float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
        byte[] indices = {0, 1, 2, 0, 2, 3};
        for (int i = 0; i < verts.length; i += 2)
        {
            verts[i] = verts[i] * w;
            verts[i + 1] = verts[i + 1] * h;
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

        m_width_2 = width * width;
    }

    @Override
    public void loadtex(GL10 gl)
    {
        tex_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(view.getContext(), tex_androidid));
    }

    @Override
    public void AsBuffer(GL11 gl)
    {
        vertexBuffer = Q3EGL.glBufferData(gl, vertexBuffer, gl.GL_ARRAY_BUFFER, new Q3EGLVertexBuffer()
                .Set(new FloatBuffer[]{ verts_p, tex_p }, 4)
                        .Buffer(),
                gl.GL_STATIC_DRAW);
        indexBuffer = Q3EGL.glBufferData(gl, indexBuffer, gl.GL_ELEMENT_ARRAY_BUFFER, inds_p, gl.GL_STATIC_DRAW);
    }

    @Override
    public void Release(GL11 gl)
    {
        if(tex_ind > 0)
        {
            Q3EGL.glDeleteTexture(gl, tex_ind);
            tex_ind = 0;
        }
        if(vertexBuffer > 0)
        {
            Q3EGL.glDeleteBuffer(gl, vertexBuffer);
            vertexBuffer = 0;
        }
        if(indexBuffer > 0)
        {
            Q3EGL.glDeleteBuffer(gl, indexBuffer);
            indexBuffer = 0;
        }
    }

    @Override
    public void Paint(GL11 gl)
    {
        super.Paint(gl);
        //Q3EGL.DrawVerts_GL1(gl, tex_ind, 6, tex_p, verts_p, inds_p, cx, cy, red, green, blue, alpha);
        Q3EGL.DrawVerts_GL1(gl, tex_ind, 6, vertexBuffer, indexBuffer, cx, cy, red, green, blue, alpha);
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        if (canbeheld)
        {
            if (act == 1)
            {
                if (!heldarr.contains(keycode))
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, keycode, 0);
                    heldarr.add(keycode);
                    alpha = Math.min(initalpha * 2, 1f);
                }
                else
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, keycode, 0);
                    heldarr.remove(Integer.valueOf(keycode));
                    alpha = initalpha;
                }
            }
            return true;
        }

        if (keycode == Q3EKeyCodes.K_VKBD)
        {
            if (act == 1)
                Q3EUtils.togglevkbd(this.view);
            return true;
        }

        if (act == 1)
        {
            lx = x;
            ly = y;
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, keycode, 0);
        }
        else if (act == -1)
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, keycode, 0);

        if(allowMouseButtonMotion)
        {
        if (keycode == Q3EKeyCodes.KeyCodes.K_MOUSE1 || keycode == Q3EKeyCodes.KeyCodes.K_MOUSE2)
        {
            if (Q3EUtils.q3ei.callbackObj.notinmenu)
            {
                Q3EUtils.q3ei.callbackObj.sendMotionEvent(x - lx, y - ly);
                lx = x;
                ly = y;
            }
            else
            {
                Q3EUtils.q3ei.callbackObj.sendMotionEvent(0, 0);//???
            }
        }
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        if (style == 0)
        {
            int dx = cx - x;
            int dy = cy - y;
            return 4 * (dx * dx + dy * dy) <= m_width_2;
        }
        else if (style == 1)
        {
            int dx = x - cx;
            int dy = cy - y;
            return (((dy) <= (dx)) && (2 * Math.abs(dx) < width) && (2 * Math.abs(dy) < height));
        }
        else if (style == 2)
        {
            int dx = x - cx;
            int dy = cy - y;
            return ((2 * Math.abs(dx) < width) && (2 * Math.abs(dy) < height));
        }
        else if (style == 3)
        {
            int dx = cx - x;
            int dy = y - cy;
            return (((dy) <= (dx)) && (2 * Math.abs(dx) < width) && (2 * Math.abs(dy) < height));
        }
        return false;
    }

    public static Button Move(Button tmp, GL10 gl)
    {
        Button newb = new Button(tmp.view, gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid, tmp.keycode, tmp.style, tmp.canbeheld, tmp.allowMouseButtonMotion, tmp.alpha);
        newb.tex_ind = tmp.tex_ind;
        newb.vertexBuffer = tmp.vertexBuffer;
        newb.indexBuffer = tmp.indexBuffer;
        return newb;
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

    // run on GL thread
    public void Resize(int w, int h)
    {
        width = w;
        height = h;

        float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
        for (int i = 0; i < verts.length; i += 2)
        {
            verts[i] = verts[i] * w;
            verts[i + 1] = verts[i + 1] * h;
        }

        verts_p.put(verts);
        verts_p.position(0);

        m_width_2 = width * width;
    }

    public int Style()
    {
        return style;
    }

    public static int HeightForWidth(int width, int type)
    {
        int bh = width;
        if (type == Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL)
            bh = width;
        else if (type == Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM)
            bh = width;
        else if (type == Q3EGlobals.ONSCREEN_BUTTON_TYPE_CENTER)
            bh = width / 2;
        return bh;
    }

    // height / width
    public static float CalcAspect(int type)
    {
        if (type == Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL)
            return 1.0f;
        else if (type == Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM)
            return 1.0f;
        else if (type == Q3EGlobals.ONSCREEN_BUTTON_TYPE_CENTER)
            return 0.5f;
        return 1.0f;
    }
}
