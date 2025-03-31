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
//no include guard: embedded into ParticleSystem.cpp

//TODO: refactor idRandom more extensively if we want to run this code on GLSL
float idRandom_RandomFloat(PINOUT(int) seed) {
	idRandom rnd(seed);
	float res = rnd.RandomFloat();
	seed = rnd.GetSeed();
	return res;
}
float idRandom_CRandomFloat(PINOUT(int) seed) {
	idRandom rnd(seed);
	float res = rnd.CRandomFloat();
	seed = rnd.GetSeed();
	return res;
}


void idParticleParm_Clear(POUT(idParticleParm) self) {
	self.table = NULL;
	self.from = 0.0f;
	self.to = 0.0f;
}
float idParticleParm_Eval(PIN(idParticleParm) self, float frac) {
	if ( self.table ) {
		return self.table->TableLookup( frac );
	}
	return self.from + frac * ( self.to - self.from );
}
float idParticleParm_Integrate(PIN(idParticleParm) self, float frac) {
	if ( self.table ) {
		common->Printf( "idParticleParm::Integrate: can't integrate tables\n" );
		return 0;
	}
	return ( self.from + frac * ( self.to - self.from ) * 0.5f ) * frac;
}


idVec3 idParticle_ParticleOriginStdSys(
	PIN(idPartStageData) stg, PIN(idParticleData) part,
	PINOUT(int) random
) {
	idVec3 origin = idVec3(0.0f, 0.0f, 0.0f);

	if ( stg.customPathType == PPATH_STANDARD ) {
		//
		// find intial origin distribution
		//
		float radiusSqr, angle1, angle2;

		switch( stg.distributionType ) {
			case PDIST_RECT: {	// ( sizeX sizeY sizeZ )
				origin.x = ( ( stg.randomDistribution ) ? idRandom_CRandomFloat(random) : 1.0f ) * stg.distributionParms[0];
				origin.y = ( ( stg.randomDistribution ) ? idRandom_CRandomFloat(random) : 1.0f ) * stg.distributionParms[1];
				origin.z = ( ( stg.randomDistribution ) ? idRandom_CRandomFloat(random) : 1.0f ) * stg.distributionParms[2];
				break;
			}
			case PDIST_CYLINDER: {	// ( sizeX sizeY sizeZ ringFraction )
				angle1 = ( ( stg.randomDistribution ) ? idRandom_CRandomFloat(random) : 1.0f ) * idMath::TWO_PI;

				SinCos16( angle1, origin.x, origin.y );
				origin.z = ( ( stg.randomDistribution ) ? idRandom_CRandomFloat(random) : 1.0f );

				// reproject points that are inside the ringFraction to the outer band
				if ( stg.distributionParms[3] > 0.0f ) {
					radiusSqr = origin.x * origin.x + origin.y * origin.y;
					if ( radiusSqr < stg.distributionParms[3] * stg.distributionParms[3] ) {
						// if we are inside the inner reject zone, rescale to put it out into the good zone
						float f = sqrt( radiusSqr ) / stg.distributionParms[3];
						float invf = 1.0f / f;
						float newRadius = stg.distributionParms[3] + f * ( 1.0f - stg.distributionParms[3] );
						float rescale = invf * newRadius;

						origin.x *= rescale;
						origin.y *= rescale;
					}
				}
				origin.x *= stg.distributionParms[0];
				origin.y *= stg.distributionParms[1];
				origin.z *= stg.distributionParms[2];
				break;
			}
			case PDIST_SPHERE: {	// ( sizeX sizeY sizeZ ringFraction )
				// iterating with rejection is the only way to get an even distribution over a sphere
				if ( stg.randomDistribution ) {
					do {
						origin.x = idRandom_CRandomFloat(random);
						origin.y = idRandom_CRandomFloat(random);
						origin.z = idRandom_CRandomFloat(random);
						radiusSqr = origin.x * origin.x + origin.y * origin.y + origin.z * origin.z;
					} while( radiusSqr > 1.0f );
				} else {
					origin = idVec3( 1.0f, 1.0f, 1.0f );
					radiusSqr = 3.0f;
				}

				if ( stg.distributionParms[3] > 0.0f ) {
					// we could iterate until we got something that also satisfied ringFraction,
					// but for narrow rings that could be a lot of work, so reproject inside points instead
					if ( radiusSqr < stg.distributionParms[3] * stg.distributionParms[3] ) {
						// if we are inside the inner reject zone, rescale to put it out into the good zone
						float f = sqrt( radiusSqr ) / stg.distributionParms[3];
						float invf = 1.0f / f;
						float newRadius = stg.distributionParms[3] + f * ( 1.0f - stg.distributionParms[3] );
						float rescale = invf * newRadius;

						origin.x *= rescale;
						origin.y *= rescale;
						origin.z *= rescale;
					}
				}
				origin.x *= stg.distributionParms[0];
				origin.y *= stg.distributionParms[1];
				origin.z *= stg.distributionParms[2];
				break;
			}
		}

		// offset will effect all particle origin types
		// add this before the velocity and gravity additions
		origin += stg.offset;

		//
		// add the velocity over time
		//
		idVec3 dir = idVec3(0.0f, 0.0f, 0.0f);

		switch( stg.directionType ) {
			case PDIR_CONE: {
				if (stg.directionParms[0] != 0.0) {
					// angle is the full angle, so 360 degrees is any spherical direction
					angle1 = idRandom_CRandomFloat(random) * stg.directionParms[0] * (PI / 180.0f);
					angle2 = idRandom_CRandomFloat(random) * PI;

					float s1, c1, s2, c2;
					SinCos16( angle1, s1, c1 );
					SinCos16( angle2, s2, c2 );

					dir.x = s1 * c2;
					dir.y = s1 * s2;
					dir.z = c1;
				}
				else {
					// constant direction optimization
					dir.Set(0.0f, 0.0f, 1.0f);
				}
				break;
			}
			case PDIR_OUTWARD: {
				dir = normalize(origin);
				dir.z += stg.directionParms[0];
				break;
			}
		}

		// add speed
		float iSpeed = idParticleParm_Integrate( stg.speed, part.frac );
		origin += dir * iSpeed * stg.particleLife;

	} else {
		//
		// custom paths completely override both the origin and velocity calculations, but still
		// use the standard gravity
		//
		float age = part.frac * stg.particleLife;

		float angle1, angle2, speed1, speed2;
		switch( stg.customPathType ) {
			case PPATH_HELIX: {		// ( sizeX sizeY sizeZ radialSpeed axialSpeed )
				speed1 = idRandom_CRandomFloat(random);
				speed2 = idRandom_CRandomFloat(random);
				angle1 = idRandom_RandomFloat(random) * (2.0f * PI) + stg.customPathParms[3] * speed1 * age;

				float s1, c1;
				SinCos16( angle1, s1, c1 );

				origin.x = c1 * stg.customPathParms[0];
				origin.y = s1 * stg.customPathParms[1];
				origin.z = idRandom_RandomFloat(random) * stg.customPathParms[2] + stg.customPathParms[4] * speed2 * age;
				break;
			}
			case PPATH_FLIES: {		// ( radialSpeed axialSpeed size )
				speed1 = clamp(idRandom_CRandomFloat(random), 0.4f, 1.0f);
				speed2 = clamp(idRandom_CRandomFloat(random), 0.4f, 1.0f);
				angle1 = idRandom_RandomFloat(random) * (PI * 2.0f) + stg.customPathParms[0] * speed1 * age;
				angle2 = idRandom_RandomFloat(random) * (PI * 2.0f) + stg.customPathParms[1] * speed1 * age;

				float s1, c1, s2, c2;
				idMath::SinCos16( angle1, s1, c1 );
				idMath::SinCos16( angle2, s2, c2 );

				origin.x = c1 * c2;
				origin.y = s1 * c2;
				origin.z = -s2;
				origin *= stg.customPathParms[2];
				break;
			}
			case PPATH_ORBIT: {		// ( radius speed axis )
				angle1 = idRandom_RandomFloat(random) * (PI * 2.0f) + stg.customPathParms[1] * age;

				float s1, c1;
				idMath::SinCos16( angle1, s1, c1 );

				origin.x = c1 * stg.customPathParms[0];
				origin.y = s1 * stg.customPathParms[0];

				float radius = stg.customPathParms[0];
				//stgatilov: this is total crap copied from method:
				//  origin.ProjectSelfOntoSphere( radius );
				float rsqr = radius * radius;
				float len = length(origin);
				if ( len < rsqr * 0.5f ) {
					origin.z = sqrt( rsqr - len );
				} else {
					origin.z = rsqr / ( 2.0f * sqrt( len ) );
				}
				break;
			}
			case PPATH_DRIP: {		// ( speed )
				origin.x = 0.0f;
				origin.y = 0.0f;
				origin.z = -( age * stg.customPathParms[0] );
				break;
			}
		}

		origin += stg.offset;
	}

	return origin;
}

idVec3 idParticle_ParticleOrigin(
	PIN(idPartStageData) stg, PIN(idPartSysData) psys, PIN(idParticleData) part,
	PINOUT(int) random
) {
	idVec3 origin = idParticle_ParticleOriginStdSys(stg, part, random);

	//stgatilov: be extremely cautious when changing the following code!
	//if you change its behavior, make sure to also change bounding box calculation in:
	//  1) idParticle_GetStageBoundsModel
	//  2) idParticle_GetStageBoundsDeform

	if ( stg.worldAxis ) { // SteveL #3950 -- allow particles to use world axis for their offset and travel direction
		origin *= transpose(psys.entityAxis);
	} else {
		origin *= part.axis; // adjust for any per-particle offset
	}
	origin += part.origin;   // adjust for any per-particle offset

	float age = part.frac * stg.particleLife;

	// add gravity after adjusting for axis
	if ( stg.worldGravity ) {
		idVec3 gra = idVec3( 0, 0, -stg.gravity );
		gra *= transpose(psys.entityAxis);
		origin += gra * age * age;
	} else {
		origin.z -= stg.gravity * age * age;
	}

	return origin;
}

idVec4 idParticle_ParticleColors(
	PIN(idPartStageData) stg, PIN(idPartSysData) psys, PIN(idParticleData) part
) {
	float	fadeFraction = 1.0f;

	// most particles fade in at the beginning and fade out at the end
	if ( part.frac < stg.fadeInFraction ) {
		fadeFraction *= ( part.frac / stg.fadeInFraction );
	} 
	if ( 1.0f - part.frac < stg.fadeOutFraction ) {
		fadeFraction *= ( ( 1.0f - part.frac ) / stg.fadeOutFraction );
	}

	// individual gun smoke particles get more and more faded as the
	// cycle goes on (note that totalParticles won't be correct for a surface-particle deform)
	if ( stg.fadeIndexFraction ) {
		int totalParticles = psys.totalParticles;
		float indexFrac = ( totalParticles - part.index ) / float(totalParticles);
		if ( indexFrac < stg.fadeIndexFraction ) {
			fadeFraction *= indexFrac / stg.fadeIndexFraction;
		}
	}

	idVec4 fcolor = (stg.entityColor ? psys.entityParmsColor : stg.color) * fadeFraction + stg.fadeColor * (1.0f - fadeFraction);
	fcolor.x = clamp(fcolor.x, 0.0f, 1.0f);
	fcolor.y = clamp(fcolor.y, 0.0f, 1.0f);
	fcolor.z = clamp(fcolor.z, 0.0f, 1.0f);
	fcolor.w = clamp(fcolor.w, 0.0f, 1.0f);

	return fcolor;
}

void idParticle_ParticleTexCoords(
	PIN(idPartStageData) stg, PIN(idPartSysData) psys, PIN(idParticleData) part,
	POUT(idVec2) tc0, POUT(idVec2) tc1, POUT(idVec2) tc2, POUT(idVec2) tc3,
	POUT(float) animationFrameFrac	//only used if animation is present
) {
	float	s, width;
	float	t, height;

	float age = part.frac * stg.particleLife;

	animationFrameFrac = -1.0f;

	if ( stg.animationFrames > 1 ) {
		width = 1.0f / stg.animationFrames;
		float	floatFrame;
		if ( stg.animationRate ) {
			// explicit, cycling animation
			floatFrame = age * stg.animationRate;
		} else {
			// single animation cycle over the life of the particle
			floatFrame = part.frac * stg.animationFrames;
		}
		int	intFrame = int(floatFrame);
		animationFrameFrac = clamp(floatFrame - intFrame, 0.0f, 1.0f);
		s = width * intFrame;
	} else {
		s = 0.0f;
		width = 1.0f;
	}

	t = 0.0f;
	height = 1.0f;

	tc0 = idVec2(s, t);
	tc1 = idVec2(s+width, t);
	tc2 = idVec2(s, t+height);
	tc3 = idVec2(s+width, t+height);
}


//particle functions below fill these "draw vertex" structs
//this is a stripped-down version of idDrawVert
struct idParticleDrawVert {
	idVec3 xyz;
	idVec2 st;
	idVec4 color;
};

void idParticle_EmitQuadAnimated(
	PIN(idParticleDrawVert) v0, PIN(idParticleDrawVert) v1, PIN(idParticleDrawVert) v2, PIN(idParticleDrawVert) v3,
	PIN(idPartStageData) stg, float animationFrameFrac, EMITTER emitter
) {
	if (animationFrameFrac < 0.0f)
		idParticle_EmitQuad(v0, v1, v2, v3, emitter);
	else {
		// if we are doing strip-animation, we need to double the quad and cross fade it
		float width = 1.0f / stg.animationFrames;
		float frac = animationFrameFrac;
		float iFrac = 1.0f - frac;

		idParticleDrawVert vm0 = v0, vm1 = v1, vm2 = v2, vm3 = v3;
		vm0.color = v0.color * iFrac;
		vm1.color = v1.color * iFrac;
		vm2.color = v2.color * iFrac;
		vm3.color = v3.color * iFrac;
		idParticle_EmitQuad(vm0, vm1, vm2, vm3, emitter);

		vm0.color = v0.color * frac;
		vm1.color = v1.color * frac;
		vm2.color = v2.color * frac;
		vm3.color = v3.color * frac;
		vm0.st.x += width;
		vm1.st.x += width;
		vm2.st.x += width;
		vm3.st.x += width;
		idParticle_EmitQuad(vm0, vm1, vm2, vm3, emitter);
	}
}

void idParticle_CreateParticle(
	PIN(idPartStageData) stg, PIN(idPartSysData) psys, PIN(idParticleData) part,
	EMITTER emitter
) {
	idVec4 color = idParticle_ParticleColors(stg, psys, part);
	// if we are completely faded out, kill the particle
	if ( color.x == 0 && color.y == 0 && color.z == 0 && color.w == 0 ) {
		return;
	}

	int random = part.randomSeed;
	idVec3 origin = idParticle_ParticleOrigin( stg, psys, part, random );

	idVec2 tc0, tc1, tc2, tc3;
	float animationFrameFrac;
	idParticle_ParticleTexCoords( stg, psys, part, tc0, tc1, tc2, tc3, animationFrameFrac );


	float	psize = idParticleParm_Eval( stg.size, part.frac );
	float	paspect = idParticleParm_Eval( stg.aspect, part.frac );

	float	width = psize;
	float	height = psize * paspect;

	if ( stg.orientation == POR_AIMED ) {
		idVec3 stepOrigin = origin;
		idVec3 stepLeft = idVec3(0.0f, 0.0f, 0.0f);

		int numTrails = int(stg.orientationParms[0]);
		float trailTime = stg.orientationParms[1];
		if ( trailTime == 0.0f ) {
			trailTime = 0.5f;
		}

		float height = 1.0f / ( 1 + numTrails );
		float t = 0;

		for ( int i = 0 ; i <= numTrails ; i++ ) {
			idParticleData partMod = part;
			float ageMod = part.frac * stg.particleLife - ( i + 1 ) * trailTime / ( numTrails + 1 );	// time to back up
			partMod.frac = ageMod / stg.particleLife;

			int random = partMod.randomSeed;
			idVec3 oldOrigin = idParticle_ParticleOrigin(stg, psys, partMod, random);

			idVec3 up = stepOrigin - oldOrigin;	// along the direction of travel
			idVec3 forwardDir = psys.viewAxis[0];
			forwardDir *= transpose(psys.entityAxis);
			up -= ( up * forwardDir ) * forwardDir;
			up = normalize(up);
			idVec3 left = cross(up, forwardDir);
			left *= psize;

			idParticleDrawVert v0, v1, v2, v3;
			v0.color = v1.color = v2.color = v3.color = color;
			v0.st = tc0;
			v1.st = tc1;
			v2.st = tc2;
			v3.st = tc3;

			if ( i == 0 ) {
				v0.xyz = stepOrigin - left;
				v1.xyz = stepOrigin + left;
			} else {
				v0.xyz = stepOrigin - stepLeft;
				v1.xyz = stepOrigin + stepLeft;
			}
			v2.xyz = oldOrigin - left;
			v3.xyz = oldOrigin + left;

			// modify texcoords
			v0.st.y = t;
			v1.st.y = t;
			v2.st.y = t + height;
			v3.st.y = t + height;

			t += height;

			idParticle_EmitQuadAnimated(v0, v1, v2, v3, stg, animationFrameFrac, emitter);

			stepOrigin = oldOrigin;
			stepLeft = left;
		}

		return;
	}

	//
	// constant rotation 
	//
	float angle = ( stg.initialAngle ) ? stg.initialAngle : 360 * idRandom_RandomFloat(random);

	float angleMove = idParticleParm_Integrate( stg.rotationSpeed, part.frac ) * stg.particleLife;
	// have hald the particles rotate each way
	if ( part.index & 1 ) {
		angle += angleMove;
	} else {
		angle -= angleMove;
	}

	angle = angle * (PI / 180);
	float s, c;
	SinCos16(angle, s, c);

	idVec3	left, up;
	if ( stg.orientation  == POR_Z ) {
		// oriented in entity space
		left.x = s;
		left.y = c;
		left.z = 0;
		up.x = c;
		up.y = -s;
		up.z = 0;
	} else if ( stg.orientation == POR_X ) {
		// oriented in entity space
		left.x = 0;
		left.y = c;
		left.z = s;
		up.x = 0;
		up.y = -s;
		up.z = c;
	} else if ( stg.orientation == POR_Y ) {
		// oriented in entity space
		left.x = c;
		left.y = 0;
		left.z = s;
		up.x = -s;
		up.y = 0;
		up.z = c;
	} else {
		// oriented in viewer space
		idVec3 entityLeft = psys.viewAxis[1];
		idVec3 entityUp = psys.viewAxis[2];
		entityLeft *= transpose(psys.entityAxis);
		entityUp *= transpose(psys.entityAxis);

		left = entityLeft * c + entityUp * s;
		up = entityUp * c - entityLeft * s;
	}

	left *= width;
	up *= height;

	idParticleDrawVert v0, v1, v2, v3;
	v0.color = v1.color = v2.color = v3.color = color;
	v0.st = tc0;
	v1.st = tc1;
	v2.st = tc2;
	v3.st = tc3;
	v0.xyz = origin - left + up;
	v1.xyz = origin + left + up;
	v2.xyz = origin - left - up;
	v3.xyz = origin + left - up;

	idParticle_EmitQuadAnimated(v0, v1, v2, v3, stg, animationFrameFrac, emitter);

	return;
}

int idParticle_GetRandomSeed(PIN(int) index, PIN(int) cycleNumber, PIN(float) randomizer) {
	//stgatilov: compute random seed as hash of:
	//  1) external randomizer
	//  2) cycle number (modulo cyclesDiversity if set)
	//  3) particle index

	//Note: linear dependency on "index" results in obvious visual regularities/patterns
	//probably related: https://en.wikipedia.org/wiki/Linear_congruential_generator#Advantages_and_disadvantages
	//"One flaw specific to LCGs is that, if used to choose points in an n-dimensional space, the points will lie on, at most, pow(n!*m, 1/n) hyperplanes"
	return cycleNumber * 1580030168 + index * index * 2654435769 + int(randomizer * 46341);
}

bool idParticle_EmitParticle(
	PIN(idPartStageData) stg, PIN(idPartSysEmit) psys, PIN(int) index,
	POUT(idParticleData) res, POUT(int) cycleNumber
) {
	int stageAge = psys.viewTimeMs + psys.entityParmsTimeOffset * 1000 - stg.timeOffset * 1000;
	int	stageCycle = stageAge / stg.cycleMsec;

	res.index = index;

	// calculate local age for this index 
	int	bunchOffset = stg.particleLife * 1000 * stg.spawnBunching * index / psys.totalParticles;
	int particleAge = stageAge - bunchOffset;
	int	particleCycle = particleAge / stg.cycleMsec;

	if ( particleCycle < 0 ) {
		// before the particleSystem spawned
		res.frac = -1000000;
		return false;
	}
	if ( stg.cycles && particleCycle >= stg.cycles ) {
		// cycled systems will only run cycle times
		res.frac = -1000000;
		return false;
	}

	int	inCycleTime = particleAge - particleCycle * stg.cycleMsec;
	if ( psys.entityParmsStopTime && psys.viewTimeMs - inCycleTime >= psys.entityParmsStopTime*1000 ) {
		// don't fire any more particles
		res.frac = -1000000;
		return false;
	}

	// supress particles before or after the age clamp
	res.frac = (float)inCycleTime / ( stg.particleLife * 1000 );

	if ( res.frac < 0.0 || res.frac > 1.0 ) {
		// yet to be spawned or in the deadTime band
		return false;
	}

	int cycleSeed = particleCycle;
	if (stg.diversityPeriod > 0)
		cycleSeed %= stg.diversityPeriod;
	cycleNumber = cycleSeed;

	res.randomSeed = idParticle_GetRandomSeed(index, cycleSeed, psys.randomizer);

	res.origin = idVec3(0.0f, 0.0f, 0.0f);
	res.axis[0] = idVec3(1.0f, 0.0f, 0.0f);
	res.axis[1] = idVec3(0.0f, 1.0f, 0.0f);
	res.axis[2] = idVec3(0.0f, 0.0f, 1.0f);

	return true;
}
