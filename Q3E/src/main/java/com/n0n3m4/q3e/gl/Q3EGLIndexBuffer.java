package com.n0n3m4.q3e.gl;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.opengles.GL11;

public class Q3EGLIndexBuffer extends Q3EGLBuffer
{
    private int stride = 0;

    public Q3EGLIndexBuffer()
    {
        type = GL11.GL_ELEMENT_ARRAY_BUFFER;
    }

    public int Stride()
    {
        return stride;
    }

    public void Data(GL11 gl, IntBuffer indexp)
    {
        stride = 4;
        int lastPos = indexp.position();

        Gen(gl);
        Bind(gl);
        gl.glBufferData(type, stride * (indexp.capacity() - lastPos), indexp, usage);
        Unbind(gl);
        indexp.position(lastPos);
    }

    public void Data(GL11 gl, ByteBuffer indexp)
    {
        stride = 1;
        int lastPos = indexp.position();

        Gen(gl);
        Bind(gl);
        gl.glBufferData(type, stride * (indexp.capacity() - lastPos), indexp, usage);
        Unbind(gl);
        indexp.position(lastPos);
    }

    public void Data(GL11 gl, ShortBuffer indexp)
    {
        stride = 2;
        int lastPos = indexp.position();

        Gen(gl);
        Bind(gl);
        gl.glBufferData(type, stride * (indexp.capacity() - lastPos), indexp, usage);
        Unbind(gl);
        indexp.position(lastPos);
    }


    public static class ByteIndexArray
    {
        private ByteBuffer buffer;

        public ByteBuffer Buffer()
        {
            return buffer;
        }

        private ByteBuffer Make(ByteBuffer indexes, byte start)
        {
            int capacity = indexes.capacity();
            int position = indexes.position();
            int count = capacity - position;
            ByteBuffer buffers = ByteBuffer.allocateDirect(count);
            for(int i = position; i < capacity; i++)
            {
                buffers.put((byte)(indexes.get(i) + start));
            }
            indexes.position(position);
            buffers.position(0);
            return buffers;
        }

        public ByteIndexArray Set(ByteBuffer indexes)
        {
            buffer = Make(indexes, (byte)0);
            return this;
        }

        public ByteIndexArray Append(ByteBuffer indexes, byte start)
        {
            ByteBuffer buf = Make(indexes, start);
            if(null == buffer)
            {
                buffer = buf;
                return this;
            }
            int size = buffer.capacity() + buf.capacity();

            ByteBuffer buffers = ByteBuffer.allocateDirect(size);
            buffers.put(buffer).put(buf);
            buffers.position(0);
            buffer = buffers;
            return this;
        }
    }
}
