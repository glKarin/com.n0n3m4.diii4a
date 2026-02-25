
class idCamturret : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idCamturret );

	void					Spawn( void );

	virtual void			Think( void );

	void					GotoAlert( void );

private:

	enum					{ OFF, OPENING, SEARCHING, SUSPICIOUS, ALERTED };
	int						state;

	int						nextStateTime;
	int						nextSearchTime;

	bool					HasLOS();
	void					PointCamera(const char* jointName, idVec3 aimPos);

	void					Event_camturretactivate( int value );
	int						Event_PlayAnim( const char* animname, bool loop = false);
};
