
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idVec3 hhSmokeParticles::defaultDir( 0.0f, 0.0f, 1.0f );

/*
================
hhSmokeParticles::hhSmokeParticles
================
*/
hhSmokeParticles::hhSmokeParticles() {
}

#if 0
/*
================
hhParticleSystem::RunTrailStage
================
*/
void hhParticleSystem::RunTrailStage( const int ev ) {
	if (GetParticle()) {
		idFXSmokeStage *stage = particleSystem->events[ev];
		float fraction;
		//HUMANHEAD: aob
		int numParticleToTrigger = 0;
		idVec3 gravity;
		//HUMANHEAD END

		for( int i=0; i<triggers[ev].Num(); i++ ) {
			smokeGroup* current = triggers[ev][i];
			if ( !current->hidden && current && current->state != SMOKE_DEAD ) {
				// do we need to insert another particle?
				if (current->nozzle && current->triggered != current->numToTrigger) {
					//HUMANHEAD: aob - moved logic to helper function
					ResetSmokeParticleTrail( &current->particles[current->triggered], current, stage );
					//HUMANHEAD END
					current->triggered++;
				}
				// move them all, kill if we need to
				bool triggerAlive = false;
				for( int j=0; j<current->triggered; j++ ) {
					if ( current->particles[j].age <= stage->timeToFade ) {
						smokeParticle *sParticle = &current->particles[j];
						idVec3 speed = vec3_origin; // HUMANHEAD JRM - changed to vec3
						if (sParticle->age >= stage->timeToFinalSpeed) {
							speed = stage->finalSpeed;
						} else {
							fraction = sParticle->age * stage->oneOverTimeToFinalSpeed;
							speed = stage->initialSpeed + (stage->finalSpeed - stage->initialSpeed) * fraction;
						}
						//HUMANHEAD: aob
						if (sParticle->age >= stage->timeToFinalGravity) {
							gravity = stage->finalGravity;
						} else {
							fraction = sParticle->age * stage->oneOverTimeToFinalGravity;
							gravity = stage->initialGravity + (stage->finalGravity - stage->initialGravity) * fraction;
						}
						//HUMANHEAD END
						if (sParticle->age != 0) {
							if (stage->slowSpawnSpeedOverLife) {
								fraction = 1.0f - ((float)j / current->numToTrigger);
								//HUMANHEAD: aob
								sParticle->position += DetermineDeltaPos( speed * fraction, sParticle->xyzmove, sParticle->direction );
								//HUMANHEAD END
							} else {
								//HUMANHEAD: aob
								sParticle->position += DetermineDeltaPos( speed, sParticle->xyzmove, sParticle->direction );
								//HUMANHEAD END
							}
							//HUMANHEAD: aob
							sParticle->position += gravity;
							//HUMANHEAD END
						}
						sParticle->age += gameLocal.msec;
						triggerAlive = true;
						if ( (sParticle->age+current->baseTime) >= stage->timeToFade ) {
							if (current->nozzle && stage->continuous ) {
								//HUMANHEAD: aob - moved logic to helper function
								ResetSmokeParticleTrail( sParticle, current, stage );
								//HUMANHEAD END
							}
						}
					}
				}
				if (!triggerAlive) {
					current->state = SMOKE_DEAD;
				}
			}
		}
	}
}

/*
================
hhParticleSystem::ResetSmokeParticleTrail
================
*/
void hhParticleSystem::ResetSmokeParticleTrail( smokeParticle* particle, smokeGroup* group, idFXSmokeStage* smokeStage ) {
	float ang = 0.0f;
	float size = 0.0f;

	particle->position = group->groupOrigin;
	particle->direction = group->groupDirection;
	// HUMANHEAD JRM - need to make spawn direction spherical - NOT box
	ang = gameLocal.random.RandomFloat() * hhMath::TWO_PI;
	size = smokeStage->xymove;
	if( !smokeStage->noXYMoveRandomness ) {
		size *= gameLocal.random.RandomFloat();
	}
	particle->xyzmove = idVec3( 0.0f, hhMath::Cos(ang) * size, hhMath::Sin(ang) * size );
	particle->xyzmove *= particle->direction.ToMat3();
	// HUMANHEAD JRM - end
	particle->age = 0;
}

/*
================
hhParticleSystem::RunSmokeStage
================
*/
void hhParticleSystem::RunSmokeStage( const int ev ) {
	if (GetParticle()) {
		idFXSmokeStage *stage = particleSystem->events[ev];
		if (stage->trails) {
			return RunTrailStage( ev );
		}
		float fraction;
		//HUMANHEAD: aob
		int numParticleToTrigger = 0;
		idVec3 gravity;
		//HUMANHEAD END
		for( int i=0; i<triggers[ev].Num(); i++ ) {
			smokeGroup* current = triggers[ev][i];
			//HUMANHEAD: aob - checked current pointer before use
			if ( current && !current->hidden && current->state != SMOKE_DEAD ) {
				// do we need to insert another particle?				
				// HUMANHEAD JRM - made a while and seperated if
				if (current->nozzle && current->frameCount >= current->triggerEvery && current->triggered < current->numToTrigger) {
					//HUMANHEAD: aob
					numParticleToTrigger = current->minToTrigger + gameLocal.random.RandomInt( current->maxToTrigger - current->minToTrigger );
					//HUMANHEAD END
					while (numParticleToTrigger && current->triggered < current->numToTrigger) {//HUMANHEAD: aob - changed numToTrigger to numParticlesToTrigger
						//HUMANHEAD: aob - moved logic to helper function
						ResetSmokeParticle( &current->particles[current->triggered], current, stage );
						current->triggered++;
						//HUMANHEAD: aob
						--numParticleToTrigger;
						//HUMANHEAD END
					}
					current->triggerEvery = stage->triggerEvery - gameLocal.random.RandomInt( stage->triggerEvery - stage->minTriggerEvery );
					current->frameCount = 0;
				}
				current->frameCount++;
				// move them all, kill if we need to
				idVec3 mwpVector;
				mwpVector.Zero();
				if (stage->moveWithParent && current->groupOrigin != current->oldGroupOrigin) {
					mwpVector = current->groupOrigin - current->oldGroupOrigin;
					current->oldGroupOrigin = current->groupOrigin;
				}
				bool triggerAlive = false;
				for( int j=0; j<current->triggered; j++ ) {
					if ( current->particles[j].age <= stage->timeToFade ) {
						smokeParticle *sParticle = &current->particles[j];
						sParticle->age += gameLocal.msec;
						sParticle->rotation += (sParticle->rotationSpeed * stage->rotationMul);
						idVec3 speed; // HUMANHEAD JRM - changed to vec3 from float
						if (sParticle->age >= stage->timeToFinalSpeed) {
							speed = stage->finalSpeed;
						} else {
							fraction = sParticle->age * stage->oneOverTimeToFinalSpeed;
							speed = stage->initialSpeed + (stage->finalSpeed - stage->initialSpeed) * fraction;
						}
						//HUMANHEAD: aob
						if (sParticle->age >= stage->timeToFinalGravity) {
							gravity = stage->finalGravity;
						} else {
							fraction = sParticle->age * stage->oneOverTimeToFinalGravity;
							gravity = stage->initialGravity + (stage->finalGravity - stage->initialGravity) * fraction;
						}
						//HUMANHEAD END
						if (sParticle->age != 0) {
							if (stage->slowSpawnSpeedOverLife) {
								fraction = 1.0f - ((float)j / current->numToTrigger);
								//HUMANHEAD: aob
								sParticle->position += DetermineDeltaPos( speed * fraction, sParticle->xyzmove, sParticle->direction );
								//HUMANHEAD END
							} else {
								//HUMANHEAD: aob
								sParticle->position += DetermineDeltaPos( speed, sParticle->xyzmove, sParticle->direction );
								//HUMANHEAD END
							}
							//HUMANHEAD: aob
							sParticle->position += gravity;
							//HUMANHEAD END
						}
						sParticle->position += mwpVector;
						triggerAlive = true;
						if ( sParticle->age >= stage->timeToFade ) {
							if( current->nozzle && stage->continuous ) {
								//HUMANHEAD: aob - moved logic into helper function
								ResetSmokeParticle( sParticle, current, stage );
								//HUMANHEAD END
							}
						}
					}
				}
				if (!triggerAlive) {
					current->state = SMOKE_DEAD;
				}
			}
		}
	}
}

/*
================
hhParticleSystem::ResetSmokeParticle
================
*/
void hhParticleSystem::ResetSmokeParticle( smokeParticle* particle, smokeGroup* group, idFXSmokeStage* smokeStage ) {
	float ang = 0.0f;
	float size = 0.0f;

	particle->position = (smokeStage->moveWithParent) ? group->oldGroupOrigin : group->groupOrigin;
	particle->direction = group->groupDirection;
	particle->rotation = gameLocal.random.RandomInt(360);
	particle->rotationSpeed = gameLocal.random.CRandomFloat() * 3.0f;
	// HUMANHEAD JRM - need to make spawn direction spherical - NOT box
	ang = gameLocal.random.RandomFloat() * hhMath::TWO_PI;
	size = smokeStage->xymove;
	if( !smokeStage->noXYMoveRandomness ) {
		size *= gameLocal.random.RandomFloat();
	}
	particle->xyzmove = idVec3( 0.0f, hhMath::Cos(ang) * size, hhMath::Sin(ang) * size );
	particle->xyzmove *= particle->direction.ToMat3();
	// HUMANHEAD JRM - end
	particle->age = 0;
}

/*
================
hhParticleSystem::DetermineDeltaPos
================
*/
idVec3 hhParticleSystem::DetermineDeltaPos( const idVec3& speed, const idVec3& xyzmove, const idVec3& dir ) {
	idVec3 deltaPos;
	//Convert to world coords
	idVec3 velocity = (dir + xyzmove) * dir.ToMat3().Transpose();

	for( int ix = 0; ix < 3; ++ix ) {
		deltaPos[ix] = speed[ix] * velocity[ix];
	}
	
	//Convert back to local coords
	return deltaPos * dir.ToMat3();
}

/*
================
hhParticleSystem::ReTrigger
================
*/
const int hhParticleSystem::ReTrigger( const idVec3& newOrigin, const idVec3& newDirection ) {

	// all triggers need to be checked, but all triggers have the same Num()  <-- optimize
	for( int i = 0; i < triggers[0].Num(); i++ ) {
		if ( StageFinished(i) ) {
			ResetStage(i, newOrigin, newDirection );
			return (myIndex<<16)|i;
		}
	}

	smokeParticle	initial;
	initial.position = vec3_origin;
	initial.rotation = 0.0f;
	initial.rotationSpeed = 0.0f;
	initial.direction = vec3_origin;
	//HUMANHEAD: aob
	initial.xyzmove.Zero();
	//HUMANHEAD END

	GetParticle();
	for( int i=0; i<particleSystem->events.Num(); i++ ) {
		smokeGroup* newGroup = new smokeGroup;

		newGroup->state = SMOKE_ALIVE;
		//HUMANHEAD: aob
		newGroup->triggered = 0;
		newGroup->minToTrigger = particleSystem->events[i]->minToTrigger;
		newGroup->maxToTrigger = particleSystem->events[i]->maxToTrigger;
		newGroup->numToTrigger = (particleSystem->events[i]->randomNumToTrigger) ? newGroup->minToTrigger + gameLocal.random.RandomInt(newGroup->maxToTrigger - newGroup->minToTrigger) : particleSystem->events[i]->numToTrigger;
		newGroup->particles.AssureSize( newGroup->numToTrigger, initial );
		//HUMANHEAD END
		newGroup->triggerEvery = particleSystem->events[i]->triggerEvery - gameLocal.random.RandomInt( particleSystem->events[i]->triggerEvery - particleSystem->events[i]->minTriggerEvery );
		newGroup->frameCount = newGroup->triggerEvery;
		//HUMANHEAD: aob
		newGroup->groupDirection = (newDirection == vec3_origin) ? defaultDir : newDirection;
		//HUMANHEAD END
		newGroup->groupOrigin = newOrigin;
		newGroup->oldGroupOrigin = newOrigin;
		newGroup->nozzle = true;
		newGroup->hidden = false;
		newGroup->baseTime = 0;
		triggers[i].Append( newGroup );
	}
	return (myIndex<<16)|(triggers[0].Num()-1);
}

/*
================
hhParticleSystem::ResetStage
================
*/
void hhParticleSystem::ResetStage( const int st, const idVec3 &start, const idVec3& direction ) {
	GetParticle();

	//HUMANHEAD: aob
	smokeParticle	initial;
	initial.position = vec3_origin;
	initial.rotation = 0.0f;
	initial.rotationSpeed = 0.0f;
	initial.direction = vec3_origin;
	initial.xyzmove.Zero();
	//HUMANHEAD END

	for( int i=0; i<particleSystem->events.Num(); i++ ) {
		smokeGroup* current = triggers[i][st];
		current->state = SMOKE_ALIVE;
		current->triggered = 0;
		//HUMANHEAD: aob
		current->minToTrigger = particleSystem->events[i]->minToTrigger;
		current->maxToTrigger = particleSystem->events[i]->maxToTrigger;
		current->numToTrigger = (particleSystem->events[i]->randomNumToTrigger) ? current->minToTrigger + gameLocal.random.RandomInt(current->maxToTrigger - current->minToTrigger) : particleSystem->events[i]->numToTrigger;
		current->particles.AssureSize( current->numToTrigger, initial );
		//HUMANHEAD END
		current->triggerEvery = particleSystem->events[i]->triggerEvery - gameLocal.random.RandomInt( particleSystem->events[i]->triggerEvery - particleSystem->events[i]->minTriggerEvery );
		current->frameCount = current->triggerEvery;
		current->groupOrigin = start;
		current->oldGroupOrigin = start;
		//HUMANHEAD: aob
		current->groupDirection = (direction == vec3_origin) ? defaultDir : direction;
		//HUMANHEAD END
		current->nozzle = true;
		current->hidden = false;
		current->baseTime = 0;
	}
}

/*
================
hhParticleSystem::SetNozzleOrigin
================
*/
void hhParticleSystem::SetNozzleDirection( const int handle, const idVec3 &vec ) {
	int st = TriggerForHandle( handle );
	GetParticle();
	for( int i=0; i<particleSystem->events.Num(); i++ ) {
		smokeGroup* current = triggers[i][st];
		//HUMANHEAD: aob
		current->groupDirection = (vec == vec3_origin) ? defaultDir : vec;
		//HUMANHEAD END
	}
}
#endif