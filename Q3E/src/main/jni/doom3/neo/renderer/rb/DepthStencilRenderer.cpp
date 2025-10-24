#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

extern int MakePowerOfTwo(int num);

idDepthStencilRenderer depthStencilRenderer;

void R_CreateOfflineScreenDepthStencilTexture(idImage *image)
{
    int w = depthStencilRenderer.width;
    int h = depthStencilRenderer.height;
    w = MakePowerOfTwo(w);
    h = MakePowerOfTwo(h);
    image->GenerateDepthStencilImage(w, h, false, TF_NEAREST, TR_CLAMP_TO_BORDER, 24, 8, false);
}

idDepthStencilRenderer::idDepthStencilRenderer()
	: width(-1),
	height(-1),
	fb(NULL),
    depthStencilTexture(NULL)
{
}

bool idDepthStencilRenderer::Init(int w, int h)
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
	fb = new idFramebuffer("idDepthStencilRenderer", width, height);
    depthStencilTexture = new idImage;
    depthStencilTexture->imgName = "idStencilTexture_depthStencil";
    R_CreateOfflineScreenDepthStencilTexture(depthStencilTexture);
	fb->Bind();
    fb->AddColorBuffer(GL_RGBA8, 0);
    fb->AddDepthStencilBuffer(GL_DEPTH24_STENCIL8);
	fb->AttachImageDepthStencil(depthStencilTexture);
	fb->Check();
	fb->Unbind();

	return true;
}

void idDepthStencilRenderer::Shutdown(void)
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

void idDepthStencilRenderer::Begin(void)
{
    assert(fb);
    fb->Bind();
}

void idDepthStencilRenderer::BeginBlit(void)
{
    assert(fb);
    fb->Bind();
    fb->AttachImageDepthStencil(depthStencilTexture);
}

void idDepthStencilRenderer::BeginRender(void)
{
    assert(fb);
    fb->Bind();
    fb->AttachDepthStencilBuffer();
}

void idDepthStencilRenderer::End(void)
{
    assert(fb);
    fb->Unbind();
}

bool idDepthStencilRenderer::IsAvailable(void)
{
    return true; //USING_GLES3;
    // return GL_BLIT_FRAMEBUFFER_AVAILABLE();
}

void idDepthStencilRenderer::BindStencil(void)
{
#ifdef GL_ES_VERSION_3_0
    assert(depthStencilTexture);
    depthStencilTexture->Bind();
    // assert(USING_GLES31);
    // if(!USING_GLES31)
    qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
#endif
}

void idDepthStencilRenderer::BindDepth(void)
{
    assert(depthStencilTexture);
    depthStencilTexture->Bind();
#ifdef GL_ES_VERSION_3_0
    // assert(USING_GLES31);
    // if(!USING_GLES31)
    qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT );
#endif
}

int idDepthStencilRenderer::UploadWidth(void) const
{
    assert(depthStencilTexture);
    return depthStencilTexture->uploadWidth;
}

int idDepthStencilRenderer::UploadHeight(void) const
{
    assert(depthStencilTexture);
    return depthStencilTexture->uploadHeight;
}

void idDepthStencilRenderer::Blit(GLint mask)
{
#ifdef GL_ES_VERSION_3_0
    // if(!USING_GLES31) return;

    // assert(USING_GLES31);
    assert(fb);

    depthStencilRenderer.BeginBlit();
    depthStencilRenderer.End();

    const bool IsScissorTest = qglIsEnabled(GL_SCISSOR_TEST);
    if(IsScissorTest)
        qglDisable(GL_SCISSOR_TEST);

    qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb->GetFramebuffer());

    //qglReadBuffer(GL_BACK);
#if 1 //karin: in shader, texcoord.xy = gl_FragCoord.xy * (vec2(1.0) / vec2(width, height)/* u_windowCoords.xy */ * (depthStencilTexture->uploadWidth / depthStencilTexture->uploadHeight) /* u_nonPowerOfTwo.xy */)
    // or texcoord.xy = gl_FragCoord.xy / textureSize(depthStencilTexture, 0)
    qglBlitFramebuffer(0, 0, width, height, 0, 0, width, height, mask, GL_NEAREST);
#else //karin: in shader, texcoord.xy = gl_FragCoord.xy * (vec2(1.0) / vec2(width, height)/* u_windowCoords.xy */)
    qglBlitFramebuffer(0, 0, width, height, 0, 0, depthStencilTexture->uploadWidth, depthStencilTexture->uploadHeight, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
#endif

    if(IsScissorTest)
        qglEnable(GL_SCISSOR_TEST);
    //backEnd.glState.currentFramebuffer = NULL;

    qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif
}

uint32_t idDepthStencilRenderer::GetFramebuffer(void) const
{
	assert(fb);
	return fb->GetFramebuffer();
}
