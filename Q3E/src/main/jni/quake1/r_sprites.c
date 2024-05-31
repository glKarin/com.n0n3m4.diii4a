
#include "quakedef.h"
#include "r_shadow.h"

extern cvar_t r_labelsprites_scale;
extern cvar_t r_labelsprites_roundtopixels;
extern cvar_t r_track_sprites;
extern cvar_t r_track_sprites_flags;
extern cvar_t r_track_sprites_scalew;
extern cvar_t r_track_sprites_scaleh;
extern cvar_t r_overheadsprites_perspective;
extern cvar_t r_overheadsprites_pushback;
extern cvar_t r_overheadsprites_scalex;
extern cvar_t r_overheadsprites_scaley;

#define TSF_ROTATE 1
#define TSF_ROTATE_CONTINOUSLY 2

// use same epsilon as in sv_phys.c, it's not in any header, that's why i redefine it
// MIN_EPSILON is for accurateness' sake :)
#ifndef EPSILON
# define EPSILON (1.0f / 32.0f)
# define MIN_EPSILON 0.0001f
#endif

/* R_Track_Sprite
   If the sprite is out of view, track it.
   `origin`, `left` and `up` are changed by this function to achive a rotation around
   the hotspot.
   
   --blub
 */
#define SIDE_TOP 1
#define SIDE_LEFT 2
#define SIDE_BOTTOM 3
#define SIDE_RIGHT 4

static void R_TrackSprite(const entity_render_t *ent, vec3_t origin, vec3_t left, vec3_t up, int *edge, float *dir_angle)
{
	float distance;
	vec3_t bCoord; // body coordinates of object
	unsigned int i;

	// temporarily abuse bCoord as the vector player->sprite-origin
	VectorSubtract(origin, r_refdef.view.origin, bCoord);
	distance = VectorLength(bCoord);

	// Now get the bCoords :)
	Matrix4x4_Transform(&r_refdef.view.inverse_matrix, origin, bCoord);

	*edge = 0; // FIXME::should assume edge == 0, which is correct currently
	for(i = 0; i < 4; ++i)
	{
		if(PlaneDiff(origin, &r_refdef.view.frustum[i]) < -EPSILON)
			break;
	}

	// If it wasn't outside a plane, no tracking needed
	if(i < 4)
	{
		float x, y;    // screen X and Y coordinates
		float ax, ay;  // absolute coords, used for division
		// I divide x and y by the greater absolute value to get ranges -1.0 to +1.0
		
		bCoord[2] *= r_refdef.view.frustum_x;
		bCoord[1] *= r_refdef.view.frustum_y;

		//Con_Printf("%f %f %f\n", bCoord[0], bCoord[1], bCoord[2]);
		
		ax = fabs(bCoord[1]);
		ay = fabs(bCoord[2]);
		// get the greater value and determine the screen edge it's on
		if(ax < ay)
		{
			ax = ay;
			// 180 or 0 degrees
			if(bCoord[2] < 0.0f)
				*edge = SIDE_BOTTOM;
			else
				*edge = SIDE_TOP;
		} else {
			if(bCoord[1] < 0.0f)
				*edge = SIDE_RIGHT;
			else
				*edge = SIDE_LEFT;
		}
		
		// umm... 
		if(ax < MIN_EPSILON) // this was == 0.0f before --blub
			ax = MIN_EPSILON;
		// get the -1 to +1 range
		x = bCoord[1] / ax;
		y = bCoord[2] / ax;

		ax = (1.0f / VectorLength(left));
		ay = (1.0f / VectorLength(up));
		// Using the placement below the distance of a sprite is
		// real dist = sqrt(d*d + dfxa*dfxa + dgyb*dgyb)
		// d is the distance we use
		// f is frustum X
		// x is x
		// a is ax
		// g is frustum Y
		// y is y
		// b is ay
		
		// real dist (r) shall be d, so
		// r*r = d*d + dfxa*dfxa + dgyb*dgyb
		// r*r = d*d * (1 + fxa*fxa + gyb*gyb)
		// d*d = r*r / (1 + fxa*fxa + gyb*gyb)
		// d = sqrt(r*r / (1 + fxa*fxa + gyb*gyb))
		// thus:
		distance = sqrt((distance*distance) / (1.0 +
					r_refdef.view.frustum_x*r_refdef.view.frustum_x * x*x * ax*ax +
					r_refdef.view.frustum_y*r_refdef.view.frustum_y * y*y * ay*ay));
		// ^ the one we want        ^ the one we have       ^ our factors
		
		// Place the sprite a few units ahead of the player
		VectorCopy(r_refdef.view.origin, origin);
		VectorMA(origin, distance, r_refdef.view.forward, origin);
		// Move the sprite left / up the screeen height
		VectorMA(origin, distance * r_refdef.view.frustum_x * x * ax, left, origin);
		VectorMA(origin, distance * r_refdef.view.frustum_y * y * ay, up, origin);

		if(r_track_sprites_flags.integer & TSF_ROTATE_CONTINOUSLY)
		{
			// compute the rotation, negate y axis, we're pointing outwards
			*dir_angle = atan(-y / x) * 180.0f/M_PI;
			// we need the real, full angle
			if(x < 0.0f)
				*dir_angle += 180.0f;
		}

		left[0] *= r_track_sprites_scalew.value;
		left[1] *= r_track_sprites_scalew.value;
		left[2] *= r_track_sprites_scalew.value;

		up[0] *= r_track_sprites_scaleh.value;
		up[1] *= r_track_sprites_scaleh.value;
		up[2] *= r_track_sprites_scaleh.value;
	}
}

static void R_RotateSprite(const mspriteframe_t *frame, vec3_t origin, vec3_t left, vec3_t up, int edge, float dir_angle)
{
	if(!(r_track_sprites_flags.integer & TSF_ROTATE))
	{
		// move down by its size if on top, otherwise it's invisible
		if(edge == SIDE_TOP)
			VectorMA(origin, -(fabs(frame->up)+fabs(frame->down)), up, origin);
	} else {
		static float rotation_angles[5] =
		{
			0, // no edge
			-90.0f,	//top
			0.0f,	// left
			90.0f,	// bottom
			180.0f,	// right
		};
		
		// rotate around the hotspot according to which edge it's on
		// since the hotspot == the origin, only rotate the vectors
		matrix4x4_t rotm;
		vec3_t axis;
		vec3_t temp;
		vec2_t dir;
		float angle;

		if(edge < 1 || edge > 4)
			return; // this usually means something went wrong somewhere, there's no way to get a wrong edge value currently
		
		dir[0] = frame->right + frame->left;
		dir[1] = frame->down + frame->up;

		// only rotate when the hotspot isn't the center though.
		if(dir[0] < MIN_EPSILON && dir[1] < MIN_EPSILON)
		{
			return;
		}

		// Now that we've kicked center-hotspotted sprites, rotate using the appropriate matrix :)

		// determine the angle of a sprite, we could only do that once though and
		// add a `qbool initialized' to the mspriteframe_t struct... let's get the direction vector of it :)

		angle = atan(dir[1] / dir[0]) * 180.0f/M_PI;

		// we need the real, full angle
		if(dir[0] < 0.0f)
			angle += 180.0f;

		// Rotate around rotation_angle - frame_angle
		// The axis SHOULD equal r_refdef.view.forward, but let's generalize this:
		CrossProduct(up, left, axis);
		if(r_track_sprites_flags.integer & TSF_ROTATE_CONTINOUSLY)
			Matrix4x4_CreateRotate(&rotm, dir_angle - angle, axis[0], axis[1], axis[2]);
		else
			Matrix4x4_CreateRotate(&rotm, rotation_angles[edge] - angle, axis[0], axis[1], axis[2]);
		Matrix4x4_Transform(&rotm, up, temp);
		VectorCopy(temp, up);
		Matrix4x4_Transform(&rotm, left, temp);
		VectorCopy(temp, left);
	}
}

static float spritetexcoord2f[4*2] = {0, 1, 0, 0, 1, 0, 1, 1};

static void R_Model_Sprite_Draw_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int i;
	model_t *model = ent->model;
	vec3_t left, up, org, mforward, mleft, mup, middle;
	float scale, dx, dy, hud_vs_screen;
	int edge = 0;
	float dir_angle = 0.0f;
	float vertex3f[12];

	// nudge it toward the view to make sure it isn't in a wall
	Matrix4x4_ToVectors(&ent->matrix, mforward, mleft, mup, org);
	VectorSubtract(org, r_refdef.view.forward, org);
	switch(model->sprite.sprnum_type)
	{
	case SPR_VP_PARALLEL_UPRIGHT:
		// flames and such
		// vertical beam sprite, faces view plane
		scale = ent->scale / sqrt(r_refdef.view.forward[0]*r_refdef.view.forward[0]+r_refdef.view.forward[1]*r_refdef.view.forward[1]);
		left[0] = -r_refdef.view.forward[1] * scale;
		left[1] = r_refdef.view.forward[0] * scale;
		left[2] = 0;
		up[0] = 0;
		up[1] = 0;
		up[2] = ent->scale;
		break;
	case SPR_FACING_UPRIGHT:
		// flames and such
		// vertical beam sprite, faces viewer's origin (not the view plane)
		scale = ent->scale / sqrt((org[0] - r_refdef.view.origin[0])*(org[0] - r_refdef.view.origin[0])+(org[1] - r_refdef.view.origin[1])*(org[1] - r_refdef.view.origin[1]));
		left[0] = (org[1] - r_refdef.view.origin[1]) * scale;
		left[1] = -(org[0] - r_refdef.view.origin[0]) * scale;
		left[2] = 0;
		up[0] = 0;
		up[1] = 0;
		up[2] = ent->scale;
		break;
	default:
		Con_Printf("R_SpriteSetup: unknown sprite type %i\n", model->sprite.sprnum_type);
		// fall through to normal sprite
	case SPR_VP_PARALLEL:
		// normal sprite
		// faces view plane
		VectorScale(r_refdef.view.left, ent->scale, left);
		VectorScale(r_refdef.view.up, ent->scale, up);
		break;
	case SPR_LABEL_SCALE:
		// normal sprite
		// faces view plane
		// fixed HUD pixel size specified in sprite
		// honors scale
		// honors a global label scaling cvar
	
		if(r_fb.water.renderingscene) // labels are considered HUD items, and don't appear in reflections
			return;

		// See the R_TrackSprite definition for a reason for this copying
		VectorCopy(r_refdef.view.left, left);
		VectorCopy(r_refdef.view.up, up);
		// It has to be done before the calculations, because it moves the origin.
		if(r_track_sprites.integer)
			R_TrackSprite(ent, org, left, up, &edge, &dir_angle);
		
		scale = 2 * ent->scale * (DotProduct(r_refdef.view.forward, org) - DotProduct(r_refdef.view.forward, r_refdef.view.origin)) * r_labelsprites_scale.value;
		VectorScale(left, scale * r_refdef.view.frustum_x / vid_conwidth.integer, left); // 1px
		VectorScale(up, scale * r_refdef.view.frustum_y / vid_conheight.integer, up); // 1px
		break;
	case SPR_LABEL:
		// normal sprite
		// faces view plane
		// fixed pixel size specified in sprite
		// tries to get the right size in HUD units, if possible
		// ignores scale
		// honors a global label scaling cvar before the rounding
		// FIXME assumes that 1qu is 1 pixel in the sprite like in SPR32 format. Should not do that, but instead query the source image! This bug only applies to the roundtopixels case, though.

		if(r_fb.water.renderingscene) // labels are considered HUD items, and don't appear in reflections
			return;

		// See the R_TrackSprite definition for a reason for this copying
		VectorCopy(r_refdef.view.left, left);
		VectorCopy(r_refdef.view.up, up);
		// It has to be done before the calculations, because it moves the origin.
		if(r_track_sprites.integer)
			R_TrackSprite(ent, org, left, up, &edge, &dir_angle);
		
		scale = 2 * (DotProduct(r_refdef.view.forward, org) - DotProduct(r_refdef.view.forward, r_refdef.view.origin));

		if(r_labelsprites_roundtopixels.integer)
		{
			hud_vs_screen = max(
				vid_conwidth.integer / (float) r_refdef.view.width,
				vid_conheight.integer / (float) r_refdef.view.height
			) / max(0.125, r_labelsprites_scale.value);

			// snap to "good sizes"
			// 1     for (0.6, 1.41]
			// 2     for (1.8, 3.33]
			if(hud_vs_screen <= 0.6)
				hud_vs_screen = 0; // don't, use real HUD pixels
			else if(hud_vs_screen <= 1.41)
				hud_vs_screen = 1;
			else if(hud_vs_screen <= 3.33)
				hud_vs_screen = 2;
			else
				hud_vs_screen = 0; // don't, use real HUD pixels

			if(hud_vs_screen)
			{
				// use screen pixels
				VectorScale(left, scale * r_refdef.view.frustum_x / (r_refdef.view.width * hud_vs_screen), left); // 1px
				VectorScale(up, scale * r_refdef.view.frustum_y / (r_refdef.view.height * hud_vs_screen), up); // 1px
			}
			else
			{
				// use HUD pixels
				VectorScale(left, scale * r_refdef.view.frustum_x / vid_conwidth.integer * r_labelsprites_scale.value, left); // 1px
				VectorScale(up, scale * r_refdef.view.frustum_y / vid_conheight.integer * r_labelsprites_scale.value, up); // 1px
			}

			if(hud_vs_screen == 1)
			{
				VectorMA(r_refdef.view.origin, scale, r_refdef.view.forward, middle); // center of screen in distance scale
				dx = 0.5 - fmod(r_refdef.view.width * 0.5 + (DotProduct(org, left) - DotProduct(middle, left)) / DotProduct(left, left) + 0.5, 1.0);
				dy = 0.5 - fmod(r_refdef.view.height * 0.5 + (DotProduct(org, up) - DotProduct(middle, up)) / DotProduct(up, up) + 0.5, 1.0);
				VectorMAMAM(1, org, dx, left, dy, up, org);
			}
		}
		else
		{
			// use HUD pixels
			VectorScale(left, scale * r_refdef.view.frustum_x / vid_conwidth.integer * r_labelsprites_scale.value, left); // 1px
			VectorScale(up, scale * r_refdef.view.frustum_y / vid_conheight.integer * r_labelsprites_scale.value, up); // 1px
		}
		break;
	case SPR_ORIENTED:
		// bullet marks on walls
		// ignores viewer entirely
		VectorCopy(mleft, left);
		VectorCopy(mup, up);
		break;
	case SPR_VP_PARALLEL_ORIENTED:
		// I have no idea what people would use this for...
		// oriented relative to view space
		// FIXME: test this and make sure it mimicks software
		left[0] = mleft[0] * r_refdef.view.forward[0] + mleft[1] * r_refdef.view.left[0] + mleft[2] * r_refdef.view.up[0];
		left[1] = mleft[0] * r_refdef.view.forward[1] + mleft[1] * r_refdef.view.left[1] + mleft[2] * r_refdef.view.up[1];
		left[2] = mleft[0] * r_refdef.view.forward[2] + mleft[1] * r_refdef.view.left[2] + mleft[2] * r_refdef.view.up[2];
		up[0] = mup[0] * r_refdef.view.forward[0] + mup[1] * r_refdef.view.left[0] + mup[2] * r_refdef.view.up[0];
		up[1] = mup[0] * r_refdef.view.forward[1] + mup[1] * r_refdef.view.left[1] + mup[2] * r_refdef.view.up[1];
		up[2] = mup[0] * r_refdef.view.forward[2] + mup[1] * r_refdef.view.left[2] + mup[2] * r_refdef.view.up[2];
		break;
	case SPR_OVERHEAD:
		// Overhead games sprites, have some special hacks to look good
		VectorScale(r_refdef.view.left, ent->scale * r_overheadsprites_scalex.value, left);
		VectorScale(r_refdef.view.up, ent->scale * r_overheadsprites_scaley.value, up);
		VectorSubtract(org, r_refdef.view.origin, middle);
		VectorNormalize(middle);
		// offset and rotate
		dir_angle = r_overheadsprites_perspective.value * (1 - fabs(DotProduct(middle, r_refdef.view.forward)));
		up[2] = up[2] + dir_angle;
		VectorNormalize(up);
		VectorScale(up, ent->scale * r_overheadsprites_scaley.value, up);
		// offset (move nearer to player, yz is camera plane)
		org[0] = org[0] - middle[0]*r_overheadsprites_pushback.value;
		org[1] = org[1] - middle[1]*r_overheadsprites_pushback.value;
		org[2] = org[2] - middle[2]*r_overheadsprites_pushback.value;
		// little perspective effect
		up[2] = up[2] + dir_angle * 0.3;
		// a bit of counter-camera rotation
		up[0] = up[0] + r_refdef.view.forward[0] * 0.07;
		up[1] = up[1] + r_refdef.view.forward[1] * 0.07;
		up[2] = up[2] + r_refdef.view.forward[2] * 0.07;
		break;
	}

	// LadyHavoc: interpolated sprite rendering
	for (i = 0;i < MAX_FRAMEBLENDS;i++)
	{
		if (ent->frameblend[i].lerp >= 0.01f)
		{
			mspriteframe_t *frame;
			texture_t *texture;
			RSurf_ActiveCustomEntity(&identitymatrix, &identitymatrix, ent->flags, 0, ent->colormod[0], ent->colormod[1], ent->colormod[2], ent->alpha * ent->frameblend[i].lerp, 4, vertex3f, spritetexcoord2f, NULL, NULL, NULL, NULL, 2, polygonelement3i, polygonelement3s, false, false);
			frame = model->sprite.sprdata_frames + ent->frameblend[i].subframe;
			texture = R_GetCurrentTexture(model->data_textures + ent->frameblend[i].subframe);
		
			// sprites are fullbright by default, but if this one is not fullbright we
			// need to combine the lighting into ambient as sprite lighting is not
			// directional
			if (!(texture->currentmaterialflags & MATERIALFLAG_FULLBRIGHT))
			{
				VectorMAM(1.0f, texture->render_modellight_ambient, 0.25f, texture->render_modellight_diffuse, texture->render_modellight_ambient);
				VectorClear(texture->render_modellight_diffuse);
				VectorClear(texture->render_modellight_specular);
			}

			// SPR_LABEL should not use depth test AT ALL
			if(model->sprite.sprnum_type == SPR_LABEL || model->sprite.sprnum_type == SPR_LABEL_SCALE)
				if(texture->currentmaterialflags & MATERIALFLAG_SHORTDEPTHRANGE)
					texture->currentmaterialflags = (texture->currentmaterialflags & ~MATERIALFLAG_SHORTDEPTHRANGE) | MATERIALFLAG_NODEPTHTEST;

			if(edge)
			{
				// FIXME:: save vectors/origin and re-rotate? necessary if the hotspot can change per frame
				R_RotateSprite(frame, org, left, up, edge, dir_angle);
				edge = 0;
			}

			R_CalcSprite_Vertex3f(vertex3f, org, left, up, frame->left, frame->right, frame->down, frame->up);

			if (r_showspriteedges.integer)
				for (i = 0; i < 4; i++)
					R_DebugLine(vertex3f + i * 3, vertex3f + ((i + 1) % 4) * 3);

			R_DrawCustomSurface_Texture(texture, &identitymatrix, texture->currentmaterialflags, 0, 4, 0, 2, false, false, false);
		}
	}

	rsurface.entity = NULL;
}

void R_Model_Sprite_Draw(entity_render_t *ent)
{
	vec3_t org;
	if (ent->frameblend[0].subframe < 0)
		return;

	Matrix4x4_OriginFromMatrix(&ent->matrix, org);
	R_MeshQueue_AddTransparent((ent->flags & RENDER_WORLDOBJECT) ? TRANSPARENTSORT_SKY : (ent->flags & RENDER_NODEPTHTEST) ? TRANSPARENTSORT_HUD : TRANSPARENTSORT_DISTANCE, org, R_Model_Sprite_Draw_TransparentCallback, ent, 0, rsurface.rtlight);
}

