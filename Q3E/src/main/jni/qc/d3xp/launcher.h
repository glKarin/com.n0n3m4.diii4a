

#define LINECOUNT			16


class idLauncher : public idMoveableItem
{
public:
	CLASS_PROTOTYPE( idLauncher );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	void					OnFrob( idEntity* activator );


	void					OnWheelMove(int value);
	void					OnYaw(int value);
	void					OnPitch(int value);

	void					SetLaunchDir(idVec3 newdir, float newForce);

	void					OnGet( void );

private:

	void					UpdateMultiplier();
	void					launcherkill( void );

	idEntity				* peakEnt;
	idJumpPad				* jumpPad;

	int						state;

	idBeam					* beamStarts[LINECOUNT];
	idBeam					* beamEnds[LINECOUNT];

	idVec3					lastLaunchdir;
	float					lastForce;

	idVec3					launchDir;
	float					force;

	idEntity				* gatorEnt;
	idEntity				* remoteEnt;
	idEntity				* hitpointEnt;
	idEntity				* displaymodel;
	idEntity				* particles;


	int						nextAimrecord;

	int						remoteLerpStart;
	int						remoteLerpEnd;

	void					UpdateBeams();
	void					UpdateRemote();
	idVec3					GetGatorPos();
	idVec3					baseGatorPos;


	int						resettime_up;
	int						resettime_down;
	int						resettime_left;
	int						resettime_right;

	void					MoveBone(const char *bonename, idVec3 direction);
	int						wheelValue;
	int						wheelMovetime;
	int						wheelMovedir;

	int						beamUpdateTime;	

	float					remoteRaiseLerp;
	int						lastGametime;

	idVec3					hitpointPos;
};
