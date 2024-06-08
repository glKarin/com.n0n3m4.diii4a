// RenderLightCommitted.cpp
//

#include "RenderSystem_local.h"


/*
====================
R_TestPointInViewLight
====================
*/
// this needs to be greater than the dist from origin to corner of near clip plane
bool idRenderLightCommitted::TestPointInViewLight(const idVec3& org, const idRenderLightLocal* light) {
	int		i;
	idVec3	local;

	for (i = 0; i < 6; i++) {
		float d = light->frustum[i].Distance(org);
		if (d > INSIDE_LIGHT_FRUSTUM_SLOP) {
			return false;
		}
	}

	return true;
}


/*
======================
idRenderLightCommitted::ClippedLightScissorRectangle
======================
*/
idScreenRect idRenderLightCommitted::ClippedLightScissorRectangle(void) {
	int i, j;
	const idRenderLightLocal* light = lightDef;
	idScreenRect r;
	idFixedWinding w;

	r.Clear();

	for (i = 0; i < 6; i++) {
		const idWinding* ow = light->frustumWindings[i];

		// projected lights may have one of the frustums degenerated
		if (!ow) {
			continue;
		}

		// the light frustum planes face out from the light,
		// so the planes that have the view origin on the negative
		// side will be the "back" faces of the light, which must have
		// some fragment inside the portalStack to be visible
		if (light->frustum[i].Distance(tr.viewDef->renderView.vieworg) >= 0) {
			continue;
		}

		w = *ow;

		// now check the winding against each of the frustum planes
		for (j = 0; j < 5; j++) {
			if (!w.ClipInPlace(-tr.viewDef->frustum[j])) {
				break;
			}
		}

		// project these points to the screen and add to bounds
		for (j = 0; j < w.GetNumPoints(); j++) {
			idPlane		eye, clip;
			idVec3		ndc;

			R_TransformModelToClip(w[j].ToVec3(), tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip);

			if (clip[3] <= 0.01f) {
				clip[3] = 0.01f;
			}

			R_TransformClipToDevice(clip, tr.viewDef, ndc);

			float windowX = 0.5f * (1.0f + ndc[0]) * (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1);
			float windowY = 0.5f * (1.0f + ndc[1]) * (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1);

			if (windowX > tr.viewDef->scissor.x2) {
				windowX = tr.viewDef->scissor.x2;
			}
			else if (windowX < tr.viewDef->scissor.x1) {
				windowX = tr.viewDef->scissor.x1;
			}
			if (windowY > tr.viewDef->scissor.y2) {
				windowY = tr.viewDef->scissor.y2;
			}
			else if (windowY < tr.viewDef->scissor.y1) {
				windowY = tr.viewDef->scissor.y1;
			}

			r.AddPoint(windowX, windowY);
		}
	}

	// add the fudge boundary
	r.Expand();

	return r;
}

/*
==================
R_CalcLightScissorRectangle

The light screen bounds will be used to crop the scissor rect during
stencil clears and interaction drawing
==================
*/
int	c_clippedLight, c_unclippedLight;

idScreenRect	idRenderLightCommitted::CalcLightScissorRectangle(void) {
	idScreenRect	r;
	srfTriangles_t* tri;
	idPlane			eye, clip;
	idVec3			ndc;

	if (lightDef->parms.lightType == LIGHT_TYPE_POINT) {
		idBounds bounds;
		tr.viewDef->viewFrustum.ProjectionBounds(idBox(lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis), bounds);
		return R_ScreenRectFromViewFrustumBounds(bounds);
	}

	if (r_useClippedLightScissors.GetInteger() == 2) {
		return ClippedLightScissorRectangle();
	}

	r.Clear();

	tri = lightDef->frustumTris;
	for (int i = 0; i < tri->numVerts; i++) {
		R_TransformModelToClip(tri->verts[i].xyz, tr.viewDef->worldSpace.modelViewMatrix,
			tr.viewDef->projectionMatrix, eye, clip);

		// if it is near clipped, clip the winding polygons to the view frustum
		if (clip[3] <= 1) {
			c_clippedLight++;
			if (r_useClippedLightScissors.GetInteger()) {
				return ClippedLightScissorRectangle();
			}
			else {
				r.x1 = r.y1 = 0;
				r.x2 = (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1) - 1;
				r.y2 = (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1) - 1;
				return r;
			}
		}

		R_TransformClipToDevice(clip, tr.viewDef, ndc);

		float windowX = 0.5f * (1.0f + ndc[0]) * (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1);
		float windowY = 0.5f * (1.0f + ndc[1]) * (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1);

		if (windowX > tr.viewDef->scissor.x2) {
			windowX = tr.viewDef->scissor.x2;
		}
		else if (windowX < tr.viewDef->scissor.x1) {
			windowX = tr.viewDef->scissor.x1;
		}
		if (windowY > tr.viewDef->scissor.y2) {
			windowY = tr.viewDef->scissor.y2;
		}
		else if (windowY < tr.viewDef->scissor.y1) {
			windowY = tr.viewDef->scissor.y1;
		}

		r.AddPoint(windowX, windowY);
	}

	// add the fudge boundary
	r.Expand();

	c_unclippedLight++;

	return r;
}