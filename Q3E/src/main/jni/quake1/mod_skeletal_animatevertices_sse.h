#ifndef MOD_SKELTAL_ANIMATEVERTICES_SSE_H
#define MOD_SKELTAL_ANIMATEVERTICES_SSE_H

#include "quakedef.h"

#ifdef SSE_POSSIBLE
void Mod_Skeletal_AnimateVertices_SSE(const model_t * RESTRICT model, const frameblend_t * RESTRICT frameblend, const skeleton_t *skeleton, float * RESTRICT vertex3f, float * RESTRICT normal3f, float * RESTRICT svector3f, float * RESTRICT tvector3f);
#endif

#endif
