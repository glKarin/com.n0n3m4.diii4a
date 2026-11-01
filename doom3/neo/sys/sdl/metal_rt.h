// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>

// Native Metal bridge; all texture transfers stay on the GPU.
bool ETQW_MetalRT_Init(int width, int height, char *error, size_t errorSize);
bool ETQW_MetalRT_Build(const float *triangles, size_t vertexCount, char *error, size_t errorSize);
unsigned int ETQW_MetalRT_Trace(const float *camera, double *milliseconds);
void ETQW_MetalRT_Shutdown();

void R_MetalRT_Draw();
void R_MetalRT_ResetWorld();
void R_MetalRT_Shutdown();
class idCmdArgs;
void R_MetalRT_Info_f(const idCmdArgs &args);
