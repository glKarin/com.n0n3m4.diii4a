
#ifndef __PREY_AI_MUTATE_H__
#define __PREY_AI_MUTATE_H__

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
class hhMutate : public hhMonsterAI {
public:
	CLASS_PROTOTYPE(hhMutate);
protected:
	void Event_OnProjectileLaunch(hhProjectile *proj);
	void LinkScriptVariables();
	idScriptBool AI_COMBAT;
};
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#endif