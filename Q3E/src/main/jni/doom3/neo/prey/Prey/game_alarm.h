#ifndef __GAME_ALARMLIGHT_H__
#define __GAME_ALARMLIGHT_H__

class hhAlarmLight : public idLight {
	CLASS_PROTOTYPE( hhAlarmLight );

public:
	void					Spawn();
	void					TurnOn();
	void					TurnOff();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

protected:
	void					SetParmState(float value6, float value7);
	void					Event_Activate(idEntity *activator);

	bool					bAlarmOn;
};

#endif
