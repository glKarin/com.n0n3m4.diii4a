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

/**
 * Management class for a universal GPU buffer containing dynamic data that is
 * regularly updated every frame. Can be used for vertex/index data, UBOs,
 * multidraw commands, ...
 *
 * The buffer is divided into three frames - one frame for the GPU to draw with,
 * one for the CPU to write to and one "in-transit" for the GL driver, so that
 * we avoid synchronization between CPU and GPU in all but the rarest of cases.
 *
 * To make use of the buffer, get its current write address with `CurrentWriteLocation`
 * and the `BytesRemaining` in the current frame region and copy your data to it.
 * Once you are done writing your data and before using it in a GPU operation, you
 * need to `Commit` the range you wrote to. Bind the buffer or the write location's
 * range as needed.
 *
 * After a frame is completed, you need to call `SwitchFrame` to switch to the next
 * frame region and issue sync fences and waits to make sure no buffer region is
 * written to that is still in use by the GPU.
 */
class GpuBuffer {
public:
	void Init( GLenum type, GLuint size, GLuint alignment );

	/// Use this variant when the CPU prepares data one frame ahead (e.g. VertexCache)
	/// Otherwise, it is assumed that GPU draw calls are issued from the same region
	/// that was written to in this frame. That's the default for buffers filled and used
	/// in the backend.
	void InitWriteFrameAhead( GLenum type, GLuint size, GLuint alignment );
	void Destroy();

	byte *CurrentWriteLocation() const;
	GLuint BytesRemaining() const;

	void Commit( GLuint numBytes );
	
	void BindRangeToIndexTarget( GLuint index, byte *offset, GLuint size );
	void Bind();
	const void * BufferOffset( const void *pointer );

	void SwitchFrame();

	GLuint GetAPIObject() const { return bufferObject; }

	static const int NUM_FRAMES = 3;

private:
	GLsync frameFences[NUM_FRAMES] = { nullptr };
	GLenum type = GL_INVALID_ENUM;
	GLuint frameSize = 0;
	GLuint totalSize = 0;
	GLuint alignment = 0;
	bool usesPersistentMapping = false;
	GLuint bufferObject = 0;
	byte *bufferContents = nullptr;
	int currentDrawingFrame = 0;
	int currentWritingFrame = 0;
	GLuint bytesCommittedInCurrentFrame = 0;

	GLuint CurrentOffset() const;
};
