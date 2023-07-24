// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLINVSLOT_H__
#define __DECLINVSLOT_H__

class sdDeclInvSlot : public idDecl {
public:
							sdDeclInvSlot( void );
	virtual					~sdDeclInvSlot( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	int						GetBank( void ) const { return bank; }
	const char*				GetTitle( void ) const { return title; }

protected:
	int						bank;
	idStr					title;
};

#endif // __DECLINVSLOT_H__
