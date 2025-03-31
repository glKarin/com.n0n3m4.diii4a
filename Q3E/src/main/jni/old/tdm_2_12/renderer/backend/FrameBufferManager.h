/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#pragma once

class FrameBuffer;

class FrameBufferManager
{
public:
	using Generator = std::function<void( FrameBuffer* )>;

	~FrameBufferManager();

	void Init();
	void Shutdown();

	FrameBuffer *CreateFromGenerator( const idStr &name, Generator generator );

	void PurgeAll();

	void BeginFrame();

	void EnterPrimary();
	void LeavePrimary(bool copyToDefault = true);
	void EnterShadowStencil();
	void LeaveShadowStencil();
	void ResolveShadowStencilAA();
	void EnterShadowMap();
	void LeaveShadowMap();

	void ResolvePrimary( GLbitfield mask = GL_COLOR_BUFFER_BIT, GLenum filter = GL_NEAREST );
	void UpdateCurrentRenderCopy();
	void UpdateCurrentDepthCopy();

	void CopyRender( const copyRenderCommand_t &cmd );

private:
	idList<FrameBuffer*> fbos;

	void UpdateResolutionAndFormats();
	void CreatePrimary(FrameBuffer *primary);
	void CreateResolve(FrameBuffer *resolve);
	void CreateGui(FrameBuffer *gui);

	void CopyRender( idImageScratch *image, int x, int y, int imageWidth, int imageHeight );
	void CopyRender( unsigned char *buffer, int x, int y, int imageWidth, int imageHeight, bool usePBO );
	bool EnsureScratchImageCreated( idImageScratch *image, int width, int height );
	GLuint pbo = 0;


	// TODO: this should be moved to a dedicated shadow stage
	void CreateStencilShadow(FrameBuffer *shadow);
	void CreateMapsShadow(FrameBuffer *shadow);
	int shadowAtlasSize = 0;
	bool depthCopiedThisView = false;
public:
	void GetShadowMapBudget( int &numAtlasTiles, int &tileSize );
	idList<renderCrop_t> CreateShadowMapPages( const idList<int> &ratios, int denominator );


public:
	int renderWidth = 0;
	int renderHeight = 0;
	GLenum colorFormat = 0;
	GLenum depthStencilFormat = 0;

	FrameBuffer *defaultFbo = nullptr;
	FrameBuffer *primaryFbo = nullptr;
	FrameBuffer *resolveFbo = nullptr;
	FrameBuffer *guiFbo = nullptr;	// source image for tonemap pass
	FrameBuffer *shadowStencilFbo = nullptr;
	FrameBuffer *shadowMapFbo = nullptr;

// public: // debug
	FrameBuffer *currentRenderFbo = nullptr;

	FrameBuffer *activeFbo = nullptr;
	FrameBuffer *activeDrawFbo = nullptr;
};

extern FrameBufferManager *frameBuffers;
