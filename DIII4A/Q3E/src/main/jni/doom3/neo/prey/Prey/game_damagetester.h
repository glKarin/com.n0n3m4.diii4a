#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __GAME_DAMAGETESTER_H__
#define __GAME_DAMAGETESTER_H__

typedef enum hhDamageDist_s {
	DD_MELEE,
	DD_CLOSE,
	DD_MEDIUM,
	DD_FAR,
	DD_MAX
} hhDamageDist_t;

class hhDamageTester : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhDamageTester );	

	~hhDamageTester();
	void		Spawn(void);
	void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void		Event_ResetTarget( void );
	void		Event_CheckRemove( void );

protected:

	bool			bUndamaged;
	int				targetIndex;
	float			testTime;

	idVec3			originalLocation;
	idMat3			originalAxis;

	int				totalDamage[DD_MAX];
	int				hitCount[DD_MAX];
	idVec3			distance[DD_MAX]; // Relatives distances for each target location (relative to axis)
	const char		*weaponName;
};

#endif /* __GAME_DAMAGETESTER_H__ */
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build