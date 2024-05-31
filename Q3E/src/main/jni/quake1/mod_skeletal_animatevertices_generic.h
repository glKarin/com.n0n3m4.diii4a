#ifndef MOD_SKELETAL_ANIMATEVERTICES_GENERIC_H
#define MOD_H

#include "quakedef.h"

void Mod_Skeletal_AnimateVertices_Generic(const model_t * RESTRICT model, const frameblend_t * RESTRICT frameblend, const skeleton_t *skeleton, float * RESTRICT vertex3f, float * RESTRICT normal3f, float * RESTRICT svector3f, float * RESTRICT tvector3f);

#endif
