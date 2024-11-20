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

#include "guichan/opengl/openglgraphics.hpp"

#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined (__amigaos4__)
#include <mgl/gl.h>
#define glVertex3i glVertex3f
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "guichan/exception.hpp"
#include "guichan/image.hpp"
#include "guichan/opengl/openglimage.hpp"

namespace gcn
{
	OpenGLGraphics::OpenGLGraphics()
	{
		setTargetPlane(640, 480);
		mAlpha = false;
	}

	OpenGLGraphics::OpenGLGraphics(int width, int height)
	{
		setTargetPlane(width, height);
	}

	OpenGLGraphics::~OpenGLGraphics()
	{

	}

	void OpenGLGraphics::_beginDraw()
	{
		glPushAttrib(
			GL_COLOR_BUFFER_BIT |
			GL_CURRENT_BIT |
			GL_DEPTH_BUFFER_BIT |
			GL_ENABLE_BIT |
			GL_FOG_BIT |
			GL_LIGHTING_BIT |
			GL_LINE_BIT |
			GL_POINT_BIT |
			GL_POLYGON_BIT |
			GL_SCISSOR_BIT |
			GL_STENCIL_BUFFER_BIT |
			GL_TEXTURE_BIT |
			GL_TRANSFORM_BIT |
			GL_POINT_BIT |
			GL_LINE_BIT
			);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glOrtho(0.0,
			(double)mWidth,
			(double)mHeight,
			0.0,
			-1.0,
			1.0);

		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);

		glEnable(GL_SCISSOR_TEST);
		glPointSize(1.0);
		glLineWidth(1.0);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		pushClipArea(Rectangle(0, 0, mWidth, mHeight));
	}

	void OpenGLGraphics::_endDraw()
	{
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glMatrixMode(GL_TEXTURE);
		glPopMatrix();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glPopAttrib();

		popClipArea();
	}

	bool OpenGLGraphics::pushClipArea(Rectangle area)
	{
		bool result = Graphics::pushClipArea(area);

		glScissor(mClipStack.top().x,
			mHeight - mClipStack.top().y - mClipStack.top().height,
			mClipStack.top().width,
			mClipStack.top().height);

		return result;
	}

	void OpenGLGraphics::popClipArea()
	{
		Graphics::popClipArea();

		if (mClipStack.empty())
		{
			return;
		}

		glScissor(mClipStack.top().x,
			mHeight - mClipStack.top().y - mClipStack.top().height,
			mClipStack.top().width,
			mClipStack.top().height);
	}

	void OpenGLGraphics::setTargetPlane(int width, int height)
	{
		mWidth = width;
		mHeight = height;
	}

	void OpenGLGraphics::drawImage(const Image* image,
		int srcX,
		int srcY,
		int dstX,
		int dstY,
		int width,
		int height)
	{
		const OpenGLImage* srcImage = dynamic_cast<const OpenGLImage*>(image);

		if (srcImage == NULL)
		{
			throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an OpenGLImage.");
		}

		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		const ClipRectangle& top = mClipStack.top();

		dstX += top.xOffset;
		dstY += top.yOffset;

		// Find OpenGL texture coordinates
		float texX1 = srcX / (float)srcImage->getTextureWidth();
		float texY1 = srcY / (float)srcImage->getTextureHeight();
		float texX2 = (srcX+width) / (float)srcImage->getTextureWidth();
		float texY2 = (srcY+height) / (float)srcImage->getTextureHeight();

		glBindTexture(GL_TEXTURE_2D, srcImage->getTextureHandle());

		glEnable(GL_TEXTURE_2D);

		// Check if blending already is enabled
		if (!mAlpha)
		{
			glEnable(GL_BLEND);
		}

		// Draw a textured quad -- the image
		glBegin(GL_QUADS);
		glTexCoord2f(texX1, texY1);
		glVertex3i(dstX, dstY, 0);

		glTexCoord2f(texX1, texY2);
		glVertex3i(dstX, dstY + height, 0);

		glTexCoord2f(texX2, texY2);
		glVertex3i(dstX + width, dstY + height, 0);

		glTexCoord2f(texX2, texY1);
		glVertex3i(dstX + width, dstY, 0);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		// Don't disable blending if the color has alpha
		if (!mAlpha)
		{
			glDisable(GL_BLEND);
		}
	}

	void OpenGLGraphics::drawPoint(int x, int y)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		const ClipRectangle& top = mClipStack.top();

		x += top.xOffset;
		y += top.yOffset;

		glBegin(GL_POINTS);
		glVertex2i(x, y);
		glEnd();
	}

	void OpenGLGraphics::drawPoints(const Point *points, int numPoints)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		const ClipRectangle& top = mClipStack.top();

		Color oldColor = getColor();
		Color lastColor = oldColor;

		glBegin(GL_POINTS);
		for(int i = 0; i < numPoints; ++i)
		{
			int x = points[i].x + top.xOffset;
			int y = points[i].y + top.yOffset;

			if(lastColor != points[i].c)
			{
				setColor(points[i].c);
				lastColor = points[i].c;
			}

			glVertex2i(x, y);
		}		
		glEnd();

		setColor(oldColor);
	}

	void OpenGLGraphics::drawLine(int x1, int y1, int x2, int y2)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		const ClipRectangle& top = mClipStack.top();

		x1 += top.xOffset;
		y1 += top.yOffset;
		x2 += top.xOffset;
		y2 += top.yOffset;

		glBegin(GL_LINES);
		glVertex2f(x1 + 0.375f,
			y1 + 0.375f);
		glVertex2f(x2 + 1.0f - 0.375f,
			y2 + 1.0f - 0.375f);
		glEnd();

		/* glBegin(GL_POINTS);
		glVertex2f(x2 + 1.0f - 0.375f,
		y2 + 1.0f - 0.375f);
		glEnd();

		glBegin(GL_POINTS);
		glVertex2f(x1 + 0.375f,
		y1 + 0.375f);
		glEnd();*/        
	}

	void OpenGLGraphics::drawLines(const Line *lines, int numLines)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		if(numLines==0)
			return;

		Color oldColor = getColor();
		Color col = lines[0].c;
		setColor(col);

		const ClipRectangle& top = mClipStack.top();

		glBegin(GL_LINES);

		for(int i = 0; i < numLines; ++i)
		{
			if(lines[i].c != col)
			{
				col = lines[i].c;
				setColor(col);
			}

			int x1 = lines[i].x1 + top.xOffset;
			int y1 = lines[i].y1 + top.yOffset;
			int x2 = lines[i].x2 + top.xOffset;
			int y2 = lines[i].y2 + top.yOffset;

			glVertex2f(x1 + 0.375f,
				y1 + 0.375f);
			glVertex2f(x2 + 1.0f - 0.375f,
				y2 + 1.0f - 0.375f);
		}

		setColor(oldColor);

		glEnd();
	}

	void OpenGLGraphics::drawPolygon(const Triangle *tris, int numTris, bool filled)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		if(numTris==0)
			return;

		Color oldColor = getColor();
		Color col = tris[0].v0.c;
		setColor(col);

		const ClipRectangle& top = mClipStack.top();

		glBegin(GL_TRIANGLES);
		for(int i = 0; i < numTris; ++i)
		{
			const int x1 = tris[i].v0.x + top.xOffset;
			const int y1 = tris[i].v0.y + top.yOffset;
			const int x2 = tris[i].v1.x + top.xOffset;
			const int y2 = tris[i].v1.y + top.yOffset;
			const int x3 = tris[i].v2.x + top.xOffset;
			const int y3 = tris[i].v2.y + top.yOffset;

			if(tris[i].v0.c != col)
			{
				col = tris[i].v0.c;
				setColor(col);
			}

			glVertex2f(
				x1 + 0.375f,
				y1 + 0.375f);

			if(tris[i].v1.c != col)
			{
				col = tris[i].v1.c;
				setColor(col);
			}

			glVertex2f(
				x2 + 1.0f - 0.375f,
				y2 + 1.0f - 0.375f);

			if(tris[i].v2.c != col)
			{
				col = tris[i].v2.c;
				setColor(col);
			}

			glVertex2f(
				x3 + 1.0f - 0.375f,
				y3 + 1.0f - 0.375f);
		}
		glEnd();
	}

	void OpenGLGraphics::drawRectangle(const Rectangle& rectangle)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		const ClipRectangle& top = mClipStack.top();

		glBegin(GL_LINE_LOOP);
		glVertex2f(rectangle.x + top.xOffset,
			rectangle.y + top.yOffset);
		glVertex2f(rectangle.x + rectangle.width + top.xOffset - 1.0f,
			rectangle.y + top.yOffset + 0.375f);
		glVertex2f(rectangle.x + rectangle.width + top.xOffset - 1.0f,
			rectangle.y + rectangle.height + top.yOffset);
		glVertex2f(rectangle.x + top.xOffset,
			rectangle.y + rectangle.height + top.yOffset);
		glEnd();
	}

	void OpenGLGraphics::fillRectangle(const Rectangle& rectangle)
	{
		if (mClipStack.empty())
		{
			throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
		}

		const ClipRectangle& top = mClipStack.top();

		glBegin(GL_QUADS);
		glVertex2i(rectangle.x + top.xOffset,
			rectangle.y + top.yOffset);
		glVertex2i(rectangle.x + rectangle.width + top.xOffset,
			rectangle.y + top.yOffset);
		glVertex2i(rectangle.x + rectangle.width + top.xOffset,
			rectangle.y + rectangle.height + top.yOffset);
		glVertex2i(rectangle.x + top.xOffset,
			rectangle.y + rectangle.height + top.yOffset);
		glEnd();
	}

	void OpenGLGraphics::setColor(const Color& color)
	{
		mColor = color;
		glColor4ub(color.r, color.g, color.b, color.a);

		mAlpha = color.a != 255;

		if (mAlpha)
		{
			glEnable(GL_BLEND);
		}
	}

	const Color& OpenGLGraphics::getColor() const
	{
		return mColor;
	}
}
