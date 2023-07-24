// Copyright (C) 2007 Id Software, Inc.
//

// This is included by Atmosphere.cpp just to keep it clean...

#include "TemplatedParticleSystem.h"
#include "Effects.h"

#define	MAX_PRECIPITATION_PARTICLES		4000	// maximum # of particles
//#define	MAX_PRECIPITATION_DISTANCE		1000	// maximum distance from refdef origin that particles are visible
#define PRECIPITATION_HEIGHT			1000	// maximum distance above the player precipitation will be spawned

#define PRECIPITATION_SNOW_HEIGHT		3		// Size of a snow particle
#define	PRECIPITATION_RAIN_HEIGHT		150

#define	PRECIPITATION_DROPDELAY			1000

/************************************************************************/
/* Just adds the parameters field so the atmosphere editor can edit     */
/* them more easily                                                     */
/************************************************************************/

struct sdPrecipArgs {
	sdPrecipitationParameters p;
	const sdHeightMapInstance*	heightMap;
	float currentWaterHeight;
	idBounds bounds;

	ID_INLINE float GetGroundHeightAtPos( const idVec3 &pos, const idVec3 &origin ) {
		return heightMap->GetHeight( pos - origin );
	}

	ID_INLINE float GetPrecipitationDistance( void ) {
		return p.precipitationDistance;
	}
};

template < class ParticleClass > class sdPrecipitationSystem : public sdTemplatedParticleSystem< ParticleClass, sdPrecipArgs  > {
public:
	sdPrecipitationSystem() { this->params.bounds.Clear(); }

	virtual ~sdPrecipitationSystem( void ) {
		FreeRenderEntity();
	}

	virtual void		PresentRenderEntity( void );
	virtual void		FreeRenderEntity( void );

	static sdPrecipitationSystem* SetupSystem( sdPrecipitationParameters &params, const sdHeightMapInstance* heightMap ) {
		sdPrecipitationSystem< ParticleClass > *sys = new sdPrecipitationSystem< ParticleClass >();
		sys->params.p = params;
		sys->params.heightMap = heightMap;
		sys->SetMaxParticles( params.maxParticles );
		sys->SetMaterial( params.material );
		sys->SetupEffect();
		sys->Init();
		return sys;
	}

private:
	void SetupEffect( void ) {
		renderEffect_t &renderEffect = effect.GetRenderEffect();
		renderEffect.declEffect = this->params.p.effect;
		renderEffect.axis.Identity();
		renderEffect.loop = true;
		renderEffect.shaderParms[SHADERPARM_RED]		= 1.0f;
		renderEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
		renderEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
		renderEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
		renderEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

		effectRunning = false;
	}

	sdEffect			effect;
	bool				effectRunning;
	
};

template < class ParticleClass > void sdPrecipitationSystem<ParticleClass>::PresentRenderEntity( void ) {
	sdTemplatedParticleSystem< ParticleClass, sdPrecipArgs >::PresentRenderEntity();


	this->params.bounds.ExpandSelf( 5.f );
	this->params.currentWaterHeight = -32000.f;
	const idClipModel* clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS this->params.bounds, CONTENTS_WATER, &clipModel, 1, NULL );
	if ( count ) {
		if ( clipModel->GetNumCollisionModels() ) {
			idCollisionModel* model = clipModel->GetCollisionModel( 0 );
			int numPlanes = model->GetNumBrushPlanes();
			if ( numPlanes ) {
				this->params.currentWaterHeight = clipModel->GetAbsBounds().GetMaxs()[2];
				this->params.bounds.Clear();
			}
		}
	}

	if ( !effect.GetRenderEffect().declEffect ) return;

	// If we are inside don't run the bacground effect
	int area = gameRenderWorld->PointInArea( this->viewOrg );
	bool runEffect = false;
	if ( area >= 0 ) {
		if ( gameRenderWorld->GetAreaPortalFlags( area ) & ( 1 << PORTAL_OUTSIDE ) ) {
			runEffect = true && !g_skipLocalizedPrecipitation.GetBool();
		}
	}

	// Update the background effect
	if ( runEffect ) {
		effect.GetRenderEffect().origin = this->viewOrg;
		if ( !effectRunning ) {
			effect.Start( gameLocal.time );
			effectRunning = true;
		} else {
			effect.Update();
		}
	} else {
		effect.StopDetach();
		effectRunning = false;
	}


}

template < class ParticleClass > void sdPrecipitationSystem<ParticleClass>::FreeRenderEntity( void ) {
	sdTemplatedParticleSystem< ParticleClass, sdPrecipArgs >::FreeRenderEntity();
	if ( !effect.GetRenderEffect().declEffect ) return;

	effect.FreeRenderEffect();
}

/************************************************************************/
/*  Snow Flakes                                                         */
/************************************************************************/

class sdFlake {
private:
	idVec3 position;
	idVec3 velocity;
	idVec3 velocityNorm;
	float height;
	float weight;
	float alpha;
	int	nextDropTime;

public:
	sdFlake( void ) {
		nextDropTime = -1; //Make yourself inactive
	}

	static const int NUM_INSTANCE_VERTEXES = 3;
	static const int NUM_INSTANCE_INDEXES = 3;

	void StaticInitializeAndRender( sdTemplatedParticleSystem<sdFlake, sdPrecipArgs> *sys ) {
		srfTriangles_t * tri = sys->GetTriSurf(); 
		idDrawVert *verts = tri->verts + tri->numVerts;
		for ( int i=0; i<3; i++ ) {
			verts[i].color[0] = 0xFF;
			verts[i].color[1] = 0xFF;
			verts[i].color[2] = 0xFF;
			verts[i].color[3] = 0xFF;

			switch ( i%3 ) {
			case 0:
				verts[i].SetST( 2.0f, 1.0f );
				break;
			case 1:
				verts[i].SetST( 0.0f, 1.0f );
				break;
			case 2:
				verts[i].SetST( 0.0f, -1.0f );
				break;
			}
		}

		tri->numVerts += 3;
	}

	bool Initialize( sdTemplatedParticleSystem<sdFlake, sdPrecipArgs> *sys ) {
		float precipDist = sys->params.GetPrecipitationDistance();

		position[0] = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position[1] = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position[2] = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position += sys->GetViewOrg();

		velocity = sys->params.p.windScale * sdAtmosphere::currentAtmosphere->GetWindVector();
		velocity[2] = -(sys->params.p.fallMin +  idRandom::StaticRandom().RandomFloat() * ( sys->params.p.fallMax - sys->params.p.fallMin ));
		float blend = idRandom::StaticRandom().RandomFloat(); // weight and height are related
		height = sys->params.p.heightMin + blend * ( sys->params.p.heightMax - sys->params.p.heightMin );
		weight = sys->params.p.weightMin + blend * ( sys->params.p.weightMax - sys->params.p.weightMin );

		// Ensure it doesn't attempt to generate every frame, to prevent
		// 'clumping' when there's only a small sky area available.
		nextDropTime = gameLocal.time + PRECIPITATION_DROPDELAY;
		velocityNorm = velocity;
		velocityNorm.NormalizeFast();
		return true;
	}

	bool Update( sdTemplatedParticleSystem<sdFlake, sdPrecipArgs> *sys ) {
		idVec2 distance;

		if( nextDropTime == -1 ) {
			return false;
		}

		position += velocity * (gameLocal.msec * 0.001f);

		// Snow lives in a box 2*MAX_PRECIPITATION_DISTANCE long and wide and 1.5*MAX_PRECIPITATION_DISTANCE high
		// This wastes less snow particles in the common case of a player on level ground
		idVec3 local = position - sys->GetViewOrg();
		float precipDist = sys->params.GetPrecipitationDistance();
		if ( local.x < -precipDist ) {
			local.x += 2 * precipDist;
		}
		if ( local.y < -precipDist ) {
			local.y += 2 * precipDist;
		}
		if ( local.z < -(precipDist * 0.5) ) {
			local.z += 1.5 * precipDist;
		}
		if ( local.x > precipDist ) {
			local.x -= 2 * precipDist;
		}
		if ( local.y > precipDist ) {
			local.y -= 2 * precipDist;
		}
		if ( local.z > precipDist ) {
			local.z -= 1.5 * precipDist;
		}
		position = local + sys->GetViewOrg();

		sys->params.bounds.AddPoint( position );

		return true;
	}

	void Render( sdTemplatedParticleSystem<sdFlake, sdPrecipArgs> *sys ) {
		idVec3			forward, right;
		idDrawVert*		face;
		idVec2			line;
		float			sinTumbling, cosTumbling, dist;
		idVec3			start, finish;
		idVec3			left, up;
		srfTriangles_t*	tri;
		float size;

		if ( nextDropTime == -1 ) {
			return;
		}

		start = position;

		sinTumbling = idMath::Sin( position[2] * 0.03125f * ( 0.5f * weight ) );
		cosTumbling = idMath::Cos( ( position[2] + position[1] ) * 0.03125f * ( 0.5f * weight ) );
		start[0] += sys->params.p.tumbleStrength * ( 1.0f - velocityNorm[2] ) * sinTumbling;
		start[1] += sys->params.p.tumbleStrength * ( 1.0f - velocityNorm[2] ) * cosTumbling;

		if ( start[2] < sys->params.currentWaterHeight ) {
			return;
		}

		if ( start[2] < sys->params.GetGroundHeightAtPos( start, sys->GetRenderEntity().origin ) ) {
			return; //temporary hide it
		}

		line = position.ToVec2() - sys->GetViewOrg().ToVec2();
		dist = 1.0f; /*( position - sys->GetViewOrg() ).LengthSqr();
		// dist becomes scale
		if( dist > ( 500.0f * 500.0f ) ) {
			dist = 1.0f + ( ( dist - ( 500.0f * 500.0f ) ) * ( 10.f / ( 2000.0f * 2000.0f ) ) );
		} else {
			dist = 1.0f;
		}*/

		size = dist * height;
		tri = sys->GetTriSurf(); 
		face = tri->verts + tri->numVerts;

		left = sys->GetViewAxis()[1];
		up = sys->GetViewAxis()[2];
		left *= size;
		up *= size;

		face->xyz = start - up - left;
		face->SetST( 2.0f, 1.0f );
		face->color[0] = 0xFF;
		face->color[1] = 0xFF;
		face->color[2] = 0xFF;
		face->color[3] = 0xFF;
		face++;

		face->xyz = start - up + left;
		face->SetST( 0.0f, 1.0f );
		face->color[0] = 0xFF;
		face->color[1] = 0xFF;
		face->color[2] = 0xFF;
		face->color[3] = 0xFF;
		face++;

		face->xyz = start + up + left;
		face->SetST( 0.0f, -1.0f );
		face->color[0] = 0xFF;
		face->color[1] = 0xFF;
		face->color[2] = 0xFF;
		face->color[3] = 0xFF;
		face++;

		tri->indexes[tri->numIndexes + 0] = tri->numVerts + 0;
		tri->indexes[tri->numIndexes + 1] = tri->numVerts + 1;
		tri->indexes[tri->numIndexes + 2] = tri->numVerts + 2;

		tri->numVerts += 3;
		tri->numIndexes += 3;

		tri->bounds.AddPoint( position );
	}
};

/************************************************************************/
/*  Rain Drops                                                          */
/************************************************************************/
class sdDrop {
private:
	idVec3 position;
	idVec3 velocity;
	idVec3 velocityNorm;
	float height;
	float weight;
	float alpha;
	int	nextDropTime;
public:
	sdDrop( void ) {
		nextDropTime = -1; //Make yourself inactive
	}

	static const int NUM_INSTANCE_VERTEXES = 3;
	static const int NUM_INSTANCE_INDEXES = 3;

	void StaticInitializeAndRender( sdTemplatedParticleSystem<sdDrop, sdPrecipArgs> *sys ) {
		srfTriangles_t * tri = sys->GetTriSurf(); 
		idDrawVert *verts = tri->verts + tri->numVerts;
		for ( int i=0; i<3; i++ ) {
			verts[i].color[0] = 0xFF;
			verts[i].color[1] = 0xFF;
			verts[i].color[2] = 0xFF;
			verts[i].color[3] = 0xFF;

			switch ( i%3 ) {
			case 0:
				verts[i].SetST( 2.0f, 1.0f );
				break;
			case 1:
				verts[i].SetST( 0.0f, 1.0f );
				break;
			case 2:
				verts[i].SetST( 0.0f, -1.0f );
				break;
			}
		}

		tri->numVerts += 3;
	}

	bool Initialize( sdTemplatedParticleSystem<sdDrop, sdPrecipArgs> *sys ) {

		float precipDist = sys->params.GetPrecipitationDistance();

		position[0] = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position[1] = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position[2] = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position += sys->GetViewOrg();

		float lower = Max( sys->params.GetGroundHeightAtPos( position, sys->GetRenderEntity().origin ), sys->params.currentWaterHeight );

		if ( position[2] < lower ) {
			float ceil = sys->GetViewOrg()[0] + precipDist;
			if ( ceil < lower ) {
				// Very deep underground no point to spawn anything
				return false;
			}
			position[2] = lower + idRandom::StaticRandom().RandomFloat() * ( ceil - lower );
		}

		velocity = sdAtmosphere::currentAtmosphere->GetWindVector() * sys->params.p.windScale;
		velocity[2] = -(sys->params.p.fallMin +  idRandom::StaticRandom().RandomFloat() * ( sys->params.p.fallMax - sys->params.p.fallMin ));
		float blend = idRandom::StaticRandom().RandomFloat(); // weight and height are related
		height = sys->params.p.heightMin + blend * ( sys->params.p.heightMax - sys->params.p.heightMin );
		weight = sys->params.p.weightMin + blend * ( sys->params.p.weightMax - sys->params.p.weightMin );

		// Ensure it doesn't attempt to generate every frame, to prevent
		// 'clumping' when there's only a small sky area available.
		nextDropTime = gameLocal.time + PRECIPITATION_DROPDELAY;
		velocityNorm = velocity;
		velocityNorm.NormalizeFast();
		return true;
	}

	bool Update( sdTemplatedParticleSystem<sdDrop, sdPrecipArgs> *sys ) {
		idVec2 distance;

		if( nextDropTime == -1 ) {
			return false;
		}

		position += velocity * (gameLocal.msec * 0.001f);

		idVec3 local = position - sys->GetViewOrg();
		float precipDist = sys->params.GetPrecipitationDistance();
		if ( local.x < -precipDist ) {
			local.x += 2 * precipDist;
		}
		if ( local.y < -precipDist ) {
			local.y += 2 * precipDist;
		}
		if ( local.z < -precipDist ) {
			//local.z += 2 * MAX_PRECIPITATION_DISTANCE;
			nextDropTime = -1;
			return false;
		}
		if ( local.x > precipDist ) {
			local.x -= 2 * precipDist;
		}
		if ( local.y > precipDist ) {
			local.y -= 2 * precipDist;
		}
		if ( local.z > precipDist ) {
			//local.z -= 2 * MAX_PRECIPITATION_DISTANCE;
			nextDropTime = -1;
			return false;
		}
		position = local + sys->GetViewOrg();

		if ( position[2] < sys->params.currentWaterHeight ) {
			nextDropTime = -1;
			return false;
		}

		// If it moved underground kill it
		if( position[2] < sys->params.GetGroundHeightAtPos( position, sys->GetRenderEntity().origin ) ) {
			nextDropTime = -1;
			return false;
		}

		sys->params.bounds.AddPoint( position );

		return true;
	}

	void Render( sdTemplatedParticleSystem<sdDrop, sdPrecipArgs> *sys ) {
		// Draw a raindrop

		idVec3		forward, right;
		idDrawVert	*face;
		srfTriangles_t *tri;
		idVec2		line;
		float		len, dist;
		idVec3		start, finish;

		if( nextDropTime == -1 ) {
			return;
		}

		start = position;
		dist = ( position - sys->GetViewOrg() ).LengthSqr();

		// Make sure it doesn't clip through surfaces
		len = height;

		// fade nearby rain particles
		if( dist < ( 128.f * 128.f ) ) {
			dist = .25f + .75f * ( dist / ( 128.f * 128.f ) );
		} else {
			dist = 1.0f;
		}

		forward = velocityNorm;
		finish = start - forward * len;

		line[0] = forward * sys->GetViewAxis()[1];
		line[1] = forward * sys->GetViewAxis()[2];

		right = sys->GetViewAxis()[1] * line[1] + sys->GetViewAxis()[2] * -line[0];
		right.Normalize();

		// dist = 1.0;
		tri = sys->GetTriSurf(); 
		face = tri->verts + tri->numVerts;

		face->xyz = finish;
		face->SetST( 2.0f, 1.0f );
		face->color[0] = 0xFF;
		face->color[1] = 0xFF;
		face->color[2] = 0xFF;
		face->color[3] = 0xFF;
		face++;

		face->xyz = start + weight * right;
		face->SetST( 0.0f, 1.0f );
		face->color[0] = 0xFF;
		face->color[1] = 0xFF;
		face->color[2] = 0xFF;
		face->color[3] = 0xFF;
		face++;

		face->xyz = start - weight * right;
		face->SetST( 0.0f, -1.0f );
		face->color[0] = 0xFF;
		face->color[1] = 0xFF;
		face->color[2] = 0xFF;
		face->color[3] = 0xFF;
		face++;

		tri->indexes[tri->numIndexes + 0] = tri->numVerts + 0;
		tri->indexes[tri->numIndexes + 1] = tri->numVerts + 1;
		tri->indexes[tri->numIndexes + 2] = tri->numVerts + 2;

		tri->numVerts += 3;
		tri->numIndexes += 3;

		tri->bounds.AddPoint( position );
	}
};


/************************************************************************/
/*  Rain Splashes                                                       */
/************************************************************************/

class sdSplash {
private:
	idVec3 position;
	idVec3 velocity;
	float size;
	float weight;
	float alpha;
	int lifeTime;
	int	dropTime;

public:
	sdSplash( void ) {
		dropTime = -1; //Make yourself inactive
	}

	static const int NUM_INSTANCE_VERTEXES = 4;
	static const int NUM_INSTANCE_INDEXES = 6;

	void StaticInitializeAndRender( sdTemplatedParticleSystem<sdSplash, sdPrecipArgs> *sys ) {
		srfTriangles_t * tri = sys->GetTriSurf(); 
		idDrawVert *verts = tri->verts + tri->numVerts;
		for ( int i=0; i<4; i++ ) {
			verts[i].color[0] = 0xFF;
			verts[i].color[1] = 0xFF;
			verts[i].color[2] = 0xFF;
			verts[i].color[3] = 0xFF;

			switch ( i%4 ) {
			case 0:
				verts[i].SetST( 1.0f, 1.0f );
				break;
			case 1:
				verts[i].SetST( 0.0f, 1.0f );
				break;
			case 2:
				verts[i].SetST( 0.0f, 0.0f );
				break;	
			case 3:
				verts[i].SetST( 1.0f, 0.0f );
				break;	
			}
		}

		tri->numVerts += 4;
	}

	bool Initialize( sdTemplatedParticleSystem<sdSplash, sdPrecipArgs> *sys ) {
		float precipDist = sys->params.GetPrecipitationDistance();
		position.x = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position.y = idRandom::StaticRandom().CRandomFloat() * precipDist;
		position += sys->GetViewOrg();
		position.z = sys->params.GetGroundHeightAtPos( position, sys->GetRenderEntity().origin );
		if ( position.z < sys->params.currentWaterHeight ) {
			position.z = sys->params.currentWaterHeight;
		}
		
		velocity = sys->params.p.windScale * sdAtmosphere::currentAtmosphere->GetWindVector();
		velocity.z = (sys->params.p.fallMin +  idRandom::StaticRandom().RandomFloat() * ( sys->params.p.fallMax - sys->params.p.fallMin ));
		float blend = idRandom::StaticRandom().RandomFloat(); // weight and height are related
		size   = sys->params.p.heightMin + blend * ( sys->params.p.heightMax - sys->params.p.heightMin );
		weight = sys->params.p.weightMin + blend * ( sys->params.p.weightMax - sys->params.p.weightMin );

		position.z += sys->params.p.tumbleStrength;

		// Ensure it doesn't attempt to generate every frame, to prevent
		// 'clumping' when there's only a small sky area available.
		lifeTime = sys->params.p.timeMin + idRandom::StaticRandom().RandomFloat() * ( sys->params.p.timeMax - sys->params.p.timeMin );
		dropTime = /*gameLocal.time + */lifeTime;
		return true;
	}

	bool Update( sdTemplatedParticleSystem<sdSplash, sdPrecipArgs> *sys ) {
		idVec2 distance;

		if ( dropTime < 0 ) {
			return false;
		}
		dropTime -= gameLocal.msec;

		position += velocity * (gameLocal.msec * 0.001f);
		size += weight * (gameLocal.msec * 0.001f); 

		// Snow lives in a box 2*MAX_PRECIPITATION_DISTANCE long and wide and 1.5*MAX_PRECIPITATION_DISTANCE high
		// This wastes less snow particles in the common case of a player on level ground
		idVec3 local = position - sys->GetViewOrg();
		bool wrapped = false;
		float precipDist = sys->params.GetPrecipitationDistance();
		if ( local.x < -precipDist ) {
			local.x += 2 * precipDist;
			wrapped = true;
		}
		if ( local.y < -precipDist ) {
			local.y += 2 * precipDist;
			wrapped = true;
		}
		if ( local.x > precipDist ) {
			local.x -= 2 * precipDist;
			wrapped = true;
		}
		if ( local.y > precipDist ) {
			local.y -= 2 * precipDist;
			wrapped = true;
		}
		position = local + sys->GetViewOrg();
		
		if ( wrapped ) {
			position.z = sys->params.GetGroundHeightAtPos( position, sys->GetRenderEntity().origin ) + sys->params.p.tumbleStrength;
			if ( position.z < (sys->params.currentWaterHeight + sys->params.p.tumbleStrength ) ) {
				position.z = sys->params.currentWaterHeight + sys->params.p.tumbleStrength;
			}
		}

		sys->params.bounds.AddPoint( position );

		return true;
	}

	void Render( sdTemplatedParticleSystem<sdSplash, sdPrecipArgs> *sys ) {
		idDrawVert*		face;
		idVec3			start;
		srfTriangles_t*	tri;
		idVec3			left, up;

		if ( dropTime < 0 ) {
			return;
		}

		start = position;
		tri = sys->GetTriSurf(); 
		face = tri->verts + tri->numVerts;

		left = sys->GetViewAxis()[1];
		up = sys->GetViewAxis()[2];
		left *= size;
		up *= size;

		byte color = (int)(((dropTime) / (float)lifeTime) *  255.0f);
		dword colorDW = color | color << 8 | color << 16 | color << 24;

		face->xyz = start - up - left;
		face->SetST( 1.f, 1.f );
		face->SetColor( colorDW );
		face++;

		face->xyz = start - up + left;
		face->SetST( 0.f, 1.f );
		face->SetColor( colorDW );
		face++;

		face->xyz = start + up + left;
		face->SetST( 0.f, 0.f );
		face->SetColor( colorDW );
		face++;

		face->xyz = start + up - left;
		face->SetST( 1.f, 0.f );
		face->SetColor( colorDW );
		face++;

		tri->indexes[tri->numIndexes + 0] = tri->numVerts + 0;
		tri->indexes[tri->numIndexes + 1] = tri->numVerts + 1;
		tri->indexes[tri->numIndexes + 2] = tri->numVerts + 2;
		tri->indexes[tri->numIndexes + 3] = tri->numVerts + 0;
		tri->indexes[tri->numIndexes + 4] = tri->numVerts + 2;
		tri->indexes[tri->numIndexes + 5] = tri->numVerts + 3;

		tri->numVerts += 4;
		tri->numIndexes += 6;

		tri->bounds.AddPoint( position );
	}
};
