// RenderTexture.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../renderer/tr_local.h"

/*
========================
idRenderTexture::idRenderTexture
========================
*/
idRenderTexture::idRenderTexture(idImage *colorImage, idImage *depthImage) {
#if 0
	deviceHandle = -1;
	if (colorImage != 0)
	{
		AddRenderImage(colorImage);
	}
	this->depthImage = depthImage;
#endif
}

/*
========================
idRenderTexture::~idRenderTexture
========================
*/
idRenderTexture::~idRenderTexture() {
#if 0
	if (deviceHandle != -1)
	{
		glDeleteFramebuffers(1, &deviceHandle);
		deviceHandle = -1;
	}
#endif
}
/*
================
idRenderTexture::AddRenderImage
================
*/
void idRenderTexture::AddRenderImage(idImage *image) {
#if 0
	if (deviceHandle != -1) {
		common->FatalError("idRenderTexture::AddRenderImage: Can't add render image after FBO has been created!");
	}

	colorImages.Append(image);
#endif
}

/*
================
idRenderTexture::InitRenderTexture
================
*/
void idRenderTexture::InitRenderTexture(void) {
#if 0
	glGenFramebuffers(1, &deviceHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, deviceHandle);

	bool isTexture3D = false;
	if ((colorImages.Num() > 0 && colorImages[0]->GetOpts().textureType == TT_CUBIC) || ((depthImage != 0) && depthImage->GetOpts().textureType == TT_CUBIC))
	{
		isTexture3D = true;
	}
	
	if (!isTexture3D)
	{
		for (int i = 0; i < colorImages.Num(); i++) {
			if (colorImages[i]->GetOpts().numMSAASamples == 0)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorImages[i]->GetDeviceHandle(), 0);
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, colorImages[i]->GetDeviceHandle(), 0);
			}
		}

		if (depthImage != 0) {
			if (depthImage->GetOpts().numMSAASamples == 0)
			{
				if (depthImage->GetOpts().format == FMT_DEPTH) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthImage->GetDeviceHandle(), 0);
				}
				else if (depthImage->GetOpts().format == FMT_DEPTH_STENCIL) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthImage->GetDeviceHandle(), 0);
				}
				else {
					common->FatalError("idRenderTexture::InitRenderTexture: Unknown depth buffer format!");
				}
			}
			else
			{
				if (depthImage->GetOpts().format == FMT_DEPTH) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthImage->GetDeviceHandle(), 0);
				}
				else if (depthImage->GetOpts().format == FMT_DEPTH_STENCIL) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthImage->GetDeviceHandle(), 0);
				}
				else {
					common->FatalError("idRenderTexture::InitRenderTexture: Unknown depth buffer format!");
				}
			}
		}

		if (colorImages.Num() > 0)
		{
			GLenum DrawBuffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
			if (colorImages.Num() >= 5) {
				common->FatalError("InitRenderTextures: Too many render targets!");
			}
			glDrawBuffers(colorImages.Num(), &DrawBuffers[0]);
		}
	}
	else
	{
		if (colorImages.Num() > 0)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, colorImages[0]->GetDeviceHandle(), 0);
		}

		if (depthImage != 0) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, depthImage->GetDeviceHandle(), 0);
		}
	}


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		common->FatalError("idRenderTexture::InitRenderTexture: Failed to create rendertexture!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

/*
================
idRenderTexture::MakeCurrent
================
*/
void idRenderTexture::MakeCurrent(void) {
#if 0
	glBindFramebuffer(GL_FRAMEBUFFER, deviceHandle);
#endif
}

/*
================
idRenderTexture::BindNull
================
*/
void idRenderTexture::BindNull(void) {
#if 0
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

/*
================
idRenderTexture::Resize
================
*/
void idRenderTexture::Resize(int width, int height) {
#if 0
	idImage *target = 0;

	if (colorImages.Num() > 0) {
		target = colorImages[0];
	}
	else {
		target = depthImage;
	}

	if (target->GetOpts().width == width && target->GetOpts().height == height) {
		return;
	}

	for(int i = 0; i < colorImages.Num(); i++) {
		colorImages[i]->Resize(width, height);
	}

	if (depthImage != 0) {
		depthImage->Resize(width, height);
	}

	if (deviceHandle != -1)
	{
		glDeleteFramebuffers(1, &deviceHandle);
		deviceHandle = -1;
	}

	InitRenderTexture();
#endif
}

#if 0
#ifdef _RAVEN // ID_INLINE
int			idRenderTexture::GetWidth() const { return ( colorImages.Num() > 0 ) ? colorImages[0]->GetUploadWidth() : depthImage->GetUploadWidth(); }
int			idRenderTexture::GetHeight() const { return (colorImages.Num() > 0) ? colorImages[0]->GetUploadHeight() : depthImage->GetUploadHeight(); }
#endif
#endif
int			idRenderTexture::GetWidth() const { return 0; }
int			idRenderTexture::GetHeight() const { return 0; }
