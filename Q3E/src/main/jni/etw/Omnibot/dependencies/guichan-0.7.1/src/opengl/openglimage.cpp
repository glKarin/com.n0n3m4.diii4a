/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naessén and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naessén a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

#include "guichan/opengl/openglimage.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
    OpenGLImage::OpenGLImage(const unsigned int* pixels, int width, int height,
                             bool convertToDisplayFormat)
    {
        mAutoFree = true;

        mWidth = width;
        mHeight = height;
		mTextureWidth = 1, mTextureHeight = 1;

        while(mTextureWidth < mWidth)
        {
            mTextureWidth *= 2;
        }

        while(mTextureHeight < mHeight)
        {
            mTextureHeight *= 2;
        }

		// Create a new pixel array and copy the pixels into it
		mPixels = new unsigned int[mTextureWidth * mTextureHeight];

#ifdef __BIG_ENDIAN__
        const unsigned int magicPink = 0xff00ffff;
#else
        const unsigned int magicPink = 0xffff00ff;
#endif
		int x, y;
		for (y = 0; y < mTextureHeight; y++)
		{
			for (x = 0; x < mTextureWidth; x++)
			{
				if (x < mWidth && y < mHeight)
				{
					unsigned int c = pixels[x + y * mWidth];

					// Magic pink to transparent
					if (c == magicPink)
					{
						c = 0x00000000;
					}

					mPixels[x + y * mTextureWidth] = c;
				}
				else
				{
					mPixels[x + y * mTextureWidth] = 0x00000000;
				}
			}
		}

        if (convertToDisplayFormat)
        {
            OpenGLImage::convertToDisplayFormat();
        }
    }

    OpenGLImage::OpenGLImage(GLuint textureHandle, int width, int height, bool autoFree)
    {
        mTextureHandle = textureHandle;
        mAutoFree = autoFree;
		mPixels = NULL;

		mWidth = width;
        mHeight = height;
		mTextureWidth = 1, mTextureHeight = 1;

        while(mTextureWidth < mWidth)
        {
            mTextureWidth *= 2;
        }

        while(mTextureHeight < mHeight)
        {
            mTextureHeight *= 2;
        }
    }

    OpenGLImage::~OpenGLImage()
    {
        if (mAutoFree)
        {
            free();
        }
    }

    GLuint OpenGLImage::getTextureHandle() const
    {
        return mTextureHandle;
    }

    int OpenGLImage::getTextureWidth() const
    {
		return mTextureWidth;
    }

    int OpenGLImage::getTextureHeight() const
    {
		return mTextureHeight;
    }

    void OpenGLImage::free()
    {
		if (mPixels == NULL)
		{
			glDeleteTextures(1, &mTextureHandle);
		}
		else
		{
			delete[] mPixels;
			mPixels = NULL;
		}
    }

    int OpenGLImage::getWidth() const
    {
        return mWidth;
    }

    int OpenGLImage::getHeight() const
    {
        return mHeight;
    }

    Color OpenGLImage::getPixel(int x, int y)
    {
		if (mPixels == NULL)
		{
			throw GCN_EXCEPTION("Image has been converted to display format");
		}

		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		{
			throw GCN_EXCEPTION("Coordinates outside of the image");
		}

		unsigned int c = mPixels[x + y * mTextureWidth];

#ifdef __BIG_ENDIAN__
		unsigned char r = (c >> 24) & 0xff;
		unsigned char g = (c >> 16) & 0xff;
		unsigned char b = (c >> 8) & 0xff;
		unsigned char a = c & 0xff;
#else
		unsigned char a = (c >> 24) & 0xff;
		unsigned char b = (c >> 16) & 0xff;
		unsigned char g = (c >> 8) & 0xff;
		unsigned char r = c & 0xff;
#endif

        return Color(r, g, b, a);
    }

    void OpenGLImage::putPixel(int x, int y, const Color& color)
    {
        if (mPixels == NULL)
		{
			throw GCN_EXCEPTION("Image has been converted to display format");
		}

		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		{
			throw GCN_EXCEPTION("Coordinates outside of the image");
		}

#ifdef __BIG_ENDIAN__
		unsigned int c = color.a | color.b << 8 | color.g << 16 | color.r << 24;
#else
		unsigned int c = color.r | color.g << 8 | color.b << 16 | color.a << 24;
#endif

		mPixels[x + y * mTextureWidth] = c;
    }

    void OpenGLImage::convertToDisplayFormat()
    {
		if (mPixels == NULL)
		{
			throw GCN_EXCEPTION("Image has already been converted to display format");
		}

        glGenTextures(1, &mTextureHandle);
        glBindTexture(GL_TEXTURE_2D, mTextureHandle);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     4,
                     mTextureWidth,
                     mTextureHeight,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     mPixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        delete[] mPixels;
		mPixels = NULL;

        GLenum error = glGetError();
        if (error)
        {
            std::string errmsg;
            switch (error)
            {
              case GL_INVALID_ENUM:
                  errmsg = "GL_INVALID_ENUM";
                  break;

              case GL_INVALID_VALUE:
                  errmsg = "GL_INVALID_VALUE";
                  break;

              case GL_INVALID_OPERATION:
                  errmsg = "GL_INVALID_OPERATION";
                  break;

              case GL_STACK_OVERFLOW:
                  errmsg = "GL_STACK_OVERFLOW";
                  break;

              case GL_STACK_UNDERFLOW:
                  errmsg = "GL_STACK_UNDERFLOW";
                  break;

              case GL_OUT_OF_MEMORY:
                  errmsg = "GL_OUT_OF_MEMORY";
                  break;
            }

            throw GCN_EXCEPTION(std::string("Unable to convert to OpenGL display format, glGetError said: ") + errmsg);
        }
    }
}
