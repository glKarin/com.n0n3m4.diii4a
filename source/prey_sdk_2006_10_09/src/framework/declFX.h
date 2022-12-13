// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __DECLFX_H__
#define __DECLFX_H__

/*
===============================================================================

	idDeclFX

===============================================================================
*/

enum { 
	FX_LIGHT,
	FX_PARTICLE,
	FX_DECAL,
	FX_MODEL,
	FX_SOUND,
	FX_SHAKE,
	FX_ATTACHLIGHT,
	// HUMANHEAD nla - Support for the 'useAxis'
	FX_USEAXIS,
	// HUMANHEAD nla
	FX_ATTACHENTITY,
	FX_LAUNCH,
	FX_SHOCKWAVE
};

//
// single fx structure
//
typedef struct {
	int						type;
	int						sibling;

	idStr					data;
	idStr					name;
	idStr					fire;

	// HUMANHEAD nla
	int					useAxis;
	idVec3				dir;
	// HUMANHEAD END

	float					delay;
	float					duration;
	float					restart;
	float					size;
	float					fadeInTime;
	float					fadeOutTime;
	float					shakeTime;
	float					shakeAmplitude;
	float					shakeDistance;
	float					shakeImpulse;
	float					lightRadius;
	float					rotate;
	float					random1;
	float					random2;

	idVec3					lightColor;
	idVec3					offset;
	idMat3					axis;

	bool					soundStarted;
	bool					shakeStarted;
	bool					shakeFalloff;
	bool					shakeIgnoreMaster;
	bool					bindParticles;
	bool					explicitAxis;
	bool					noshadows;
	bool					particleTrackVelocity;
	bool					trackOrigin;
} idFXSingleAction;

// HUMANHEAD nla
enum fxAxis { AXIS_CURRENT, AXIS_NORMAL, AXIS_BOUNCE, AXIS_INCOMING, AXIS_CUSTOMLOCAL };	
// HUMANHEAD END

//
// grouped fx structures
//
class idDeclFX : public idDecl {
public:
	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;
	virtual void			List( void ) const;

	idList<idFXSingleAction>events;
	idStr					joint;

private:
	void					ParseSingleFXAction( idLexer &src, idFXSingleAction& FXAction );

	// HUMANHEAD nla
	void	ParseUseAxis		( idStr &text, idFXSingleAction& action ) const;
	// HUMANHEAD END
};

#endif /* !__DECLFX_H__ */
