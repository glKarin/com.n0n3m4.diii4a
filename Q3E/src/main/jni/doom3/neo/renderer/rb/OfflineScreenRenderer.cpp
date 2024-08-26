#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

extern int MakePowerOfTwo(int num);

// idOfflineScreenRenderer offlineScreenRenderer;

idOfflineScreenRenderer::idOfflineScreenRenderer()
	: width(-1),
	height(-1),
	fb(NULL)
{
}

idOfflineScreenRenderer::~idOfflineScreenRenderer()
{
}

bool idOfflineScreenRenderer::Init(int w, int h)
{
	if(fb)
	{
		if(w != width || h != height)
		{
			Shutdown();
		}
		else
		{
			return true;
		}
	}

	width = w;
	height = h;
	fb = new idFramebuffer("idOfflineScreenRenderer", width, height);
	fb->Bind();
	fb->AddColorBuffer(GL_RGBA8, 0);
	fb->AddDepthStencilBuffer(GL_DEPTH24_STENCIL8);
	fb->Check();
	fb->Unbind();

	return true;
}

void idOfflineScreenRenderer::Shutdown(void)
{
	if(fb)
	{
		fb->Purge();
		delete fb;
		fb = NULL;
	}
}

void idOfflineScreenRenderer::Bind(void)
{
	assert(fb);
	fb->Bind();
}

void idOfflineScreenRenderer::Unbind(void)
{
	assert(fb);
	fb->Unbind();
}

void idOfflineScreenRenderer::Blit(void)
{
	assert(fb);
	qglBindFramebuffer(GL_READ_FRAMEBUFFER, fb->GetFramebuffer());
	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLenum drawbuf = GL_BACK;
	qglDrawBuffers(1, &drawbuf);

	qglBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	Unbind();
	backEnd.glState.currentFramebuffer = NULL;
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t idOfflineScreenRenderer::GetFramebuffer(void) const
{
	assert(fb);
	return fb->GetFramebuffer();
}
