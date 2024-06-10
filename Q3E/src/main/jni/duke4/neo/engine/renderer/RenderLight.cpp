// RenderLight.cpp
//

#include "RenderSystem_local.h"

idRenderLightLocal::idRenderLightLocal() {
	memset(&parms, 0, sizeof(parms));
	memset(modelMatrix, 0, sizeof(modelMatrix));
	memset(shadowFrustums, 0, sizeof(shadowFrustums));
	memset(lightProject, 0, sizeof(lightProject));
	memset(frustum, 0, sizeof(frustum));
	memset(frustumWindings, 0, sizeof(frustumWindings));

	lightHasMoved = false;
	world = NULL;
	index = 0;
	areaNum = 0;
	lastModifiedFrameNum = 0;
	archived = false;
	lightShader = NULL;
	falloffImage = NULL;
	globalLightOrigin = vec3_zero;
	frustumTris = NULL;
	numShadowFrustums = 0;
	viewCount = 0;
	viewLight = NULL;
	references = NULL;
	foggedPortals = NULL;

	currentOcclusionQuery = nullptr;
	visibleFrame = 0;
	lightRendered = false;
}


/*
=================
R_CreateLightRefs
=================
*/
#define	MAX_LIGHT_VERTS	40
void idRenderLightLocal::CreateLightRefs(void) {
	idVec3	points[MAX_LIGHT_VERTS];
	int		i;
	srfTriangles_t* tri;

	tri = frustumTris;

	// because a light frustum is made of only six intersecting planes,
	// we should never be able to get a stupid number of points...
	if (tri->numVerts > MAX_LIGHT_VERTS) {
		common->Error("R_CreateLightRefs: %i points in frustumTris!", tri->numVerts);
	}
	for (i = 0; i < tri->numVerts; i++) {
		points[i] = tri->verts[i].xyz;
	}

	if (r_showUpdates.GetBool() && (tri->bounds[1][0] - tri->bounds[0][0] > 1024 ||
		tri->bounds[1][1] - tri->bounds[0][1] > 1024)) {
		common->Printf("big lightRef: %f,%f\n", tri->bounds[1][0] - tri->bounds[0][0]
			, tri->bounds[1][1] - tri->bounds[0][1]);
	}

	// determine the areaNum for the light origin, which may let us
	// cull the light if it is behind a closed door
	// it is debatable if we want to use the entity origin or the center offset origin,
	// but we definitely don't want to use a parallel offset origin
	areaNum = world->PointInArea(globalLightOrigin);
	if (areaNum == -1) {
		areaNum = world->PointInArea(parms.origin);
	}

	// bump the view count so we can tell if an
	// area already has a reference
	tr.viewCount++;
}


/*
========================
R_ComputePointLightProjectionMatrix

Computes the light projection matrix for a point light.
========================
*/
static float R_ComputePointLightProjectionMatrix(idRenderLightLocal* light, idRenderMatrix& localProject) {
	//assert(light->parms.pointLight);

	// A point light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / light->parms.lightRadius[0];
	localProject[1][1] = 0.5f / light->parms.lightRadius[1];
	localProject[2][2] = 0.5f / light->parms.lightRadius[2];
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	return 1.0f;
}

static const float SPOT_LIGHT_MIN_Z_NEAR = 8.0f;
static const float SPOT_LIGHT_MIN_Z_FAR = 16.0f;

/*
========================
R_ComputeSpotLightProjectionMatrix

Computes the light projection matrix for a spot light.
========================
*/
static float R_ComputeSpotLightProjectionMatrix(idRenderLightLocal* light, idRenderMatrix& localProject) {
	const float targetDistSqr = light->parms.target.LengthSqr();
	const float invTargetDist = idMath::InvSqrt(targetDistSqr);
	const float targetDist = invTargetDist * targetDistSqr;

	const idVec3 normalizedTarget = light->parms.target * invTargetDist;
	const idVec3 normalizedRight = light->parms.right * (0.5f * targetDist / light->parms.right.LengthSqr());
	const idVec3 normalizedUp = light->parms.up * (-0.5f * targetDist / light->parms.up.LengthSqr());

	localProject[0][0] = normalizedRight[0];
	localProject[0][1] = normalizedRight[1];
	localProject[0][2] = normalizedRight[2];
	localProject[0][3] = 0.0f;

	localProject[1][0] = normalizedUp[0];
	localProject[1][1] = normalizedUp[1];
	localProject[1][2] = normalizedUp[2];
	localProject[1][3] = 0.0f;

	localProject[3][0] = normalizedTarget[0];
	localProject[3][1] = normalizedTarget[1];
	localProject[3][2] = normalizedTarget[2];
	localProject[3][3] = 0.0f;

	// Set the falloff vector.
	// This is similar to the Z calculation for depth buffering, which means that the
	// mapped texture is going to be perspective distorted heavily towards the zero end.
	const float zNear = Max(light->parms.start * normalizedTarget, SPOT_LIGHT_MIN_Z_NEAR);
	const float zFar = Max(light->parms.end * normalizedTarget, SPOT_LIGHT_MIN_Z_FAR);
	const float zScale = (zNear + zFar) / zFar;

	localProject[2][0] = normalizedTarget[0] * zScale;
	localProject[2][1] = normalizedTarget[1] * zScale;
	localProject[2][2] = normalizedTarget[2] * zScale;
	localProject[2][3] = -zNear * zScale;

	// now offset to the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range
	idVec4 projectedTarget;
	localProject.TransformPoint(light->parms.target, projectedTarget);

	const float ofs0 = 0.5f - projectedTarget[0] / projectedTarget[3];
	localProject[0][0] += ofs0 * localProject[3][0];
	localProject[0][1] += ofs0 * localProject[3][1];
	localProject[0][2] += ofs0 * localProject[3][2];
	localProject[0][3] += ofs0 * localProject[3][3];

	const float ofs1 = 0.5f - projectedTarget[1] / projectedTarget[3];
	localProject[1][0] += ofs1 * localProject[3][0];
	localProject[1][1] += ofs1 * localProject[3][1];
	localProject[1][2] += ofs1 * localProject[3][2];
	localProject[1][3] += ofs1 * localProject[3][3];

	return 1.0f / (zNear + zFar);
}

/*
========================
R_ComputeParallelLightProjectionMatrix

Computes the light projection matrix for a parallel light.
========================
*/
static float R_ComputeParallelLightProjectionMatrix(idRenderLightLocal* light, idRenderMatrix& localProject) {
	//assert(light->parms.parallel);

	// A parallel light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / light->parms.lightRadius[0];
	localProject[1][1] = 0.5f / light->parms.lightRadius[1];
	localProject[2][2] = 0.5f / light->parms.lightRadius[2];
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	return 1.0f;
}

/*
=================
R_DeriveLightData

Fills everything in based on parms
=================
*/
void idRenderLightLocal::DeriveLightData(void) {
	int i;

	// decide which light shader we are going to use
	if (parms.shader) {
		lightShader = parms.shader;
	}
	if (!lightShader) {
		if (parms.lightType == LIGHT_TYPE_POINT) {
			lightShader = declManager->FindMaterial("lights/defaultPointLight");
		}
		else {
			lightShader = declManager->FindMaterial("lights/defaultProjectedLight");
		}
	}

	// get the falloff image
	falloffImage = lightShader->LightFalloffImage();
	if (!falloffImage) {
		// use the falloff from the default shader of the correct type
		const idMaterial* defaultShader;

		if (parms.lightType == LIGHT_TYPE_POINT) {
			defaultShader = declManager->FindMaterial("lights/defaultPointLight");
			falloffImage = defaultShader->LightFalloffImage();
		}
		else {
			// projected lights by default don't diminish with distance
			defaultShader = declManager->FindMaterial("lights/defaultProjectedLight");
			falloffImage = defaultShader->LightFalloffImage();
		}
	}

	// set the projection
	if (parms.lightType == LIGHT_TYPE_SPOTLIGHT || parms.lightType == LIGHT_TYPE_PARALLEL) {
		// projected light

		SetLightProject(lightProject, vec3_origin /* parms.origin */, parms.target,
			parms.right, parms.up, parms.start, parms.end);
	}
	else {
		// point light
		memset(lightProject, 0, sizeof(lightProject));
		lightProject[0][0] = 0.5f / parms.lightRadius[0];
		lightProject[1][1] = 0.5f / parms.lightRadius[1];
		lightProject[3][2] = 0.5f / parms.lightRadius[2];
		lightProject[0][3] = 0.5f;
		lightProject[1][3] = 0.5f;
		lightProject[2][3] = 1.0f;
		lightProject[3][3] = 0.5f;
	}

	// set the frustum planes
	SetLightFrustum(lightProject, frustum);

	// rotate the light planes and projections by the axis
	R_AxisToModelMatrix(parms.axis, parms.origin, modelMatrix);

	for (i = 0; i < 6; i++) {
		idPlane		temp;
		temp = frustum[i];
		R_LocalPlaneToGlobal(modelMatrix, temp, frustum[i]);
	}
	for (i = 0; i < 4; i++) {
		idPlane		temp;
		temp = lightProject[i];
		R_LocalPlaneToGlobal(modelMatrix, temp, lightProject[i]);
	}

	// adjust global light origin for off center projections and parallel projections
	// we are just faking parallel by making it a very far off center for now
	if (parms.lightType == LIGHT_TYPE_PARALLEL) {
		idVec3	dir;

		dir = parms.lightCenter;
		if (!dir.Normalize()) {
			// make point straight up if not specified
			dir[2] = 1;
		}
		globalLightOrigin = parms.origin + dir * 100000;
	}
	else {
		globalLightOrigin = parms.origin + parms.axis * parms.lightCenter;
	}

	FreeLightDefFrustum();

	frustumTris = R_PolytopeSurface(6, frustum, frustumWindings);

	// a projected light will have one shadowFrustum, a point light will have
	// six unless the light center is outside the box
	MakeShadowFrustums();

	idRenderMatrix localProject;
	float zScale = 1.0f;
	switch (parms.lightType)
	{
		case LIGHT_TYPE_POINT:
			zScale = R_ComputePointLightProjectionMatrix(this, localProject);
			break;

		case LIGHT_TYPE_SPOTLIGHT:
			zScale = R_ComputeSpotLightProjectionMatrix(this, localProject);
			break;

		case LIGHT_TYPE_PARALLEL:
			zScale = R_ComputeParallelLightProjectionMatrix(this, localProject);
			break;
	}

	// Rotate and translate the light projection by the light matrix.
	// 99% of lights remain axis aligned in world space.
	idRenderMatrix lightMatrix;
	idRenderMatrix::CreateFromOriginAxis(parms.origin, parms.axis, lightMatrix);

	idRenderMatrix inverseLightMatrix;
	if (!idRenderMatrix::Inverse(lightMatrix, inverseLightMatrix)) {
		idLib::Warning("lightMatrix invert failed");
	}

	// 'baseLightProject' goes from global space -> light local space -> light projective space
	idRenderMatrix::Multiply(localProject, inverseLightMatrix, baseLightProject);

	// Invert the light projection so we can deform zero-to-one cubes into
	// the light model and calculate global bounds.
	if (!idRenderMatrix::Inverse(baseLightProject, inverseBaseLightProject)) {
		idLib::Warning("baseLightProject invert failed");
	}

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idRenderMatrix::ProjectedBounds(globalLightBounds, inverseBaseLightProject, bounds_zeroOneCube, false);
}

/*
====================
idRenderLightLocal::FreeLightDefFrustum
====================
*/
void idRenderLightLocal::FreeLightDefFrustum(void) {
	int i;

	// free the frustum tris
	if (frustumTris) {
		R_FreeStaticTriSurf(frustumTris);
		frustumTris = NULL;
	}
	// free frustum windings
	for (i = 0; i < 6; i++) {
		if (frustumWindings[i]) {
			delete frustumWindings[i];
			frustumWindings[i] = NULL;
		}
	}
}


/*
===================
R_MakeShadowFrustums

Called at definition derivation time
===================
*/
void idRenderLightLocal::MakeShadowFrustums(void) {
	int		i, j;

	if (parms.lightType == LIGHT_TYPE_POINT) {
#if 0
		idVec3	adjustedRadius;

		// increase the light radius to cover any origin offsets.
		// this will cause some shadows to extend out of the exact light
		// volume, but is simpler than adjusting all the frustums
		adjustedRadius[0] = parms.lightRadius[0] + idMath::Fabs(parms.lightCenter[0]);
		adjustedRadius[1] = parms.lightRadius[1] + idMath::Fabs(parms.lightCenter[1]);
		adjustedRadius[2] = parms.lightRadius[2] + idMath::Fabs(parms.lightCenter[2]);

		numShadowFrustums = 0;
		// a point light has to project against six planes
		for (i = 0; i < 6; i++) {
			shadowFrustum_t* frust = &shadowFrustums[numShadowFrustums];

			frust->numPlanes = 6;
			frust->makeClippedPlanes = false;
			for (j = 0; j < 6; j++) {
				idPlane& plane = frust->planes[j];
				plane[0] = pointLightFrustums[i][j][0] / adjustedRadius[0];
				plane[1] = pointLightFrustums[i][j][1] / adjustedRadius[1];
				plane[2] = pointLightFrustums[i][j][2] / adjustedRadius[2];
				plane.Normalize();
				plane[3] = -(plane.Normal() * globalLightOrigin);
				if (j == 5) {
					plane[3] += adjustedRadius[i >> 1];
				}
			}

			numShadowFrustums++;
		}
#else
		// exact projection,taking into account asymetric frustums when
		// globalLightOrigin isn't centered

		static int	faceCorners[6][4] = {
			{ 7, 5, 1, 3 },		// positive X side
			{ 4, 6, 2, 0 },		// negative X side
			{ 6, 7, 3, 2 },		// positive Y side
			{ 5, 4, 0, 1 },		// negative Y side
			{ 6, 4, 5, 7 },		// positive Z side
			{ 3, 1, 0, 2 }		// negative Z side
		};
		static int	faceEdgeAdjacent[6][4] = {
			{ 4, 4, 2, 2 },		// positive X side
			{ 7, 7, 1, 1 },		// negative X side
			{ 5, 5, 0, 0 },		// positive Y side
			{ 6, 6, 3, 3 },		// negative Y side
			{ 0, 0, 3, 3 },		// positive Z side
			{ 5, 5, 6, 6 }		// negative Z side
		};

		bool	centerOutside = false;

		// if the light center of projection is outside the light bounds,
		// we will need to build the planes a little differently
		if (fabs(parms.lightCenter[0]) > parms.lightRadius[0]
			|| fabs(parms.lightCenter[1]) > parms.lightRadius[1]
			|| fabs(parms.lightCenter[2]) > parms.lightRadius[2]) {
			centerOutside = true;
		}

		// make the corners
		idVec3	corners[8];

		for (i = 0; i < 8; i++) {
			idVec3	temp;
			for (j = 0; j < 3; j++) {
				if (i & (1 << j)) {
					temp[j] = parms.lightRadius[j];
				}
				else {
					temp[j] = -parms.lightRadius[j];
				}
			}

			// transform to global space
			corners[i] = parms.origin + parms.axis * temp;
		}

		numShadowFrustums = 0;
		for (int side = 0; side < 6; side++) {
			shadowFrustum_t* frust = &shadowFrustums[numShadowFrustums];
			idVec3& p1 = corners[faceCorners[side][0]];
			idVec3& p2 = corners[faceCorners[side][1]];
			idVec3& p3 = corners[faceCorners[side][2]];
			idPlane backPlane;

			// plane will have positive side inward
			backPlane.FromPoints(p1, p2, p3);

			// if center of projection is on the wrong side, skip
			float d = backPlane.Distance(globalLightOrigin);
			if (d < 0) {
				continue;
			}

			frust->numPlanes = 6;
			frust->planes[5] = backPlane;
			frust->planes[4] = backPlane;	// we don't really need the extra plane

			// make planes with positive side facing inwards in light local coordinates
			for (int edge = 0; edge < 4; edge++) {
				idVec3& p1 = corners[faceCorners[side][edge]];
				idVec3& p2 = corners[faceCorners[side][(edge + 1) & 3]];

				// create a plane that goes through the center of projection
				frust->planes[edge].FromPoints(p2, p1, globalLightOrigin);

				// see if we should use an adjacent plane instead
				if (centerOutside) {
					idVec3& p3 = corners[faceEdgeAdjacent[side][edge]];
					idPlane sidePlane;

					sidePlane.FromPoints(p2, p1, p3);
					d = sidePlane.Distance(globalLightOrigin);
					if (d < 0) {
						// use this plane instead of the edged plane
						frust->planes[edge] = sidePlane;
					}
					// we can't guarantee a neighbor, so add sill planes at edge
					shadowFrustums[numShadowFrustums].makeClippedPlanes = true;
				}
			}
			numShadowFrustums++;
		}

#endif
		return;
	}

	// projected light

	numShadowFrustums = 1;
	shadowFrustum_t* frust = &shadowFrustums[0];

	// flip and transform the frustum planes so the positive side faces
	// inward in local coordinates

	// it is important to clip against even the near clip plane, because
	// many projected lights that are faking area lights will have their
	// origin behind solid surfaces.
	for (i = 0; i < 6; i++) {
		idPlane& plane = frust->planes[i];

		plane.SetNormal(-frustum[i].Normal());
		plane.SetDist(-frustum[i].Dist());
	}

	frust->numPlanes = 6;

	frust->makeClippedPlanes = true;
	// projected lights don't have shared frustums, so any clipped edges
	// right on the planes must have a sil plane created for them
}


/*
===================
R_SetLightFrustum

Creates plane equations from the light projection, positive sides
face out of the light
===================
*/
void idRenderLightLocal::SetLightFrustum(const idPlane lightProject[4], idPlane frustum[6]) {
	int		i;

	// we want the planes of s=0, s=q, t=0, and t=q
	frustum[0] = lightProject[0];
	frustum[1] = lightProject[1];
	frustum[2] = lightProject[2] - lightProject[0];
	frustum[3] = lightProject[2] - lightProject[1];

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	frustum[4] = lightProject[3];

	frustum[5] = lightProject[3];
	frustum[5][3] -= 1.0f;
	frustum[5] = -frustum[5];

	for (i = 0; i < 6; i++) {
		float	l;

		frustum[i] = -frustum[i];
		l = frustum[i].Normalize();
		frustum[i][3] /= l;
	}
}


/*
=====================
R_SetLightProject

All values are reletive to the origin
Assumes that right and up are not normalized
This is also called by dmap during map processing.
=====================
*/
void idRenderLightLocal::SetLightProject(idPlane lightProject[4], const idVec3 origin, const idVec3 target, const idVec3 rightVector, const idVec3 upVector, const idVec3 start, const idVec3 stop) {
	float		dist;
	float		scale;
	float		rLen, uLen;
	idVec3		normal;
	float		ofs;
	idVec3		right, up;
	idVec3		startGlobal;
	idVec4		targetGlobal;

	right = rightVector;
	rLen = right.Normalize();
	up = upVector;
	uLen = up.Normalize();
	normal = up.Cross(right);
	//normal = right.Cross( up );
	normal.Normalize();

	dist = target * normal; //  - ( origin * normal );
	if (dist < 0) {
		dist = -dist;
		normal = -normal;
	}

	scale = (0.5f * dist) / rLen;
	right *= scale;
	scale = -(0.5f * dist) / uLen;
	up *= scale;

	lightProject[2] = normal;
	lightProject[2][3] = -(origin * lightProject[2].Normal());

	lightProject[0] = right;
	lightProject[0][3] = -(origin * lightProject[0].Normal());

	lightProject[1] = up;
	lightProject[1][3] = -(origin * lightProject[1].Normal());

	// now offset to center
	targetGlobal.ToVec3() = target + origin;
	targetGlobal[3] = 1;
	ofs = 0.5f - (targetGlobal * lightProject[0].ToVec4()) / (targetGlobal * lightProject[2].ToVec4());
	lightProject[0].ToVec4() += ofs * lightProject[2].ToVec4();
	ofs = 0.5f - (targetGlobal * lightProject[1].ToVec4()) / (targetGlobal * lightProject[2].ToVec4());
	lightProject[1].ToVec4() += ofs * lightProject[2].ToVec4();

	// set the falloff vector
	normal = stop - start;
	dist = normal.Normalize();
	if (dist <= 0) {
		dist = 1;
	}
	lightProject[3] = normal * (1.0f / dist);
	startGlobal = start + origin;
	lightProject[3][3] = -(startGlobal * lightProject[3].Normal());
}


/*
====================
R_FreeLightDefDerivedData

Frees all references and lit surfaces from the light
====================
*/
void idRenderLightLocal::FreeLightDefDerivedData(void) {
	areaReference_t* lref, * nextRef;

	// rmove any portal fog references
	for (doublePortal_t* dp = foggedPortals; dp; dp = dp->nextFoggedPortal) {
		dp->fogLight = NULL;
	}

	// free all the references to the light
	for (lref = references; lref; lref = nextRef) {
		nextRef = lref->ownerNext;

		// unlink from the area
		lref->areaNext->areaPrev = lref->areaPrev;
		lref->areaPrev->areaNext = lref->areaNext;
	}
	references = NULL;

	FreeLightDefFrustum();
}

void idRenderLightLocal::FreeRenderLight() {
}
void idRenderLightLocal::UpdateRenderLight(const renderLight_t* re, bool forceUpdate) {
}
void idRenderLightLocal::GetRenderLight(renderLight_t* re) {
}
void idRenderLightLocal::ForceUpdate() {
}
int idRenderLightLocal::GetIndex() {
	return index;
}

bool idRenderLightLocal::IsVisible() { 
	return visibleFrame + 60 < tr.frameCount; 
}

/*
===============
R_RenderLightFrustum

Called by the editor and dmap to operate on light volumes
===============
*/
void idRenderSystemLocal::RenderLightFrustum(const renderLight_t& renderLight, idPlane lightFrustum[6]) {
	idRenderLightLocal	fakeLight;

	memset(&fakeLight, 0, sizeof(fakeLight));
	fakeLight.parms = renderLight;

	fakeLight.DeriveLightData();

	R_FreeStaticTriSurf(fakeLight.frustumTris);

	for (int i = 0; i < 6; i++) {
		lightFrustum[i] = fakeLight.frustum[i];
	}
}