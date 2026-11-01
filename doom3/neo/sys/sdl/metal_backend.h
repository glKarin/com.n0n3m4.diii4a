// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
struct SDL_Window;
bool MetalBackend_Init(SDL_Window *window, int width, int height);
bool MetalBackend_Active();
void MetalBackend_SetCacheDirectory(const char *path);
void MetalBackend_Shutdown();
void *MetalBackend_Proc(const char *name);
void MetalBackend_Present(int interval);
void MetalBackend_DrawableSize(int *width, int *height);
void MetalBackend_Info();
// Native Metal ownership stays in Objective-C++; these are borrowed handles.
void *MetalBackend_Device();
void *MetalBackend_Queue();
void *MetalBackend_CommandBuffer();
void MetalBackend_EndEncoding();
unsigned MetalBackend_ImportTexture(void *texture);
void MetalBackend_Finish();
