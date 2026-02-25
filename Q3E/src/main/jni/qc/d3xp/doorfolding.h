
#define BARCOUNT	8

class idDoorFolding : public idEntity
{
public:
							CLASS_PROTOTYPE( idDoorFolding );

	void					Think( void );
	void					Spawn( void );

private:

	enum					{ OFF, ON };
	int						state;

	void					Event_activate( int value );

	idMover					* movers[BARCOUNT];


	idBeam*					beam[BARCOUNT*8];

};

