
class idCommentaryNode : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idCommentaryNode );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );	

	void					OnFrob( idEntity* activator );

private:

	bool					frobbed;
	idVec3					orgOrigin;
};
