//----------------------------------------------------------------
// Icon.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef	__ICON_H__
#define	__ICON_H__

class rvIcon {
public:
	rvIcon();
	~rvIcon();
	void		UpdateIcon( const idVec3 &origin, const idMat3 &axis );
	qhandle_t	CreateIcon( const char *mtr, int suppressViewID = 0 );
	void		FreeIcon( void );
	qhandle_t	GetHandle( void ) const;

	int			GetHeight( void ) const;
	int			GetWidth( void ) const;
private:
	void	Draw( jointHandle_t joint );
	void	Draw( const idVec3 &origin );

	renderEntity_t		renderEnt;
	qhandle_t			iconHandle;
};

ID_INLINE qhandle_t rvIcon::GetHandle( void ) const {
	return iconHandle;
}

#endif	/* !_ICON_H_ */

