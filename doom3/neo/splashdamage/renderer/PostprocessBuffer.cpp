#include "idlib/precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"

#include "PostprocessBuffer.h"

static idCVar harm_r_clearPostprocessBuffer("harm_r_clearPostprocessBuffer", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "clear postprocess buffer image on every draw");

sdPostprocessBuffer::sdPostprocessBuffer()
	: width(-1),
	height(-1),
	fb(NULL),
	currentBuffer(-1)
{
	for(int k = 0; k < sizeof(globalImages->postProcessBuffers) / sizeof(globalImages->postProcessBuffers[0]); k++)
    {
        images[k] = NULL;
    }
}

bool sdPostprocessBuffer::Init(int w, int h, float scale)
{
	if(fb)
	{
		if(w != fb->Width() || h != fb->Height())
		{
			Shutdown();
		}
		else
		{
			return true;
		}
	}

	if(scale == 1.0f)
	{
		width = w;
		height = h;
	}
	else
	{
		width = idMath::Ftoi(roundf((float)w * scale));
		height = idMath::Ftoi(roundf((float)h * scale));
	}
	fb = new idFramebuffer("sdPostprocessBuffer", w, h);
	for(int k = 0; k < sizeof(globalImages->postProcessBuffers) / sizeof(globalImages->postProcessBuffers[0]); k++)
	{
		images[k] = globalImages->GetImage(va("_postProcessBuffer_%d", k));
	}
	fb->Bind();
    fb->AddDepthStencilBuffer(GL_DEPTH24_STENCIL8);
    fb->AddColorBuffer(GL_RGBA8, 0);
	fb->Check();
	fb->Unbind();

	return true;
}

void sdPostprocessBuffer::Shutdown(void)
{
	if(fb)
	{
		fb->Purge();
		delete fb;
		fb = NULL;
	}
	for(int k = 0; k < sizeof(globalImages->postProcessBuffers) / sizeof(globalImages->postProcessBuffers[0]); k++)
    {
        images[k] = NULL;
    }
	currentBuffer = -1;
}

void sdPostprocessBuffer::Begin(int index)
{
    assert(fb);
    assert(index >= 0 && index < sizeof(globalImages->postProcessBuffers) / sizeof(globalImages->postProcessBuffers[0]));
    fb->Bind();
	currentBuffer = index;
	UploadImage();
	fb->AttachImage2D(images[currentBuffer]);
	if(harm_r_clearPostprocessBuffer.GetBool())
		Clear();
}

void sdPostprocessBuffer::UploadImage(void) const
{
    assert(currentBuffer != -1 && images[currentBuffer]);
	if(images[currentBuffer]->uploadWidth < fb->Width() || images[currentBuffer]->uploadHeight < fb->Height())
	{
		int nw = MakePowerOfTwo(fb->Width());
		int nh = MakePowerOfTwo(fb->Height());
		byte *pic = (byte *)Mem_ClearedAlloc(nw * nh * 4);
		images[currentBuffer]->GenerateImage(pic, nw, nh, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY);
		images[currentBuffer]->sourceWidth = fb->Width();
		images[currentBuffer]->sourceHeight = fb->Height();
		Mem_Free(pic);
	}
}

void sdPostprocessBuffer::End(void)
{
    assert(fb);
	/*
	images[currentBuffer]->CopyFramebuffer(backEnd.viewDef->viewport.x1,
			backEnd.viewDef->viewport.y1,  backEnd.viewDef->viewport.x2 -  backEnd.viewDef->viewport.x1 + 1,
			backEnd.viewDef->viewport.y2 -  backEnd.viewDef->viewport.y1 + 1, true);
	*/
#if 0
	static idCVar ppp("ppp", "0", 0, "");
	int w = images[currentBuffer]->uploadWidth;
	int h = images[currentBuffer]->uploadHeight;
	if (ppp.GetBool())
	{
		GLint packAlign;
		qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
		qglPixelStorei(GL_PACK_ALIGNMENT, 1);	// otherwise small rows get padded to 32 bits

		byte *data = (byte *)malloc(w * h * 4);
		qglReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglPixelStorei(GL_PACK_ALIGNMENT, packAlign);	// otherwise small rows get padded to 32 bits

		extern void R_WritePNG(const char *filename, const byte *data, int w, int h, int comp, bool flipVertical = false, int quality = 100, const char *basePath = NULL);
		//R_WritePNG(va("texturesxxx/%d_%d.png", tr.frameCount, currentBuffer), data, w, h, 4);

		//fileSystem->WriteTGA(va("texturesxxx/%d_%d.tga", tr.frameCount, currentBuffer), data, w, h);
		free(data);
	}
#endif
	fb->AttachColorBuffer();
    fb->Unbind();
	currentBuffer = -1;
#if 0
	if (ppp.GetBool())
	{
		GLint packAlign;
		qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
		qglPixelStorei(GL_PACK_ALIGNMENT, 1);	// otherwise small rows get padded to 32 bits

		byte *data = (byte *)malloc(w * h * 4);
		qglReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglPixelStorei(GL_PACK_ALIGNMENT, packAlign);	// otherwise small rows get padded to 32 bits

		extern void R_WritePNG(const char *filename, const byte *data, int w, int h, int comp, bool flipVertical = false, int quality = 100, const char *basePath = NULL);
		//R_WritePNG(va("texturesxxx/%d_%d.png", tr.frameCount, currentBuffer), data, w, h,4);

		fileSystem->WriteTGA(va("texturesxxx/%d_%d.tga", tr.frameCount, currentBuffer), data, w, h);
		free(data);
	}
#endif
}

int sdPostprocessBuffer::UploadWidth(void) const
{
    assert(currentBuffer != -1 && images[currentBuffer]);
    return images[currentBuffer]->uploadWidth;
}

int sdPostprocessBuffer::UploadHeight(void) const
{
    assert(currentBuffer != -1 && images[currentBuffer]);
    return images[currentBuffer]->uploadHeight;
}

void sdPostprocessBuffer::Clear(void) const
{
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

uint32_t sdPostprocessBuffer::GetFramebuffer(void) const
{
	assert(fb);
	return fb->GetFramebuffer();
}

void sdPostprocessBuffer::ClearAll(void) const
{
	if(harm_r_clearPostprocessBuffer.GetBool())
		return;
	assert(fb);
    fb->Bind();
	for(int k = 0; k < sizeof(globalImages->postProcessBuffers) / sizeof(globalImages->postProcessBuffers[0]); k++)
	{
		fb->AttachImage2D(images[k]);
		Clear();
		fb->AttachColorBuffer();
	}
    fb->Unbind();
}

sdPostprocessBuffer postprocessBuffer;
