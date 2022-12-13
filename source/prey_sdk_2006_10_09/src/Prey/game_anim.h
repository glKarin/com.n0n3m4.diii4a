
#ifndef __PREY_GAME_ANIM_H__
#define __PREY_GAME_ANIM_H__

class hhAnim : public idAnim {

public:	

					hhAnim() : idAnim() { exactMatch = false; }
					hhAnim( const idDeclModelDef *modelDef, const idAnim *anim ) : idAnim( modelDef, anim ) { }

	virtual bool	AddFrameCommandExtra( idToken &token, frameCommand_t &fc, idLexer &src, idStr &errorText );
	virtual bool	CallFrameCommandsExtra( const frameCommand_t &command, idEntity *ent ) const;	

	bool			exactMatch;

protected:
	virtual idStr 	InitFrameCommandEvent( frameCommand_t &command, const idStr& cmdStr ) const;
	
};


#endif /* __PREY_GAME_ANIM_H__ */
