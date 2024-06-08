// RenderTexture.cpp
//

#include "RenderSystem_local.h"

idList<idRenderTexture*> idRenderTexture::renderTextures;

/*
========================
idRenderTexture::FindRenderTexture
========================
*/
idRenderTexture* idRenderTexture::FindRenderTexture(const char* name) {
	for (int i = 0; i < renderTextures.Num(); i++)
	{
		if (renderTextures[i]->GetName() == name)
			return renderTextures[i];
	}
	return nullptr;
}

/*
========================
idRenderTexture::idRenderTexture
========================
*/
idRenderTexture::idRenderTexture(idStr name, idImage *colorImage, idImage *depthImage) {
	deviceHandle = -1;
	this->name = name;

	if (colorImage != nullptr)
	{
		AddRenderImage(colorImage);
	}
	this->depthImage = depthImage;

	renderTextures.Append(this);
}

/*
========================
idRenderTexture::~idRenderTexture
========================
*/
idRenderTexture::~idRenderTexture() {
	if (deviceHandle != -1)
	{
		glDeleteFramebuffers(1, &deviceHandle);
		deviceHandle = -1;
	}
#ifdef __ANDROID__ //karin: debug framebuffer
    if(idRenderTexture::currentRenderTexture == this)
        idRenderTexture::currentRenderTexture = NULL;
#endif
}
/*
================
idRenderTexture::AddRenderImage
================
*/
void idRenderTexture::AddRenderImage(idImage *image) {
	if (deviceHandle != -1) {
		common->FatalError("idRenderTexture::AddRenderImage: Can't add render image after FBO has been created!");
	}

	colorImages.Append(image);
}

/*
================
idRenderTexture::InitRenderTexture
================
*/
void idRenderTexture::InitRenderTexture(void) {
	glGenFramebuffers(1, &deviceHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, deviceHandle);

	bool isTexture3D = false;
	if ((colorImages.Num() > 0 && colorImages[0]->GetOpts().textureType == TT_CUBIC) || ((depthImage != nullptr) && depthImage->GetOpts().textureType == TT_CUBIC))
	{
		isTexture3D = true;
	}
	
	if (!isTexture3D)
	{
		for (int i = 0; i < colorImages.Num(); i++) {
			GLuint colorTexNum = colorImages[i]->GetTexNum();

			if (colorImages[i]->GetOpts().numMSAASamples == 0)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorTexNum, 0);
			}
			else
			{
#if !defined(__ANDROID__) //karin: GLES not support GL_TEXTURE_2D_MULTISAMPLE texture attachment
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, colorTexNum, 0);
#else
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorTexNum, 0);
#endif
			}
		}

		if (depthImage != nullptr) {
			GLuint depthImageTexNum = depthImage->GetTexNum();
			if (depthImage->GetOpts().numMSAASamples == 0)
			{
				if (depthImage->GetOpts().format == FMT_DEPTH || depthImage->GetOpts().format == FMT_DEPTH32) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthImageTexNum, 0);
				}
				else if (depthImage->GetOpts().format == FMT_DEPTH_STENCIL) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthImageTexNum, 0);
				}
				else {
					common->FatalError("idRenderTexture::InitRenderTexture: Unknown depth buffer format!");
				}
			}
			else
			{
				if (depthImage->GetOpts().format == FMT_DEPTH) {
#if !defined(__ANDROID__) //karin: GLES not support GL_TEXTURE_2D_MULTISAMPLE texture attachment
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthImageTexNum, 0);
#else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthImageTexNum, 0);
#endif
				}
				else if (depthImage->GetOpts().format == FMT_DEPTH_STENCIL) {
#if !defined(__ANDROID__) //karin: GLES not support GL_TEXTURE_2D_MULTISAMPLE texture attachment
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthImageTexNum, 0);
#else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthImageTexNum, 0);
#endif
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
#ifdef __ANDROID__xxx //karin: depth framebuffer not need draw
		else
			glDrawBuffers(0, NULL);
#endif
	}
	else
	{
		if (colorImages.Num() > 0)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, colorImages[0]->GetTexNum(), 0);
		}

		if (depthImage != nullptr) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, depthImage->GetTexNum(), 0);
		}
	}

	GLenum r = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (r != GL_FRAMEBUFFER_COMPLETE) {
		common->FatalError("idRenderTexture::InitRenderTexture: Failed to create rendertexture error code %d!", r);
	}
#if 1 //karin: print framebuffer info
	printf("Check framebuffer: %s\n", name.c_str());
	GLint value[4] = {0};
	for(int i = 0; i < 6; i++)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
		if(value[0] != GL_NONE)
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
		else
			value[1] = 0;
		printf("GL_COLOR_ATTACHMENT%d::OBJECT_TYPE -> %x %d\n", i, value[0], value[1]);
	}

	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
	if(value[0] != GL_NONE)
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
	else
		value[1] = 0;
	printf("GL_DEPTH_ATTACHMENT::OBJECT_TYPE -> %x %d\n", value[0], value[1]);

	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
	if(value[0] != GL_NONE)
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
	else
		value[1] = 0;
	printf("GL_STENCIL_ATTACHMENT::OBJECT_TYPE -> %x %d\n", value[0], value[1]);

	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
	if(value[0] != GL_NONE)
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
	else
		value[1] = 0;
	printf("GL_DEPTH_STENCIL_ATTACHMENT::OBJECT_TYPE -> %x %d\n", value[0], value[1]);
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
================
idRenderTexture::MakeCurrent
================
*/
void idRenderTexture::MakeCurrent(void) {
	glBindFramebuffer(GL_FRAMEBUFFER, deviceHandle);
#ifdef __ANDROID__ //karin: debug framebuffer
	idRenderTexture::currentRenderTexture = this;
    // PrintCurrentRenderTexture();
#endif
}

/*
================
idRenderTexture::BindNull
================
*/
void idRenderTexture::BindNull(void) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#ifdef __ANDROID__ //karin: debug framebuffer
	idRenderTexture::currentRenderTexture = NULL;
    // PrintCurrentRenderTexture();
#endif
}

/*
================
idRenderTexture::Resize
================
*/
void idRenderTexture::Resize(int width, int height) {
	idImage *target = nullptr;

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

	if (depthImage != nullptr) {
		depthImage->Resize(width, height);
	}

	if (deviceHandle != -1)
	{
		glDeleteFramebuffers(1, &deviceHandle);
		deviceHandle = -1;
	}

	InitRenderTexture();
}

#ifdef __ANDROID__ //karin: debug framebuffer
idRenderTexture * idRenderTexture::currentRenderTexture = NULL;

bool idRenderTexture::IsDefaultFramebufferActive(void)
{
    return currentRenderTexture == NULL;
}

void idRenderTexture::PrintCurrentRenderTexture(void)
{
	if(currentRenderTexture)
		printf("CURRENTFRAMEBUFFER: %s %d %d\n", currentRenderTexture->name.c_str(), currentRenderTexture->GetDeviceHandle(),
			   glIsFramebuffer(currentRenderTexture->GetDeviceHandle()));
	else
		printf("CURRENTFRAMEBUFFER: NULL DEFAULT 0\n");
}
#endif
