#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

idStencilTexture stencilTexture;

void R_CreateStencilTextureDepthStencilTexture(idImage *image)
{
	int w = stencilTexture.width;
	int h = stencilTexture.height;
	w = MakePowerOfTwo(w);
	h = MakePowerOfTwo(h);
	image->GenerateDepthStencilImage(w, h, false, TF_NEAREST, TR_CLAMP_TO_BORDER, 24, 8, false);
}

idStencilTexture::idStencilTexture()
	: width(-1),
	height(-1),
	fb(NULL),
	depthStencilTexture(NULL)
{
}

bool idStencilTexture::Init(int w, int h)
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
	fb = new idFramebuffer("idStencilTexture", width, height);
	//depthStencilTexture = globalImages->ImageFromFunction("idStencilTexture_depthStencil", R_CreateStencilTextureDepthStencilTexture);
	depthStencilTexture = new idImage;
	depthStencilTexture->imgName = "idStencilTexture_depthStencil";
	R_CreateStencilTextureDepthStencilTexture(depthStencilTexture);
	//depthStencilTexture->Reload(false, false);
	fb->Bind();
#ifdef GL_ES_VERSION_3_0
    GLenum drawbuf = GL_NONE;
    qglDrawBuffers(1, &drawbuf);
#endif
	fb->AttachImageDepthStencil(depthStencilTexture);
	fb->Check();
	fb->Unbind();

	return true;
}

void idStencilTexture::Shutdown(void)
{
	if(fb)
	{
		fb->Purge();
		delete fb;
		fb = NULL;
	}
	if(depthStencilTexture)
	{
		depthStencilTexture->PurgeImage();
		delete depthStencilTexture;
		depthStencilTexture = NULL;
	}
}

void idStencilTexture::Begin(void)
{
	assert(fb);
	fb->Bind();
}

void idStencilTexture::End(void)
{
	assert(fb);
	fb->Unbind();
}

void idStencilTexture::Bind(void) // Stencil
{
#ifdef GL_ES_VERSION_3_0
	assert(depthStencilTexture);
	depthStencilTexture->Bind();
	// assert(USING_GLES31);
	// if(!USING_GLES31)
		qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
#endif
}

void idStencilTexture::BindDepth(void)
{
	assert(depthStencilTexture);
	depthStencilTexture->Bind();
#ifdef GL_ES_VERSION_3_0
	// assert(USING_GLES31);
	// if(!USING_GLES31)
		qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT );
#endif
}

int idStencilTexture::UploadWidth(void) const
{
	assert(depthStencilTexture);
	return depthStencilTexture->uploadWidth;
}

int idStencilTexture::UploadHeight(void) const
{
	assert(depthStencilTexture);
	return depthStencilTexture->uploadHeight;
}

void idStencilTexture::BlitStencil(void)
{
#ifdef GL_ES_VERSION_3_0
	// if(!USING_GLES31) return;

	// assert(USING_GLES31);
	assert(fb);
	const bool IsScissorTest = qglIsEnabled(GL_SCISSOR_TEST);
	if(IsScissorTest)
		qglDisable(GL_SCISSOR_TEST);

	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb->GetFramebuffer());

	//qglReadBuffer(GL_BACK);
#if 1 //karin: in shader, texcoord.xy = gl_FragCoord.xy * (vec2(1.0) / vec2(width, height)/* u_windowCoords.xy */ * (depthStencilTexture->uploadWidth / depthStencilTexture->uploadHeight) /* u_nonPowerOfTwo.xy */)
	// or texcoord.xy = gl_FragCoord.xy / textureSize(depthStencilTexture, 0)
	qglBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
#else //karin: in shader, texcoord.xy = gl_FragCoord.xy * (vec2(1.0) / vec2(width, height)/* u_windowCoords.xy */)
	qglBlitFramebuffer(0, 0, width, height, 0, 0, depthStencilTexture->uploadWidth, depthStencilTexture->uploadHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
#endif

	if(IsScissorTest)
		qglEnable(GL_SCISSOR_TEST);
	//backEnd.glState.currentFramebuffer = NULL;
#endif
}

bool idStencilTexture::IsAvailable(void)
{
	return USING_GLES3;
	// return GL_BLIT_FRAMEBUFFER_AVAILABLE();
}

void idStencilTexture::BlitDepth(void)
{
#ifdef GL_ES_VERSION_3_0
	// if(!USING_GLES31) return;

	// assert(USING_GLES31);
	assert(fb);
	const bool IsScissorTest = qglIsEnabled(GL_SCISSOR_TEST);
	if(IsScissorTest)
		qglDisable(GL_SCISSOR_TEST);

	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb->GetFramebuffer());

	//qglReadBuffer(GL_BACK);
#if 1 //karin: in shader, texcoord.xy = gl_FragCoord.xy * (vec2(1.0) / vec2(width, height)/* u_windowCoords.xy */ * (depthStencilTexture->uploadWidth / depthStencilTexture->uploadHeight) /* u_nonPowerOfTwo.xy */)
	// or texcoord.xy = gl_FragCoord.xy / textureSize(depthStencilTexture, 0)
	qglBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
#else //karin: in shader, texcoord.xy = gl_FragCoord.xy * (vec2(1.0) / vec2(width, height)/* u_windowCoords.xy */)
	qglBlitFramebuffer(0, 0, width, height, 0, 0, depthStencilTexture->uploadWidth, depthStencilTexture->uploadHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
#endif

	if(IsScissorTest)
		qglEnable(GL_SCISSOR_TEST);
	//backEnd.glState.currentFramebuffer = NULL;
#endif
}
