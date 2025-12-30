#ifndef __WL_STATE_H__
#define __WL_STATE_H__

/*
=============================================================================

							WL_STATE DEFINITIONS

=============================================================================
*/

void    InitHitRect (AActor *ob, unsigned radius);

bool TrySpot (AActor *ob, MapSpot spot);
bool TryWalk (AActor *ob);
void SelectChaseDir (AActor *ob);
void SelectDodgeDir (AActor *ob);
void SelectRunDir (AActor *ob);
void SelectWanderDir (AActor *ob);
bool MoveObj (AActor *ob, int32_t move);
bool SightPlayer (AActor *ob, double minseedist, double maxseedist, double maxheardist, double fov, const Frame *state);

void    DamageActor (AActor *ob, AActor *attacker, unsigned damage);

bool CheckSlidePass(unsigned int style, unsigned int intercept, unsigned int amount);
bool CheckLine (AActor *ob, AActor *ob2);

#endif
