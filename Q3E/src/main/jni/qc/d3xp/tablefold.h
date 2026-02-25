
class idTableFold : public idAnimatedEntity
{
public:
							CLASS_PROTOTYPE( idTableFold );

	//void					Think( void );
	void					Spawn( void );
	void					OnFrob( idEntity* activator );

private:

	enum					{ CLOSED, OPENED };
	int						state;


	void					Event_reset( void );
	void					Event_open( bool value );


	idEntity*				frobcube;
	idEntity*				frobcube_handle;

	idMover *				mover;
};


