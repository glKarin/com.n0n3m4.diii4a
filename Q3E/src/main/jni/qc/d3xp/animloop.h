
class idAnimloop : public idAnimated
{
public:
	CLASS_PROTOTYPE( idAnimloop );


	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );


	//void					Think( void );
	void					Spawn( void );





private:
	void					Loopdone( void );

	int						blendFrames;
	float					randomtime;
	float					randomstart;
	idStr					animName;

	bool					active;
	void					loopsetactive(int value);



};
