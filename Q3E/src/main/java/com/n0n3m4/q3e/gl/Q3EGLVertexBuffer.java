package com.n0n3m4.q3e.gl;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL11;

public class Q3EGLVertexBuffer extends Q3EGLBuffer
{
    private int stride = 0;

    public Q3EGLVertexBuffer()
    {
        type = GL11.GL_ARRAY_BUFFER;
    }

    public int Stride()
    {
        return stride;
    }

    public void Data(GL11 gl, FloatBuffer verts, int count)
    {
        stride = count * 4;
        int lastPos = verts.position();

        Gen(gl);
        Bind(gl);
        gl.glBufferData(type, 4 * (verts.capacity() - lastPos), verts, usage);
        Unbind(gl);
        verts.position(lastPos);
    }

    public static class VertexArray
    {
        private FloatBuffer buffer;

        public FloatBuffer Buffer()
        {
            return buffer;
        }

        private FloatBuffer Make(FloatBuffer[] verts, int vertexCount)
        {
            int[] vertsPos = new int[verts.length];
            int[] vertsSize = new int[verts.length];
            int[] vertsOffset = new int[verts.length];
            int num = 0;
            for(int i = 0; i < verts.length; i++)
            {
                vertsPos[i] = verts[i].position();
                vertsOffset[i] = verts[i].position();
                vertsSize[i] = (verts[i].capacity() - verts[i].position()) / vertexCount;
                num += vertsSize[i];
            }
            int size = 4 * vertexCount * num;

            FloatBuffer buffers = ByteBuffer.allocateDirect(size).order(ByteOrder.nativeOrder()).asFloatBuffer();
            for(int i = 0; i < vertexCount; i++)
            {
                for(int v = 0; v < verts.length; v++)
                {
                    for(int p = 0; p < vertsSize[v]; p++)
                    {
                        buffers.put(verts[v].get(vertsOffset[v]++));
                    }
                }
            }
            buffers.position(0);

            for(int i = 0; i < verts.length; i++)
            {
                verts[i].position(vertsPos[i]);
            }

            return buffers;
        }

        public VertexArray Set(FloatBuffer[] verts, int vertexCount)
        {
            buffer = Make(verts, vertexCount);
            return this;
        }

        public VertexArray Append(FloatBuffer[] verts, int vertexCount)
        {
            FloatBuffer buf = Make(verts, vertexCount);
            if(null == buffer)
            {
                buffer = buf;
                return this;
            }
            int size = buffer.capacity() + buf.capacity();

            FloatBuffer buffers = ByteBuffer.allocateDirect(size * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
            buffers.put(buffer).put(buf);
            buffers.position(0);
            buffer = buffers;
            return this;
        }
    }

}
