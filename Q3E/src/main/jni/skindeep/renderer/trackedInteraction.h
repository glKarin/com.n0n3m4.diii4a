#ifndef __TRACKED_INTERACTION_H__
#define __TRACKED_INTERACTION_H__

#include "Image.h"
#include "Material.h"
#include "idlib/math/Vector.h"
#include "idlib/math/Matrix.h"

typedef struct trackedInteraction_s {
	int			numSurfaces;		// Number of surfaces in interaction (-1 means untested, 0 means no interaction)
	idVec3		lightOrigin;		// Light's origin
	idVec3		lightCenter;		// Where shadows are casted from (offset from origin)
	idMat3		lightAxis;			// Light's rotation angles
	idVec3		lightRadius;		// Radius along X/Y/Z axes. To conserve space, for spotlights this represents light_right/light_up/light_target respectively.
	bool		lightCastsShadows;	// Need to know this for occlusion testing
	bool		lightIsParallel;	// We need to do some funky stuff to account for parallel lights handle shadow origins
	bool		lightIsProjected;	// Is this a spotlight? (changes the info that gets fed into lightRadius)
	idImage*	lightFalloff;	// Map governing the light's falloff
	const idMaterial* lightShader;	// Shader being projected by the light
	float colour[3];					// RGB values, 0.0 to 1.0. Unspecified should be all 1.0s.
} trackedInteraction_t;

typedef void(*interactionCallback_t)(renderEntity_s*, idList<trackedInteraction_t>*);

#endif
