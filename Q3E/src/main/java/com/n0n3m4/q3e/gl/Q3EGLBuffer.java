package com.n0n3m4.q3e.gl;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

import javax.microedition.khronos.opengles.GL11;

public class Q3EGLBuffer
{
    protected int buffer = 0;
    protected int type = 0;
    protected int usage = GL11.GL_STATIC_DRAW;

    public void Bind(GL11 gl)
    {
        gl.glBindBuffer(type, buffer);
    }

    public void Unbind(GL11 gl)
    {
        gl.glBindBuffer(type, 0);
    }

    public void Delete(GL11 gl)
    {
        if(buffer > 0)
        {
            System.err.println("DDD " + buffer);
            IntBuffer buffers = ByteBuffer.allocateDirect(4).order(ByteOrder.nativeOrder()).asIntBuffer();
            buffers.put(buffer);
            gl.glDeleteBuffers(1, buffers);
            buffer = 0;
        }
    }

    public void Gen(GL11 gl)
    {
        if(buffer <= 0)
        {
            IntBuffer buffers = ByteBuffer.allocateDirect(4).order(ByteOrder.nativeOrder()).asIntBuffer();
            gl.glGenBuffers(1, buffers);
            buffer = buffers.get(0);
        }
    }

    public int Buffer()
    {
        return buffer;
    }
}
