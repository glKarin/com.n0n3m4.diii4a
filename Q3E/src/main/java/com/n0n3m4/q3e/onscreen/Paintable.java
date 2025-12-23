package com.n0n3m4.q3e.onscreen;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Paintable
{
    float red = 1;
    float green = 1;
    float blue = 1;
    public float alpha;

    public String tex_androidid;

    public void Paint(GL11 gl)
    {
        //Empty
    }

    public void loadtex(GL10 gl)
    {
        //Empty by default
    }

    public void AsBuffer(GL11 gl)
    {
        //Empty by default
    }

    public void Release(GL11 gl)
    {
        //Empty by default
    }

    public void PaintInEditor(GL11 gl)
    {
        Paint(gl);
    }
}
